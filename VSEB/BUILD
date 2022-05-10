config_setting(
    name = "linux_x86_64",
    values = {"cpu": "k8"},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "mac",
    values = {"cpu": "darwin_arm64"},
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Ben-Strasser-Csv-Library",
    srcs = ["csv.h"]
)


cc_binary(
    name="Edison",
    srcs = ["main.cpp"],
    deps = select({
        "//:linux_x86_64": [
            "@python_linux//:python3-lib",
            "@com_github_marpaia_alpaca_trade_api_cpp//alpaca:alpaca",
            "@boost//:date_time",
            "Ben-Strasser-Csv-Library"
        ],
        "//:mac": [
            "@com_github_marpaia_alpaca_trade_api_cpp//alpaca:alpaca",
            "@boost//:date_time",
            "Ben-Strasser-Csv-Library"
        ]
    })
)