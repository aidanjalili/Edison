#include <iostream>
#include <string>
#include <stdlib.h>
#include <boost/date_time.hpp>

#include "alpaca/alpaca.h"

using namespace std;
//Variables and structs...


struct StockVolumeInformation
{
    string ticker;
    double avgvolume;
    double stdevofvolume;
    double todaysvolume;
};

string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";


//Forward declare all functions (*note that this is a one file program)
int Init();
bool FirstRun();
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar);
int GetData(string InputDir, string startdate, string enddate);


int Init()
{
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);
    return 0;
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


//For get Data func -- only works with bars that are recieved with api v2
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar)
{
    ofstream OutputFile(filename);

    //Write the columns
    OutputFile << "Date, Open, Close, High, Low, Volume\n";

    //Write the rows
    for(auto iter = InputBar.begin(); iter!=InputBar.end(); iter++)
    {
        OutputFile << (*iter).time << "," << to_string((*iter).open_price) << "," << to_string((*iter).close_price) << "," << to_string((*iter).high_price) << "," << to_string((*iter).low_price) << "," << to_string((*iter).volume) << "\n";
    }

    OutputFile.close();

}


int GetData(string InputDir, string startdate, string enddate)
{
    auto env = alpaca::Environment();
    auto client = alpaca::Client(env);


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

        if (auto status = bars_response.first; status.ok() == false)
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

    if (int ret = Init(); ret != 0)
        return ret;

    auto env = alpaca::Environment();
    auto client = alpaca::Client(env);

    if (FirstRun())
    {

        //Run getdata and check for fetching ticker error

        //Get yesterday's date
        boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
        boost::gregorian::days oneday(1);
        auto Yesterday = Today - oneday;
        std::string YesterdayAsString = to_iso_extended_string(Yesterday);

        //Get 6 months from now date
        boost::gregorian::months sixmonths(6);
        boost::gregorian::date SixMonthsBeforeYesterday = Today - sixmonths;
        std::string SixMonthsBeforeYesterdayAsString = to_iso_extended_string(SixMonthsBeforeYesterday);

        if (int ret = GetData(DIRECTORY, SixMonthsBeforeYesterdayAsString+"T09:30:00-04:00", YesterdayAsString+"T16:00:00-04:00"); ret != 0)
            return ret;

    }

    auto last_trade_response = client.getLastTrade("AAPL");
    if (auto status = last_trade_response.first; !status.ok()) {
        std::cerr << "Error getting last trade information: " << status.getMessage() << std::endl;
        return status.getCode();
    }

    auto last_trade = last_trade_response.second;
    std::cout << "The last traded price of AAPL was: $" << last_trade.trade.price << std::endl;

    /*
     * BUT BEFORE ALL THAT GET GET_LAST_TRADE TO WORK WITH V2
     *
     */


    //while true here --> will have if statement with condition of if market is open today and
    //will run refresh function every night after the market was open during the day at ~11:30
    //to delete oldest day of stock data, and add newest.
    //the first thing to do if market is open today is check the time, then if that's sufficiently late in the day,
    //calculate todays sum volumes and compare to avg+8stdeviations using avg/stdev functions from backtestingVSEB
    //but modifying to use new volumesinfo data struct. --> refresh function will also clear that data struct


}