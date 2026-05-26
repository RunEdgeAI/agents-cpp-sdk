/**
 * @file ui.hpp
 * @brief In-Car AI HTTP UI server
 */
#pragma once

#include <agents-cpp/agents/voice_agent.h>
#include <agents-cpp/http_client.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/utils.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

using namespace agents;

// Shared vehicle state written by tool callbacks in main, read by HTTP handlers here.
// Authoritative state for the currently active navigation route. Written by
// the in-process nav tools (set_destination / add_stop / remove_stop / clear)
// and read by get_active_route — the model never recalls route state from
// prior tool results, it asks for it.
struct RouteLeg {
    std::string destination;       // human-readable end of this leg (mirrors stops[i+1])
    std::string duration_text;     // "47 min"
    std::string distance_text;     // "55.0 mi"
};

struct ActiveRoute {
    std::vector<std::string> stops;       // origin first, final destination last
    std::vector<RouteLeg> legs;           // one per leg between consecutive stops
    std::string total_duration_text;      // "1 hr 39 min"
    std::string total_distance_text;      // "92.5 mi"
    bool active = false;
};

struct VehicleState {
    std::mutex  nav_mutex;
    std::string nav_json;

    std::mutex  route_mutex;
    ActiveRoute active_route;

    std::mutex  ac_mutex;
    std::string ac_json;

    std::mutex  loc_mutex;
    std::string loc_json;   // {"latitude":..., "longitude":..., "address":...}

    std::mutex  weather_mutex;
    std::string weather_json;  // {"temperature":N, "units":"F"|"C", "summary":"..."}

    std::mutex  music_mutex;
    std::string music_text;    // raw "Now playing:" line from itunes/applemusic tools
    bool        music_playing = false;
    std::chrono::steady_clock::time_point music_last_check{};
};

// Heuristic: treat a tool's textual result as a now-playing report if it
// matches the shapes the front-end already parses ("Now playing: ...",
// "Name: ..., Artist: ...", or "Track - Artist - Album"). Lets us capture
// music updates from any agent-driven tool without hard-coding tool names.
inline bool looksLikeNowPlaying(const std::string& s) {
    static const std::regex re(
        R"((^|\n)\s*Now playing:\s*\S|(^|\n)\s*(Name|Track)\s*:)",
        std::regex::icase);
    return std::regex_search(s, re);
}

// True when `name` is a tool we know reports current song state.
inline bool isMusicNowPlayingTool(const std::string& name) {
    if (name == "itunes_current_song") return true;
    static const std::regex re(
        R"((^|_)(now_playing|current_(song|track)|nowplaying)($|_))",
        std::regex::icase);
    return std::regex_search(name, re);
}

// Recognize transport-control tools (play / pause / stop / next / previous /
// resume) from the iTunes/Apple Music MCPs. On match, `outAction` is set to
// the lowercase verb so the caller can decide the resulting play_state.
inline bool isMusicTransportTool(const std::string& name, std::string& outAction) {
    static const std::regex re(
        R"(^(?:itunes|applemusic)[_-]?(play|pause|stop|next|previous|resume)$)",
        std::regex::icase);
    std::smatch m;
    if (!std::regex_match(name, m, re)) return false;
    outAction = m[1].str();
    std::transform(outAction.begin(), outAction.end(), outAction.begin(), ::tolower);
    return true;
}

// Parse a wttr.in `format=4` response (e.g. "New York: ⛅ +59°F") into a
// structured JSON snapshot for the UI chip. Best-effort; returns "{}" if the
// shape isn't recognized.
inline std::string parseWeatherSnapshot(const std::string& wttr_text) {
    std::smatch m;
    std::regex temp_re(R"(([+-]?\d+)\s*°\s*([CF]))");
    if (!std::regex_search(wttr_text, m, temp_re)) return "{}";
    JsonObject out;
    try { out["temperature"] = std::stoi(m[1].str()); } catch (...) { return "{}"; }
    out["units"] = m[2].str();
    out["summary"] = wttr_text;
    return out.dump();
}

