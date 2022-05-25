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

/*Params*/
bool ISLIVE = true;
const string DIRECTORY = "/home/aidanjalili03/Edison-Live/Edison/VSEB";

void Log(string InputFile, string Message)
{
    ofstream OutputFile;
    OutputFile.open(InputFile, ios_base::app);
    OutputFile << Message << "\n";
    OutputFile.close();
}

int LiquidateEverything(alpaca::Client& client, bool CancelFirst)//only internal dependency is LOG func...
{
    if (CancelFirst)
    {
        //cancels all open orders...
        auto cancel_orders_response = client.cancelOrders();
        if (auto status = cancel_orders_response.first; !status.ok()) {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            exit(status.getCode());
        }
    }

    auto get_positions_response = client.getPositions();
    if (auto status = get_positions_response.first; !status.ok())
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return status.getCode();
    }

    for (auto& position : get_positions_response.second)
    {
        if (stoi(position.qty) > 0)
        {
            //Sell...
            auto submit_order_response = client.submitOrder(
                    position.symbol,
                    stoi(position.qty),
                    alpaca::OrderSide::Sell,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            if (auto status = submit_order_response.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                string Message = "***Liquidate Everything Was Called But Could not close position for: " + position.symbol + " !!";
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                continue;
            }

        }
        else
        {
            //Cover...
            auto submit_order_response = client.submitOrder(
                    position.symbol,
                    stoi(position.qty)*-1,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            if (auto status = submit_order_response.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                string Message = "***Could not close position for: " + position.symbol + " !!";
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                continue;
            }
        }
    }

    return 0;

}


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

    if (ISLIVE)
    {
        //try to stop algo in case its still running
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
    }


    //Then liquidate everything (tho cancelling open orders first)
    if (int status = LiquidateEverything(client, true); status!= 0)
    {
        string Message = "LIQUIDATING EVERYTHING SOMEHOW FAILED!";
        Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
        return status;
    }
    else
    {
        string Message = "LIQUIDATING EVERYTHING SUCEEDED (HOPEFULLY, ANYWAY)!";
        Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
        return 0;
    }


}

