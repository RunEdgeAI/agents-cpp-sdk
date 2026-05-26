/**
 * @file nav_tools.hpp
 * @brief In-process stateful navigation tools for the in-car demo.
 *
 * Five tools manage an authoritative ActiveRoute in VehicleState:
 *   - set_destination(destination)
 *   - add_stop(address, position?)
 *   - remove_stop(index)
 *   - clear_route()
 *   - get_active_route()
 *
 * The upstream Google MCP routing tools (maps_directions, maps_plan_route)
 * are removed from the agent's catalog after registration. Captured
 * shared_ptrs keep them callable from inside these tools.
 *
 * Header-only. Include after ui.hpp.
 */
#pragma once

#include "ui.hpp"

#include <agents-cpp/context.h>
#include <agents-cpp/tool.h>
#include <agents-cpp/types.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// `incar_nav` namespace is also opened by ui.hpp (helpers there); we extend
// it at file scope so both call sites resolve to the same namespace.
namespace incar_nav {

using namespace agents;

// ── formatting helpers ────────────────────────────────────────────────────

inline std::string formatMilesText(int meters) {
    if (meters <= 0) return "";
    double feet = meters * 3.28084;
    char buf[32];
    if (feet < 528.0) std::snprintf(buf, sizeof(buf), "%d ft", static_cast<int>(feet));
    else std::snprintf(buf, sizeof(buf), "%.1f mi", meters * 0.000621371);
    return std::string(buf);
}

inline std::string formatDurationText(int seconds) {
    if (seconds <= 0) return "";
    int mins = (seconds + 30) / 60;
    if (mins >= 60) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d hr %d min", mins / 60, mins % 60);
        return std::string(buf);
    }
    return std::to_string(mins) + " min";
}

// ── result-parsing helpers ────────────────────────────────────────────────

// Unwrap the MCP envelope to get the inner JSON-text payload.
inline std::string unwrapEnvelope(const ToolResult& r) {
    if (r.data.contains("content") && r.data["content"].is_array() &&
        !r.data["content"].empty()) {
        return r.data["content"][0].value("text", std::string{});
    }
    return r.content;
}

struct LegMetrics { int seconds = 0; int meters = 0; };

// Walk routes[0].legs[] and return per-leg (seconds, meters). Handles both
// v1 (`duration.value`/`distance.value`) and v2 (`duration: "1234s"`/
// `distanceMeters`) shapes — `maps_plan_route` and `maps_directions` from
// the Google MCP can return either depending on path.
inline std::vector<LegMetrics> extractLegMetrics(const std::string& text) {
    std::vector<LegMetrics> out;
    try {
        auto j = JsonObject::parse(text);
        if (!j.contains("routes") || !j["routes"].is_array() || j["routes"].empty())
            return out;
        const auto& route = j["routes"][0];
        if (!route.contains("legs") || !route["legs"].is_array()) return out;
        for (const auto& leg : route["legs"]) {
            LegMetrics m;
            if (leg.contains("duration") && leg["duration"].is_object())
                m.seconds = leg["duration"].value("value", 0);
            else if (leg.contains("duration") && leg["duration"].is_string()) {
                std::string d = leg["duration"].get<std::string>();
                if (!d.empty() && d.back() == 's') d.pop_back();
                try { m.seconds = std::stoi(d); } catch (...) {}
            }
            if (leg.contains("distance") && leg["distance"].is_object())
                m.meters = leg["distance"].value("value", 0);
            else if (leg.contains("distanceMeters"))
                m.meters = leg.value("distanceMeters", 0);
            out.push_back(m);
        }
    } catch (...) {}
    return out;
}

// Convert any nav_json text's top-level total_distance to imperial in-place
// (matches what the old onAfterToolExecution did before we owned routing).
inline std::string convertToImperial(const std::string& text) {
    try {
        auto j = JsonObject::parse(text);
        if (j.contains("total_distance") && j["total_distance"].contains("value")) {
            j["total_distance"]["text"] = formatMilesText(j["total_distance"].value("value", 0));
        }
        return j.dump();
    } catch (...) {
        return text;
    }
}

// ── origin resolution ────────────────────────────────────────────────────

