#include <iostream>
#include "alpaca/alpaca.h"
#include <stdlib.h>

void SetEnvironmentVariables()
{
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);
}

void Init()
{
    SetEnvironmentVariables();
}

//TO DO:

/*
 * Get bazel run to use correct version of openssl/fix error with arm arch. -- DONE
 *
 * then..
 * learn what auto means
 */

int main()
{
    Init();

    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: "
                  << status.getMessage()
                  << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);

    auto bars_response = client.getBars(
            {"AAPL"},
            "2020-04-01T09:30:00-04:00",
            "2020-04-03T09:30:00-04:00"
    );
    if (auto status = bars_response.first; !status.ok())
    {
        std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto bars = bars_response.second.bars["AAPL"];

    auto start_price = bars.front().open_price;
    auto end_price = bars.back().close_price;
    auto percent_change = (end_price - start_price) / start_price * 100;
    std::cout << "AAPL moved " << percent_change << "% over the time range." << std::endl;
}