// Routes API v2 (what @cablate/mcp-google-map's `maps_directions` returns)
// → Directions API v1 (what the local `navigate` tool produces and what the
// front-end was built around). Detection is by presence of v2-only keys.
namespace incar_nav {

inline JsonObject routesV2ToDirectionsV1(const JsonObject& v2) {
    auto parseSeconds = [](const std::string& s) -> int {
        if (s.empty()) return 0;
        try { return std::stoi(s.back() == 's' ? s.substr(0, s.size() - 1) : s); }
        catch (...) { return 0; }
    };
    auto secondsToText = [](int t) {
        return t >= 60 ? std::to_string(t / 60) + " mins" : std::to_string(t) + " s";
    };
    auto metersToText = [](int m) {
        // Imperial units (matches AGENTS.md guidance)
        double feet = m * 3.28084;
        if (feet < 528.0) {  // < 0.1 mi → use feet
            char buf[32]; std::snprintf(buf, sizeof(buf), "%d ft", static_cast<int>(feet));
            return std::string(buf);
        }
        char buf[32]; std::snprintf(buf, sizeof(buf), "%.1f mi", m * 0.000621371);
        return std::string(buf);
    };
    auto extractLatLng = [](const JsonObject& loc) {
        JsonObject pt;
        if (loc.contains("latLng")) {
            pt["lat"] = loc["latLng"].value("latitude", 0.0);
            pt["lng"] = loc["latLng"].value("longitude", 0.0);
        }
        return pt;
    };

    JsonObject v1{{"status", "OK"}, {"routes", JsonObject::array()}};
    if (!v2.contains("routes") || !v2["routes"].is_array()) return v1;

    for (const auto& v2Route : v2["routes"]) {
        JsonObject v1Route{{"summary", v2Route.value("description", "")}, {"legs", JsonObject::array()}};
        if (v2Route.contains("polyline") && v2Route["polyline"].contains("encodedPolyline")) {
            v1Route["overview_polyline"] = JsonObject{{"points", v2Route["polyline"]["encodedPolyline"]}};
        }
        if (v2Route.contains("legs") && v2Route["legs"].is_array()) {
            for (const auto& v2Leg : v2Route["legs"]) {
                int legM = v2Leg.value("distanceMeters", 0);
                int legS = parseSeconds(v2Leg.value("duration", std::string("0s")));
                JsonObject v1Leg{
                    {"distance", JsonObject{{"value", legM}, {"text", metersToText(legM)}}},
                    {"duration", JsonObject{{"value", legS}, {"text", secondsToText(legS)}}},
                    {"steps",    JsonObject::array()},
                };
                if (v2Leg.contains("startLocation")) v1Leg["start_location"] = extractLatLng(v2Leg["startLocation"]);
                if (v2Leg.contains("endLocation"))   v1Leg["end_location"]   = extractLatLng(v2Leg["endLocation"]);
                if (v2Leg.contains("steps") && v2Leg["steps"].is_array()) {
                    for (const auto& s : v2Leg["steps"]) {
                        std::string instr;
                        std::string maneuver;
                        if (s.contains("navigationInstruction")) {
                            instr    = s["navigationInstruction"].value("instructions", "");
                            maneuver = s["navigationInstruction"].value("maneuver", "");
                        }
                        int sm = s.value("distanceMeters", 0);
                        int ss = parseSeconds(s.value("staticDuration", std::string("0s")));
                        JsonObject v1Step{
                            {"html_instructions", instr},
                            {"distance", JsonObject{{"value", sm}, {"text", metersToText(sm)}}},
                            {"duration", JsonObject{{"value", ss}, {"text", secondsToText(ss)}}},
                        };
                        if (!maneuver.empty()) v1Step["maneuver"] = maneuver;
                        if (s.contains("startLocation")) v1Step["start_location"] = extractLatLng(s["startLocation"]);
                        if (s.contains("endLocation"))   v1Step["end_location"]   = extractLatLng(s["endLocation"]);
                        v1Leg["steps"].push_back(v1Step);
                    }
                }
                v1Route["legs"].push_back(v1Leg);
            }
        }
        v1["routes"].push_back(v1Route);
    }
    return v1;
}

// Returns nav_json normalized to v1 shape if it looks like v2; otherwise
// passes it through unchanged. Empty string -> empty (caller handles 204).
inline std::string normalizeNavJson(const std::string& raw) {
    if (raw.empty()) return raw;
    JsonObject parsed;
    try { parsed = JsonObject::parse(raw); } catch (...) { return raw; }

    // v1 has html_instructions and a status field; v2 has navigationInstruction
    // (and lacks status). One probe is enough to disambiguate.
    if (parsed.contains("routes") && parsed["routes"].is_array() && !parsed["routes"].empty()) {
        const auto& legs = parsed["routes"][0].value("legs", JsonObject::array());
        if (!legs.empty()) {
            const auto& steps = legs[0].value("steps", JsonObject::array());
            if (!steps.empty() && steps[0].contains("navigationInstruction")) {
                return routesV2ToDirectionsV1(parsed).dump();
            }
        }
    }
    return raw;  // already v1 (or shape we don't recognize — pass through)
}

}  // namespace incar_nav

class InCarUI {
public:
    InCarUI();
    ~InCarUI();

    InCarUI(const InCarUI&) = delete;
    InCarUI& operator=(const InCarUI&) = delete;

    void onPartialResponse(const std::string& chunk);
    void onFinalResponse(const std::string&);
    void start(VoiceAgent& agent,
               std::shared_ptr<Context> ctx,
               const VoiceAgent::Config& cfg,
               VehicleState& state,
               int port = 9000);
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
