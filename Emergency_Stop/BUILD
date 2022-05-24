cc_library(
    name = "Ben-Strasser-Csv-Library",
    srcs = ["csv.h"]
)

cc_binary(
    name = "Emergency_Stop",
    srcs = ["main.cpp"],
    deps = ["@com_github_marpaia_alpaca_trade_api_cpp//alpaca:alpaca",
            ":Ben-Strasser-Csv-Library"]
)