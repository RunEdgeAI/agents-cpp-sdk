# Main library build recipe

# Export .env file for examples and demos
exports_files([
        ".env",
        "sample_media/ui/in-car.html"
    ],
    visibility = ["__subpackages__"]
)

# Global workspace config — .example files (templates) + skills loaded by all agents
filegroup(
    name = "workspace_config",
    srcs = glob([".agents/**"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ui",
    hdrs = ["sample_media/ui/ui.hpp"],
    includes = ["sample_media/ui"],
    deps = ["//:agents_cpp"],
    visibility = ["//visibility:public"]
)

cc_import(
    name = "agents_cpp",
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = [
        "include",
    ],
    shared_library = select({
        "@bazel_tools//src/conditions:linux_x86_64": "lib/linux/x64/libagents_cpp_shared_lib.so",
        "@bazel_tools//src/conditions:linux_aarch64": "lib/linux/aarch64/libagents_cpp_shared_lib.so",
        "@platforms//os:macos": "lib/macos/libagents_cpp_shared_lib.dylib",
        "@platforms//os:windows": "lib/windows/agents_cpp_shared_lib.dll",
        "//conditions:default": None,
    }),
    deps = [
        "@nlohmann_json//:json"
    ],
    visibility = ["//visibility:public"]
)

# End of BUILD file
