"""
In-Car AI Assistant Demo
"""

filegroup(
    name = "in_car_workspace_config",
    srcs = glob([".agents/**"]),
)

cc_binary(
    name = "in-car-ai",
    srcs = [
	"in-car-ai.cpp",
	"nav_tools.hpp"
    ],
    deps = [
        "//:agents_cpp",
        "//:ui"
    ],
    data = [
        "//:.env",
        "//:workspace_config",
        ":in_car_workspace_config",
        "//:sample_media/ui/in-car.html",
    ],
    visibility = ["//visibility:public"],
)