// Resolve the current vehicle origin into "lat,lng" form. Prefers the cached
// VehicleState loc_json (populated by ipapi seeding); falls back to invoking
// the get_location tool.
inline std::string fetchOrigin(Context& ctx, VehicleState& state) {
    auto formatLatLng = [](double lat, double lng) {
        char buf[64]; std::snprintf(buf, sizeof(buf), "%.4f,%.4f", lat, lng);
        return std::string(buf);
    };
    std::string loc;
    { std::lock_guard<std::mutex> lk(state.loc_mutex); loc = state.loc_json; }
    Logger::info("[nav_tools] fetchOrigin: loc_json_bytes={}, preview={}",
                 loc.size(), loc.substr(0, std::min<size_t>(loc.size(), 200)));
    if (!loc.empty()) {
        try {
            auto j = JsonObject::parse(loc);
            double lat = j.value("latitude", 0.0);
            double lng = j.value("longitude", 0.0);
            Logger::info("[nav_tools] fetchOrigin: parsed lat={}, lng={}", lat, lng);
            if (lat != 0.0 || lng != 0.0) return formatLatLng(lat, lng);
        } catch (const std::exception& e) {
            Logger::warn("[nav_tools] fetchOrigin: parse failed: {}", e.what());
        }
    }
    if (auto getloc = ctx.getTool("get_location")) {
        Logger::info("[nav_tools] fetchOrigin: calling get_location fallback");
        auto r = getloc->execute({{"address_needed", false}});
        Logger::info("[nav_tools] fetchOrigin: get_location success={}, data={}",
                     r.success, r.data.dump().substr(0, 200));
        if (r.success && r.data.contains("location") && r.data["location"].is_object()) {
            const auto& l = r.data["location"];
            double lat = l.value("latitude", 0.0);
            double lng = l.value("longitude", 0.0);
            if (lat != 0.0 || lng != 0.0) return formatLatLng(lat, lng);
        }
    } else {
        Logger::warn("[nav_tools] fetchOrigin: get_location tool not in registry");
    }
    Logger::warn("[nav_tools] fetchOrigin: returning empty origin");
    return "";
}

// ── state writers ─────────────────────────────────────────────────────────

// Pull the first leg's start_location (v2 shape: routes[0].legs[0].startLocation.latLng)
// out of the route text, if present. Used to snap loc_json to the route origin so the
// purple location marker stays aligned with the green route-start marker.
inline std::pair<double, double> extractRouteOrigin(const std::string& text) {
    try {
        auto j = JsonObject::parse(text);
        if (j.contains("routes") && j["routes"].is_array() && !j["routes"].empty()) {
            const auto& route = j["routes"][0];
            if (route.contains("legs") && route["legs"].is_array() && !route["legs"].empty()) {
                const auto& leg = route["legs"][0];
                if (leg.contains("startLocation") && leg["startLocation"].contains("latLng")) {
                    const auto& ll = leg["startLocation"]["latLng"];
                    return {ll.value("latitude", 0.0), ll.value("longitude", 0.0)};
                }
            }
        }
    } catch (...) {}
    return {0.0, 0.0};
}

inline void persistRoute(VehicleState& state,
                         const std::vector<std::string>& stops,
                         const ToolResult& r) {
    std::string text = convertToImperial(unwrapEnvelope(r));
    { std::lock_guard<std::mutex> lk(state.nav_mutex); state.nav_json = text; }

    // Snap loc_json to the route's first-leg start_location so the UI's purple
    // location marker sits on top of the green route-start marker. See ROADMAP
    // "Real-vehicle movement support" for why this is OK for a stationary demo
    // but needs to come apart once real GPS tracking is wired in.
    auto [origin_lat, origin_lng] = extractRouteOrigin(text);
    if (origin_lat != 0.0 || origin_lng != 0.0) {
        JsonObject loc;
        loc["latitude"] = origin_lat;
        loc["longitude"] = origin_lng;
        std::lock_guard<std::mutex> lk(state.loc_mutex);
        state.loc_json = loc.dump();
    }

    auto leg_metrics = extractLegMetrics(text);

    ActiveRoute ar;
    ar.stops = stops;
    ar.active = true;
    int total_s = 0, total_m = 0;
    for (size_t i = 0; i < leg_metrics.size(); ++i) {
        total_s += leg_metrics[i].seconds;
        total_m += leg_metrics[i].meters;
        RouteLeg leg;
        leg.destination   = (i + 1 < stops.size()) ? stops[i + 1] : std::string{};
        leg.duration_text = formatDurationText(leg_metrics[i].seconds);
        leg.distance_text = formatMilesText(leg_metrics[i].meters);
        ar.legs.push_back(std::move(leg));
    }
    ar.total_duration_text = formatDurationText(total_s);
    ar.total_distance_text = formatMilesText(total_m);

    { std::lock_guard<std::mutex> lk(state.route_mutex); state.active_route = std::move(ar); }
}

