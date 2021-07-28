#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstdio>
#include <boost/date_time.hpp>
#include <unistd.h>
#include <sstream>

#include "alpaca/alpaca.h"

using namespace std;


/*Global variables and structs...*/
struct StockVolumeInformation
{
    string ticker;
    double avgvolume;
    double stdevofvolume;
    double todaysvolume;
};

string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";
vector<alpaca::Date> datesmarketisopen;

//These varaibles are "reset" every day...
bool HaveAlreadyRunRefreshToday = false;
bool HaveAlreadyPlacedOrders = false;


//Forward declare all functions (*note that this is a one file program)
int Init();
bool FirstRun();
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar);
int GetData(string InputDir, string startdate, string enddate);
int ExampleLastTradeCode();
int Refresh(string InputDir);
void DeleteRecord(string InputFileNameAndDir, unsigned long RowNumberToBeDeleted);
bool IsTodayATradingDay(vector<alpaca::Date>& datesmarketisopen);
void ResetVariables();

int Init()
{
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);

    auto env = alpaca::Environment();
    auto client = alpaca::Client(env);
    auto get_calendar_response = client.getCalendar("2020-01-01", "2028-12-31");
    if (auto status = get_calendar_response.first; !status.ok()) {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    datesmarketisopen = get_calendar_response.second;
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
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar, bool append)
{
    if (append == true)
    {
        ofstream OutputFile(filename, ios::app);

        //Should just run once but anyway...
        for(auto iter = InputBar.begin(); iter!=InputBar.end(); iter++)
        {
            OutputFile << (*iter).time << "," << to_string((*iter).open_price) << "," << to_string((*iter).close_price) << "," << to_string((*iter).high_price) << "," << to_string((*iter).low_price) << "," << to_string((*iter).volume) << "\n";
        }

        OutputFile.close();
    }
    else
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

        WriteToCsvOutputs(InputDir+"/RawData/"+(*iter).symbol+".csv", bars, false);

        cout << "******* Wrote " + (*iter).symbol + ".csv ********" << endl;
    }

    return 0;
}


int ExampleLastTradeCode()
{
    //These two lines you wouldn't need if the rest of the code is being copy pasted into
    //A function that already has client and env defined
    auto env = alpaca::Environment();
    auto client = alpaca::Client(env);

    auto last_trade_response = client.getLastTrade("AAPL");
    if (auto status = last_trade_response.first; !status.ok()) {
        std::cerr << "Error getting last trade information: " << status.getMessage() << std::endl;
        return status.getCode();
    }

    auto last_trade = last_trade_response.second;
    std::cout << "The last traded price of AAPL was: $" << last_trade.trade.price << std::endl;
    return 0;//this line also goes...
}

//for Refresh func. but can be used elsewhere (only works for csv files up to abt 4 million rows)
void DeleteRecord(string InputFileNameAndDir, unsigned long RowNumberToBeDeleted)//format be (from root for example) /blah/bah2/blah2/myfile
{
    std::ifstream InputFile(InputFileNameAndDir+".csv");

    std::ofstream newFile(InputFileNameAndDir+"new.csv");

    unsigned long index = 0;//hopefully the number of rows in the csv isn't more than 4 million
    string line;
    while(std::getline(InputFile, line))
    {
        if (index == RowNumberToBeDeleted)
        {
            index++;
            continue;
        }

        std::stringstream currentline(line);
        newFile << currentline.str() + "\n";
        index ++;

    }

    // removing the existing file
    remove( (InputFileNameAndDir+".csv").c_str());

    // renaming the new file with the existing file name
    rename( (InputFileNameAndDir+"new.csv").c_str(), (InputFileNameAndDir+".csv").c_str() );
}


