"""
OpenClaw-style agent demo — composable workspace prompts, skills, memory, hooks
"""

cc_binary(
    name = "openclaw_demo",
    srcs = ["openclaw_demo.cpp"],
    deps = ["//:agents_cpp"],
    data = [
        "//:.env",
        "//:workspace_config",
    ],
    visibility = ["//visibility:public"],
)
