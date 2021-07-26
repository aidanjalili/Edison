/*
 * THIS PROGRAM MUST BE FIRST RUN BEOFRE AUG. 26TH... ONCE IT STARTS ITLL WORK FINE THROUGH THAT DATE
 * BUT IT MUST BE INITIALLY RUN BEFORE THEN
 */

#include <iostream>
#include <string>
#include "alpaca/alpaca.h"
#include <stdlib.h>
#include <chrono>
#include <ctime>


//Variables and structs...
auto env = alpaca::Environment();
auto client = alpaca::Client(env);
struct StockVolumeInformation
{
    string ticker;
    double avg;
    double stdev;
};
string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";


//Forward declare all functions (*note that this is a one file program)



using namespace std;

string TodaysDateAsString()
{
    time_t curr_time;
    tm * curr_tm;
    char date_string[100];

    time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(date_string, 50, "%Y-%m-%d", curr_tm);
    return date_string;
}

int Init()
{
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);


    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: "
                  << status.getMessage()
                  << std::endl;
        return status.getCode();
    }
    else
    {
        return 0;
    }

}


bool FirstRun()
{
    cout << "Is this the first run of this program? (y/n)" << endl;
    string input;
    cin >> input;
    if (input == "y")
        return true;
    else
        return false;
}


//For get Data func
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar)
{
    ofstream OutputFile(filename);

    //Write the columns
    OutputFile << "Date, Open, Close, High, Low, Volume\n";

    //Write the rows
    for(auto iter = InputBar.begin(); iter!=InputBar.end(); iter++)
    {
        OutputFile << date::format("%F", chrono::sys_seconds{chrono::seconds( (*iter).time )} ) << "," << to_string((*iter).open_price) << "," << to_string((*iter).close_price) << "," << to_string((*iter).high_price) << "," << to_string((*iter).low_price) << "," << to_string((*iter).volume) << "\n";
    }

    OutputFile.close();

}


int GetData(string InputDir, string startdate, string enddate)
{

    /*Tickers*/
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return -1;//a -1 return means there was an error in getting the ticker list
    }
    auto assets = get_assets_response.second;
    //And ends here

    /*Loops thru tickers here*/ //--> should put into a seperate function later
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {
        auto bars_response = client.getBars({(*iter).symbol}, startdate, enddate, "", "", "1Day", 10000);

        if (auto status = bars_response.first; status.ok() == false))
        {
            std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
            //Just pray that this doesnt end up in a never ending loop --> originally designed to retry after rate limit has been reached
            iter--;
            continue;
        }


        auto bars = bars_response.second.bars[(*iter).symbol];

        WriteToCsvOutputs(InputDir+"/RawData/"+(*iter).symbol+".csv", bars);

        cout << "******* Wrote " + (*iter).symbol + ".csv ********" << endl;
    }

    return 0;
}

//TO DO:


int main()
{
    //checking for init error
    if (int ret = Init(); ret != 0)
        return ret;

    if (FirstRun() == true)
    {

        //Run getdata and check for fetching ticker error
        sixmonthsago = //do this next
        if (int ret = GetData(DIRECTORY, TodaysDateAsString(), ); ret != 0)
            return ret;


    }


}