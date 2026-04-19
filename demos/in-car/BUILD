"""
In-Car AI Assistant Demo
"""

filegroup(
    name = "in_car_workspace_config",
    srcs = glob([".agents/**"]),
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "in-car-ai",
    srcs = ["in-car-ai.cpp", "ui.hpp"],
    deps = ["//:agents_cpp"],
    data = [
        "//:.env",
        "//:workspace_config",
        ":in_car_workspace_config",
        "//:sample_media/ui/in-car.html",
    ],
)
