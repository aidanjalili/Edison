bool ISLIVE = false;

#include <iostream>
#include "alpaca/alpaca.h"
#include <stdlib.h>
#include "csv.h"
#include <tuple>
#include <cmath>
#include <fstream>
#include <numeric>
#include <filesystem>
#include <assert.h>
#include <random>


#include "stdlib.h"
#include "stdio.h"
#include "string.h"

using namespace std;

int main()
{
    /*INIT*/
    if (ISLIVE)
    {
        setenv("APCA_API_KEY_ID", "AKUL7PSSDDM0UW4BXKH8", 1);
        setenv("APCA_API_SECRET_KEY", "BJSzEiXaZxaMExzV8iWj8bc3akKiSNC3QDr8vP0s", 1);
        setenv("APCA_API_BASE_URL", "api.alpaca.markets", 1);
    }
    else
    {
        setenv("APCA_API_KEY_ID", "PK7GGLHYBZJQPWI9H6OM", 1);
        setenv("APCA_API_SECRET_KEY", "ik09bgz4ruy00W7YvQqaXkk9TTJn9INgJfqSg82O", 1);
    }
    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);
    /*INIT Over*/

    //try to stop algo if it is indeed running
    try
    {
        string pw = "Noodle23!23";
        string input = "echo \"" +  pw + "\"" + " | " +  "sudo -S pkill screen";
        system(input.c_str());
    }
    catch(...)
    {
        //do nothing
    }

    //cancels all open orders...
    auto cancel_orders_response = client.cancelOrders();
    if (auto status = cancel_orders_response.first; !status.ok()) {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        exit(status.getCode());
    }

    //Liquidates everything...
    auto close_positions_response = client.closePositions();
    if (auto status = close_positions_response.first; !status.ok()) {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        exit(status.getCode());
    }


    return 0;
}