inline void clearRoute(VehicleState& state) {
    { std::lock_guard<std::mutex> lk(state.nav_mutex); state.nav_json.clear(); }
    { std::lock_guard<std::mutex> lk(state.route_mutex); state.active_route = ActiveRoute{}; }
}

// ── polyline encode/decode + multi-leg route stitching ───────────────────

// Decode Google's encoded polyline format to a list of (lat*1e5, lng*1e5) ints.
inline std::vector<std::pair<int,int>> decodePolyline(const std::string& enc) {
    std::vector<std::pair<int,int>> out;
    size_t i = 0;
    int lat = 0, lng = 0;
    while (i < enc.size()) {
        auto read = [&](int& dst) {
            int shift = 0, result = 0, b;
            do {
                if (i >= enc.size()) return;
                b = static_cast<int>(static_cast<unsigned char>(enc[i++])) - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            dst += (result & 1) ? ~(result >> 1) : (result >> 1);
        };
        read(lat);
        read(lng);
        out.emplace_back(lat, lng);
    }
    return out;
}

inline std::string encodePolyline(const std::vector<std::pair<int,int>>& points) {
    std::string out;
    int prev_lat = 0, prev_lng = 0;
    auto encode = [&](int v) {
        unsigned u = (v < 0) ? ~(static_cast<unsigned>(v) << 1) : (static_cast<unsigned>(v) << 1);
        while (u >= 0x20) {
            out.push_back(static_cast<char>((0x20 | (u & 0x1f)) + 63));
            u >>= 5;
        }
        out.push_back(static_cast<char>(u + 63));
    };
    for (const auto& [lat, lng] : points) {
        encode(lat - prev_lat);
        encode(lng - prev_lng);
        prev_lat = lat;
        prev_lng = lng;
    }
    return out;
}

// Compute a route through N stops by calling maps_directions per leg and
// stitching the v2 responses into a single combined v2-shaped result. This
// is necessary because the @cablate MCP's maps_plan_route returns a custom
// simplified shape (no polyline, no steps) that the UI can't render.
inline ToolResult computeRoute(std::shared_ptr<Tool> directions,
                                const std::vector<std::string>& stops) {
    if (stops.size() < 2) {
        return ToolError(JsonObject{{"error", "need at least origin and destination"}});
    }
    if (!directions) {
        return ToolError(JsonObject{{"error", "directions backend unavailable"}});
    }

    // Single-leg fast path.
    if (stops.size() == 2) {
        return directions->execute({{"origin", stops[0]}, {"destination", stops[1]}});
    }

    // Multi-leg: stitch per-leg v2 responses into one route with combined legs[]
    // and a concatenated polyline (decoded + re-encoded so deltas chain correctly).
    JsonObject combined_legs = JsonObject::array();
    std::vector<std::pair<int,int>> combined_points;

    for (size_t i = 0; i + 1 < stops.size(); ++i) {
        auto r = directions->execute({{"origin", stops[i]}, {"destination", stops[i + 1]}});
        if (!r.success) return r;
        std::string text = unwrapEnvelope(r);
        try {
            auto j = JsonObject::parse(text);
            if (!j.contains("routes") || !j["routes"].is_array() || j["routes"].empty()) continue;
            const auto& route = j["routes"][0];
            if (route.contains("legs") && route["legs"].is_array()) {
                for (const auto& leg : route["legs"]) combined_legs.push_back(leg);
            }
            if (route.contains("polyline") && route["polyline"].contains("encodedPolyline")) {
                auto pts = decodePolyline(route["polyline"]["encodedPolyline"].get<std::string>());
                // Skip first point of subsequent legs to avoid duplicates at junctions.
                size_t start = combined_points.empty() ? 0 : 1;
                for (size_t k = start; k < pts.size(); ++k) combined_points.push_back(pts[k]);
            }
        } catch (const std::exception& e) {
            return ToolError(JsonObject{{"error",
                std::string("failed to parse leg ") + std::to_string(i) + ": " + e.what()}});
        }
    }

    JsonObject route_obj;
    route_obj["legs"] = combined_legs;
    if (!combined_points.empty()) {
        route_obj["polyline"] = JsonObject{{"encodedPolyline", encodePolyline(combined_points)}};
    }
    JsonObject combined;
    combined["routes"] = JsonObject::array();
    combined["routes"].push_back(route_obj);

    std::string text_out = combined.dump();
    ToolResult out;
    out.success = true;
    out.content = text_out;
    // Match the MCP envelope shape (data is the content array) so unwrapEnvelope
    // and downstream consumers work uniformly.
    out.data = JsonObject::array();
    out.data.push_back(JsonObject{{"type", "text"}, {"text", text_out}});
    return out;
}

// ── tool registration ────────────────────────────────────────────────────

inline void registerNavTools(Context& ctx, VehicleState& state) {
    auto directions = ctx.getTool("maps_directions");
    if (!directions) {
        Logger::warn("[nav_tools] maps_directions not loaded; nav tools will be unable to compute routes");
    }

    auto compute_route = [directions](const std::vector<std::string>& stops) -> ToolResult {
        return computeRoute(directions, stops);
    };

    // set_destination
    ctx.registerTool(createTool(
        "set_destination",
        "Start a new navigation route to a single destination. REPLACES any active route — "
        "use add_stop instead to augment an existing route. Origin is determined automatically "
        "from the vehicle's current location. Pass the destination as a street address, place "
        "name, or 'lat,lng' string. For ambiguous city names ('Santa Cruz', 'Portland', "
        "'Cambridge'), include the state or country: 'Santa Cruz, CA'.",
        { Parameter{"destination",
                    "Destination address, place name, or 'lat,lng' coordinates.",
                    "string", true} },
        [&ctx, &state, compute_route](const JsonObject& params) -> ToolResult {
            std::string dest = params.value("destination", std::string{});
            if (dest.empty()) return ToolError(JsonObject{{"error", "destination is required"}});
            std::string origin = fetchOrigin(ctx, state);
            if (origin.empty()) return ToolError(JsonObject{{"error", "could not determine current location"}});
            auto r = compute_route({origin, dest});
            if (!r.success) { clearRoute(state); return r; }
            persistRoute(state, {origin, dest}, r);
            return r;
        }
    ));

    // add_stop
    ctx.registerTool(createTool(
        "add_stop",
        "Add a stop to the currently active route. By default the stop is inserted before the "
        "final destination ('on the way' semantics). Pass an optional 0-based 'position' to "
        "insert at a specific spot among the intermediate stops. Errors if no route is active — "
        "start one with set_destination first.",
        {
            Parameter{"address",  "Address or place name to add as a stop.",                       "string",  true},
            Parameter{"position", "Optional 0-based insertion index among intermediate stops.",    "integer", false},
        },
        [&state, compute_route](const JsonObject& params) -> ToolResult {
            std::string addr = params.value("address", std::string{});
            if (addr.empty()) return ToolError(JsonObject{{"error", "address is required"}});
            std::vector<std::string> stops;
            {
                std::lock_guard<std::mutex> lk(state.route_mutex);
                if (!state.active_route.active)
                    return ToolError(JsonObject{{"error", "no active route; call set_destination first"}});
                stops = state.active_route.stops;
            }
            if (stops.size() < 2) return ToolError(JsonObject{{"error", "invalid route state"}});
            int insert_at = static_cast<int>(stops.size()) - 1;  // default: before final
            if (params.contains("position") && params["position"].is_number_integer()) {
                int pos = params["position"].get<int>();
                int abs_pos = 1 + pos;
                int max_pos = static_cast<int>(stops.size()) - 1;
                insert_at = std::max(1, std::min(abs_pos, max_pos));
            }
            stops.insert(stops.begin() + insert_at, addr);
            auto r = compute_route(stops);
            if (!r.success) return r;
            persistRoute(state, stops, r);
            return r;
        }
    ));

    // remove_stop
    ctx.registerTool(createTool(
        "remove_stop",
        "Remove an intermediate stop from the active route by 0-based index (index 0 = first "
        "intermediate stop; does NOT include origin or final destination). The route is "
        "recomputed automatically. To find the right index, call get_active_route first.",
        { Parameter{"index", "0-based index among intermediate stops.", "integer", true} },
        [&state, compute_route](const JsonObject& params) -> ToolResult {
            if (!params.contains("index") || !params["index"].is_number_integer())
                return ToolError(JsonObject{{"error", "index is required"}});
            int idx = params["index"].get<int>();
            std::vector<std::string> stops;
            {
                std::lock_guard<std::mutex> lk(state.route_mutex);
                if (!state.active_route.active) return ToolError(JsonObject{{"error", "no active route"}});
                stops = state.active_route.stops;
            }
            int abs_pos = 1 + idx;
            if (abs_pos < 1 || abs_pos >= static_cast<int>(stops.size()) - 1)
                return ToolError(JsonObject{{"error", "index out of range for intermediate stops"}});
            stops.erase(stops.begin() + abs_pos);
            auto r = compute_route(stops);
            if (!r.success) return r;
            persistRoute(state, stops, r);
            return r;
        }
    ));

    // clear_route
    ctx.registerTool(createTool(
        "clear_route",
        "Cancel the currently active navigation route. The map UI returns to no-nav state.",
        {},
        [&state](const JsonObject&) -> ToolResult {
            clearRoute(state);
            return ToolResult{true, "Route cleared", JsonObject{{"cleared", true}}};
        }
    ));

    // get_active_route
    ctx.registerTool(createTool(
        "get_active_route",
        "Read the currently active route. Returns: active (bool), stops (ordered list), legs "
        "(per-leg destination/duration/distance), total_duration_text, total_distance_text, "
        "next_stop_address, final_destination. Call this WHENEVER the user asks 'how long "
        "until we get there', 'where are we going', 'what's my next stop', or any other "
        "question about the current route — do NOT try to recall route details from earlier "
        "tool results.",
        {},
        [&state](const JsonObject&) -> ToolResult {
            ActiveRoute snap;
            { std::lock_guard<std::mutex> lk(state.route_mutex); snap = state.active_route; }
            JsonObject out;
            out["active"] = snap.active;
            if (!snap.active) {
                out["message"] = "No route is currently active.";
                return ToolResult{true, out.dump(), out};
            }
            out["stops"] = snap.stops;
            JsonObject legs_arr = JsonObject::array();
            for (const auto& l : snap.legs) {
                legs_arr.push_back(JsonObject{
                    {"destination",   l.destination},
                    {"duration_text", l.duration_text},
                    {"distance_text", l.distance_text},
                });
            }
            out["legs"] = legs_arr;
            out["total_duration_text"] = snap.total_duration_text;
            out["total_distance_text"] = snap.total_distance_text;
            out["next_stop_address"]   = (snap.stops.size() >= 2) ? snap.stops[1] : "";
            out["final_destination"]   = snap.stops.empty() ? "" : snap.stops.back();
            return ToolResult{true, out.dump(), out};
        }
    ));

    // Hide the upstream Google routing tools from the model. Captured
    // shared_ptrs above keep them callable from inside our tools.
    ctx.removeTool("maps_directions");
    ctx.removeTool("maps_plan_route");

    Logger::info("[nav_tools] Registered 5 in-process nav tools; "
                 "removed maps_directions and maps_plan_route from the model's catalog");
    // Verify registration landed.
    for (const char* n : {"set_destination","add_stop","remove_stop","clear_route","get_active_route"}) {
        auto t = ctx.getTool(n);
        Logger::info("[nav_tools] post-reg check: {} ptr={}, params_count={}",
                     n, (void*)t.get(), t ? t->getParameters().size() : 0);
    }
}

}  // namespace incar_nav
