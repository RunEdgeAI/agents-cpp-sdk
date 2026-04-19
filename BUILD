# Main library build recipe

cc_import(
    name = "agents_cpp",
    hdrs = glob([
        "include/agents-cpp/**/*.h",
        "include/spdlog/**/*.h"
    ]),
    includes = [
        "include",
    ],
    shared_library = select({
        "@platforms//os:linux": "lib/linux/libagents_cpp_shared_lib.so",
        "@platforms//os:macos": "lib/macos/libagents_cpp_shared_lib.dylib",
        "@platforms//os:windows": "lib/windows/agents_cpp_shared_lib.dll",
        "//conditions:default": None,
    }),
    deps = [
        "@cpp-httplib//:httplib",
        "@nlohmann_json//:json"
    ],
    visibility = ["//visibility:public"]
)

# Export .env file for examples and demos
exports_files([
        ".env",
        "sample_media/ui/index.html",
        "sample_media/ui/in-car.html",
    ],
    visibility = ["__subpackages__"]
)

# Global workspace config — .example files (templates) + skills loaded by all agents
filegroup(
    name = "workspace_config",
    srcs = glob([".agents/**"]),
    visibility = ["//visibility:public"],
)

# End of BUILD file
