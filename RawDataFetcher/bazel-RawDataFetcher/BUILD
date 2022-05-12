cc_library(
    name = "Init",
    srcs = ["Init.cpp"],
    hdrs = ["Init.h"],
    deps = ["@com_github_marpaia_alpaca_trade_api_cpp//alpaca:alpaca"]
)

cc_library(
    name = "Howard-Hinnant-Date-Library",
    hdrs = ['date.h']
)


cc_binary(
    name = "Edison",
    srcs = ["main.cpp"],
    deps = [
        ":Init",
        ":Howard-Hinnant-Date-Library"
    ]
)