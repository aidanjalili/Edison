#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>

#include "alpaca/alpaca.h"
#include "date.h"
#include "Init.h"


using namespace std;

//TO DO:
/*
 *
 * from 2018-12-31T09:30:00-04:00
 * to 2020-01-03T09:30:00-04:00
 */



//The only data we care about is date, open, close, and volume
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar)
{
    ofstream OutputFile(filename);

    //Write the columns
    OutputFile << "Date, Open, Close, Volume\n";

    //Write the rows
    for(auto iter = InputBar.begin(); iter!=InputBar.end(); iter++)
    {
        OutputFile << date::format("%F", chrono::sys_seconds{chrono::seconds( (*iter).time )} ) << "," << to_string((*iter).open_price) << "," << to_string((*iter).close_price) << "," << to_string((*iter).volume) << "\n";
    }

    OutputFile.close();

}



int main()
{
    //Init starts here
    //Init would simply be run here instead of the following...


    /*Set environment variables*/
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);

    /*Setting the env and client variables for the main func here... this needs to be done in the main func. for some reason*/
    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);

    /*Tickers*/
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; !status.ok())
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return -1;//a -1 return to main means there was an error in getting the ticker list
    }
    auto assets = get_assets_response.second;
    //And ends here

    /*Loops thru tickers here*/ //--> should put into a seperate function later
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {
        auto bars_response = client.getBars({(*iter).symbol}, "2018-12-30T09:30:00-04:00", "2020-01-03T09:30:00-04:00", "", "", "1D", 1000);
        if (auto status = bars_response.first; !status.ok())
        {
            std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
            if (status.getCode() == 429)
            {
                iter--;
                continue;
            }
            else
            {
                //in the future could log here in a txt file which tickers didn't work for whatever reason
                continue;
            }

        }
        auto bars = bars_response.second.bars[(*iter).symbol];

        WriteToCsvOutputs(EffectiveWorkingDirectory+"RawData/"+(*iter).symbol+".csv", bars);

        cout << "******* Wrote " + (*iter).symbol + ".csv ********" << endl;
    }

    return 0;
}