//Nightly refresh function
int Refresh(string InputDir)
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

    //Deletes first line and adds latest data
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {
        DeleteRecord(InputDir+"/RawData/"+(*iter).symbol, 1);

        //Then add today's data
        std::ofstream InputFile(InputDir+"/RawData/"+(*iter).symbol+".csv", ios::app);
        boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
        std::string TodayAsString = to_iso_extended_string(Today);

        auto bars_response = client.getBars({(*iter).symbol}, TodayAsString, TodayAsString, "", "", "1Day");
        //NEED TO REMOVE "[]" characters for json IsArray() to work
        if (auto status = bars_response.first; !status.ok())
        {
            cout << "PROBLEM!" << endl;
            //So we delete the problem...
            remove( (InputDir+"/RawData/"+(*iter).symbol+".csv").c_str() );
            continue;
        }

        auto bars = bars_response.second.bars[(*iter).symbol];
        WriteToCsvOutputs(InputDir+"/RawData/"+(*iter).symbol+".csv", bars, true);
    }

}

bool IsTodayATradingDay(vector<alpaca::Date>& datesmarketisopen)
{
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);

    for (const auto& date : datesmarketisopen)
    {
        if (TodaysDateAsString == date.date)
        {
            return true;
        }
    }
    return false;
}

void ResetVariables()
{
    HaveAlreadyRunRefreshToday = false;
    HaveAlreadyPlacedOrders = false;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
    //Run init() func. and check for errors
    if (int ret = Init(); ret != 0)
        return ret;


    if (FirstRun())
    {

        //Get yesterday's date
        boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
        boost::gregorian::days oneday(1);
        auto Yesterday = Today - oneday;
        std::string YesterdayAsString = to_iso_extended_string(Yesterday);

        //Get 6 months before yesterday date
        boost::gregorian::months sixmonths(6);
        boost::gregorian::date SixMonthsBeforeYesterday = Yesterday - sixmonths;
        std::string SixMonthsBeforeYesterdayAsString = to_iso_extended_string(SixMonthsBeforeYesterday);

        //Run getdata and check for fetching ticker error
        if (int ret = GetData(DIRECTORY, SixMonthsBeforeYesterdayAsString+"T09:30:00-04:00", YesterdayAsString+"T16:00:00-04:00"); ret != 0)
            return ret;

    }

    //Seperate what we do at three different times below
    while (true)
    {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

        //If today is a day that the market was/is open
        if (IsTodayATradingDay(datesmarketisopen))
        {
            //if time is 340pm -- need to make sure our Market-On-Close (MOC) order is submited by 350
            if (now.time_of_day().hours() == 15 && now.time_of_day().minutes() == 40)
            {
                ///TO DO's Start here

                //Replace the times I recall api for assets with one call in INIT to a global variable
                //to accomplish above will prolly have to test everything --> good news is raw data has been deleted

                /**
                 * THE FOLLOWING NEEDS TO BE DONE VERY CAREFULLY, AS OTHER THAN COMPILING
                 * NO TESTS WILL BE DONE BEYOND JUST RESETTING API KEYS TO PAPER ACCNT
                 * AS THATS THE ONLY REAL WAY TO TEST IT
                 */
                //Check "currently bought" directory and sell oldest one
                //double check todays date is correct to sell given
                //a "DateToSellGivenDateOfBuy" function
                //(*btw that func can use a for loop that adds days after adding the customary +2 days and checking that that day is a trading day)
                //Sell those orders with a MOC order
                //--> (then place that currently bought file into an "archives" dir.)

                //calculate todays sum volumes and compare to avg+8stdeviations using avg/stdev functions from backtestingVSEB
                //but modifying to use new volumesinfo data struct. --> resetvariables function will also clear that data struct

                HaveAlreadyPlacedOrders = true;
            }

            //If time is 1130 -- run Refresh()
            if (now.time_of_day().hours() == 23 && now.time_of_day().minutes() == 30)
            {
                Refresh(DIRECTORY);//This should take abt 15 mins depending on wifi speed...
            }

            //at midnight reset all variables .. doesn't matter if this is done (up to) 4 times during the min.
            if (now.time_of_day().hours() == 21 && now.time_of_day().minutes() == 59)
            {
                ResetVariables();
            }
        }

        cout << "Alog is now running... Current date/time is: " << to_iso_extended_string(boost::posix_time::second_clock::local_time()) << endl;
        sleep(15);
    }


    return 0;//tho never run ig...
}
#pragma clang diagnostic pop