"""
Voice agent recipe
"""
cc_binary(
    name = "voice_agent_demo",
    srcs = [
        "voice_agent_demo.cpp",
    ],
    deps = [
        "//:agents_cpp",
        "//:ui"
    ],
    data = [
        "//:.env",
        "//:sample_media/ui/in-car.html",
    ],
    visibility = ["//visibility:public"],
)
