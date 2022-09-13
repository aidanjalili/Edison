/*
 * ALGO DOES THE FOLLOWING....
 * Volume >= averagevolume+ 0.01*stdev && Close > 1.25*yesterdaysclose and days later = 5 and stop loss at 2 percent
 * if earnings call was today, I don't short it NOT ANYMORE, –– THIS NO LONGER APPLYS/WAS COMMENTED OUT
 */

/*
 * next time loop thru files as opposed to assets... this way is kinda stupid ngl... (makes it necessary for a lot of try catches)
 */

/*
 * Current version is unstobable/shouldn't (hopefully) stop for shit
 */

#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstdio>
#include <boost/date_time.hpp>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <numeric>
#include <sys/stat.h>
#include <cstdlib>

#include "Python.h"
#include "csv.h"
#include "alpaca/alpaca.h"



using namespace std;


/*Constants to modify as necessary by the user*/ //(in green is what to do when ur giving dad bak his 1,500)
const string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";//should eventually change to just echoing a pwd command, but for rn on server change to output of pwd
const bool TWENTY_FIVE_K_PROTECTION = true;///simply change this to false before the afternoon/buying time of the day the funds were transfered out of alpaca
const int TWENTY_FIVE_K_PROTECTION_AMOUNT = 25000;//rn it's actually much less than 25k lol
const double LIMIT_AMOUNT = 26000.00;///change this back to 500 (subtravt 1,500 from it)
const string API_PUBLIC_KEY = "AKUL7PSSDDM0UW4BXKH8";
const string API_PRIVATE_KEY = "BJSzEiXaZxaMExzV8iWj8bc3akKiSNC3QDr8vP0s";
const bool IS_LIVE = true;
int LIMIT_TO_INVEST_IN_EACH_TICKER = 50000;

/*Global variables and structs...*/
struct HomeMadeTimeObj
{
    int minutes;
    int hours;
};

struct StockVolumeInformation
{
    string ticker;
    double avgvolume;
    double stdevofvolume;
    unsigned long long todaysvolume;
};

struct buyorder{
    string ticker;
    string buyid;
    string sell_lim_id;
    string lim_price;
};



vector<alpaca::Date> datesmarketisopen;
vector<alpaca::Asset> assets;
vector<string> bannedtickers;
vector<string> LateLimSellsToday;


//These varaibles are "reset" every day...
bool HaveAlreadyRunRefreshToday = false;
bool HaveAlreadyPlacedOrders = false;
bool NoLimitSellsToday = false;
bool HavePlacedLimitOrders = false;
bool SomethingWasCoveredToday = false;//this one is reset right after use
bool NeedToPlaceLimOrders = false;//same for this one
bool HasShitGoneDown = false;
bool TodaysDailyLimSellsPlaced = false;
bool WeveDoneThisOnce = false;//never used outside refresh function and deprecated now...
bool ClearGeneralLog = false;
bool FinalCheckForDay = false;


//Forward declare all functions (*note that this is a one file program) -- could put this into a headerfile but oh well
//Ik bad practice but oh well -- like i said this is a one file program...
int Init(alpaca::Client& client);
bool FirstRun();
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar);
int GetData(string InputDir, string startdate, string enddate, alpaca::Client& client);
int ExampleLastTradeCode(alpaca::Client& client);
void Refresh(string InputDir, alpaca::Client& client);
void DeleteRecord(string InputFileNameAndDir, unsigned long RowNumberToBeDeleted);
bool IsGivenDayATradingDay(string GivenDate, vector<alpaca::Date>& datesmarketisopen);
void ResetVariables();
HomeMadeTimeObj FetchTimeToBuy(vector<alpaca::Date>& datesmarketisopen);
int Sell(alpaca::Client& client);
void ExitWarningSystem();
void Archive(string FileToBeArchived, vector<buyorder>& DataCurrentlyInFile, vector<string>& SellOrdersToAdd);
string DateToSellGivenDateToBuy(string DateofBuy);
double avg(const std::vector<double>& v);
double Stdeviation(const vector<double>& v, double mean);
vector<StockVolumeInformation> FetchTodaysVolumeInfo(alpaca::Client& client);
string NTradingDaysAgo(int marketdaysago);
bool TickerHasGoneUpSinceLastTradingDay(string ticker, alpaca::Client& client);
pair<double, int> CalculateAmntToBeInvested(vector<string>& tickers, alpaca::Client& client);
void RecordBuyOrders(string date, vector<buyorder>& buyorders);
int Buy(int RunNumber, alpaca::Client& client);
void UpdateAssets(alpaca::Client& client);
int PlaceLimSellOrders(alpaca::Client& client, string FILENAME);
void Log(string InputFile, string Message);
bool FilterAssets(alpaca::Asset& asset);
HomeMadeTimeObj FetchTimeToPlaceLimitOrders(vector<alpaca::Date>& datesmarketisopen);
void EmergencyAbort(alpaca::Client& client);
int ChangeUpTheFiles(alpaca::Client& client);
int SellTwo(alpaca::Client& client);
int LiquidateEverything(alpaca::Client& client);
bool IsPathExist(const std::string &s);
void WaitTillTopofTheFifteen();
bool IsTheFirstofaMonth();


void WaitTillTopofTheFifteen()
{
    //retrieve unixtimestamp...
    std::time_t timestamp_old = std::time(0);
    //find where the top of the 15 seconds is...
    std::time_t top_of_the_fifteen = int(timestamp_old/15)*15;
    top_of_the_fifteen+=15;
    //loop until it's right at the next second..
    for (std::time_t timestamp_now = std::time(0); timestamp_now<top_of_the_fifteen; timestamp_now = std::time(0))
    {
        //do nothing!
    }
}

bool IsTheFirstofaMonth()
{
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
    if (TodaysDateAsString.substr(8,2) == "01")
        return true;
    else
        return false;

}

double Stdeviation(const vector<double>& v, double mean)
{
    vector<unsigned long long> squareofdifferencestomean;
    for (int i = 0; i < v.size(); i++)
    {
        squareofdifferencestomean.push_back( (v[i] - mean)*(v[i] - mean) );
    }
    unsigned long long sumofsquares = 0;
    for (auto iter = squareofdifferencestomean.begin(); iter!= squareofdifferencestomean.end(); iter++)
    {
        sumofsquares+=(*iter);
    }
    unsigned long long variance = sumofsquares/squareofdifferencestomean.size();
    return std::sqrt(variance);

}

double avg(const std::vector<double>& v)
{
    unsigned long long sum = 0;
    for (int i = 0; i < v.size(); i++)
    {
        sum+=v[i];
    }
    return sum/v.size();
}

int Init(alpaca::Client& client)
{
    /*banned tickers*/
    bannedtickers.push_back("ABCDE");


    /*Market Dates*/
    auto get_calendar_response = client.getCalendar("2020-01-01", "2028-12-31");
    if (auto status = get_calendar_response.first; !status.ok())
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    datesmarketisopen = get_calendar_response.second;

    UpdateAssets(client);

    return 0;

}

bool FilterAssets(alpaca::Asset& asset)
{
    if (asset.tradable == false || asset.easy_to_borrow == false)
        return true;
    else
    {
        for (int i = 0; i<bannedtickers.size(); i++)
        {
            if (asset.symbol == bannedtickers[i])
                return true;
        }
    }
    return false;
}

void UpdateAssets(alpaca::Client& client)
{
    /*Tickers*/
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        //hope there is never an error getting assets from the api... tho ig if they're is nothing would happen, it j wouldn't update...
    }
    assets = get_assets_response.second;
    //filter assets...
    erase_if(assets, FilterAssets);

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


int GetData(string InputDir, string startdate, string enddate, alpaca::Client& client)
{
    /*Loops thru tickers here*/ //--> should put into a seperate function later
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {
        //this try catch is probably unecessary but ig it doesn't hurt...
        try
        {
            auto bars_response = client.getBars({(*iter).symbol}, startdate, enddate, "", "", "1Day", 10000);
            if (auto status = bars_response.first; status.ok() == false)
            {
                std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
                continue;
            }

            auto bars = bars_response.second.bars[(*iter).symbol];
            WriteToCsvOutputs(InputDir+"/RawData/"+(*iter).symbol+".csv", bars, false);

            cout << "******* Wrote " + (*iter).symbol + ".csv ********" << endl;
        }
        catch(...)
        {
            //some error getting bars for this symbol... so we continue onto the next
            cout << "problem getting historical daily data for: " << (*iter).symbol << endl;
            continue;
        }

    }

    return 0;
}


int ExampleLastTradeCode(alpaca::Client& client)
{
    //These two lines you wouldn't need if the rest of the code is being copy pasted into
    //A function that already has client and env defined

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
    string msg = "removing of: " + InputFileNameAndDir+".csv" + "logged. Differentiater: 1";
    Log(DIRECTORY + "/General_Log.txt", msg);

    // renaming the new file with the existing file name
    rename( (InputFileNameAndDir+"new.csv").c_str(), (InputFileNameAndDir+".csv").c_str() );

    InputFile.close();
    newFile.close();
}


//Nightly refresh function -- might change to type int later to throw a non 0 return if error
void Refresh(string InputDir, alpaca::Client& client)
{
    //Deletes first line and adds latest data
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {
        //the following is the one way to merely check if a file exists in c++... (that I know of)
        //but in general/next time we will j loop thru files as opposed to assets, this way is stupid.

        if (!filesystem::exists(InputDir+"/RawData/"+(*iter).symbol+".csv"))
            continue;

        DeleteRecord(InputDir+"/RawData/"+(*iter).symbol, 1);

        //Then add today's data
        boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
        std::string TodayAsString = to_iso_extended_string(Today);

        auto bars_response = client.getBars({(*iter).symbol}, TodayAsString, TodayAsString, "", "", "1Day");
        if (auto status = bars_response.first; !status.ok())
        {
            cout << "PROBLEM!" << endl;
            //So we delete the problem...
            remove( (InputDir+"/RawData/"+(*iter).symbol+".csv").c_str() );
            string msg = "Removing of: " + InputDir+"/RawData/"+(*iter).symbol+".csv" + " logged. Differntiator 2";
            Log(DIRECTORY + "/General_Log.txt", msg);
            continue;
        }

        auto bars = bars_response.second.bars[(*iter).symbol];
        WriteToCsvOutputs(InputDir+"/RawData/"+(*iter).symbol+".csv", bars, true);

        //TO DO: If any stocks that I currently bought have changed from easy to borrow
        //To hard to borrow... and then do something to like idk cover as soon as possible
    }

}

bool IsGivenDayATradingDay(string GivenDate, vector<alpaca::Date>& datesmarketisopen)
{

    for (const auto& date : datesmarketisopen)
    {
        if (GivenDate == date.date)
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
    NoLimitSellsToday = false;
    HavePlacedLimitOrders = false;
    HasShitGoneDown = false;
    TodaysDailyLimSellsPlaced = false;
    LateLimSellsToday.clear();
    WeveDoneThisOnce = false;
    ClearGeneralLog = false;
    FinalCheckForDay = false;

}

HomeMadeTimeObj FetchTimeToBuy(vector<alpaca::Date>& datesmarketisopen)
{
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);

    HomeMadeTimeObj ret;

    for (auto& date : datesmarketisopen)
    {
        if (date.date == TodaysDateAsString)
        {
            auto closingtime = boost::posix_time::duration_from_string(date.close);
            auto timetobuy = closingtime - boost::posix_time::minutes(9);//just need to make sure its done by close
            string timetobuyasstring = to_simple_string(timetobuy);
            ret.hours = stoi(timetobuyasstring.substr(0,2));
            ret.minutes = stoi(timetobuyasstring.substr(3,2));
            return ret;
        }
    }
}

HomeMadeTimeObj FetchTimeToPlaceLimitOrders(vector<alpaca::Date>& datesmarketisopen)
{

    HomeMadeTimeObj ret;
    ret.hours = 0;
    ret.minutes = 5;
    return ret;//returns 12:05am
}

int Sell(alpaca::Client& client)
{
    vector<string> files;
    for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
    {
        files.push_back(file.path());
    }
    sort(files.begin(), files.end());//Now we only work the oldest one -- so the first one in the vector
    //Double check today is correct DateToSell...
    string DateofBuy = files[0].substr (DIRECTORY.size()+17, 10);//Date of buy in iso format e.g. 1900-12-30
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
    cout << DateofBuy << endl;
    if (DateToSellGivenDateToBuy(DateofBuy) != TodaysDateAsString)
    {
        if (int ret = SellTwo(client); ret!=0)
            return ret;
        else
            return 69;
    }
    //above if statement saves us some work... otherwise we'd do all the following to actually sell some shit for no reason...

    std::ifstream InputFile(files[0]);
    string line, word;
    vector<buyorder> buyorders;
    vector<string> row;
    int index = 0;
    while(std::getline(InputFile, line))
    {
        if (index == 0)
        {
            index++;
            continue;
        }
        row.clear();

        stringstream s(line);

        while (getline(s, word, ','))
        {
            row.push_back(word);
        }

        buyorder currentbuyorder;
        currentbuyorder.ticker = row[0];
        currentbuyorder.buyid = row[1];
        currentbuyorder.sell_lim_id = row[2];
        currentbuyorder.lim_price = row[3];
        buyorders.push_back(currentbuyorder);
    }
    InputFile.close();
    vector<string> sellorderids;

    for (int i = 0; i < buyorders.size(); i++)
    {
        auto get_order_response = client.getOrder(buyorders[i].sell_lim_id);
        if (auto status = get_order_response.first; !status.ok()) {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            return status.getCode();
        }
        auto order = get_order_response.second;

        auto get_buy_order_shit_response = client.getOrder(buyorders[i].buyid);
        auto buy_order = get_buy_order_shit_response.second;//assuming no error fetching api...

        if (buy_order.status == "canceled" || buy_order.status == "rejected")
        {
            sellorderids.push_back("N/A");
            continue;
        }

        if (order.status == "new")//which means it hasn't been filled yet
        {
            //then we'll fill it ourselves...
            client.cancelOrder(order.id);
            sleep(1);

            //need to cast order.qty as int as all(/most) order object members are strings ig...
            stringstream qty(order.qty);
            int orderquantity;
            qty >> orderquantity;

            auto submit_order_response = client.submitOrder(
                    order.symbol,
                    orderquantity,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            sleep(3);
            if (auto status = submit_order_response.first; !status.ok()) {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            sellorderids.push_back(submit_order_response.second.id);

        }
        else
        {
            sellorderids.push_back("N/A");
        }
    }

    Archive(files[0],buyorders,sellorderids);
    SomethingWasCoveredToday = true;
    SomethingWasCoveredToday = false;


    //This will now "sell" all the rest of the shit in currently bought...
    if (int ret = SellTwo(client); ret!=0)
        return ret;

    return 0;
}

int SellTwo(alpaca::Client& client)
{
    vector<string> files;
    for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
    {
        files.push_back(file.path());
    }

    //for every file, put in a MOC sell order...
    for (int i = 0; i < files.size(); i++)//technically could be an iterator again...
    {
        io::CSVReader<4> in( files[i].c_str() );
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");
        string ticker, buyid, sell_lim_id, lim_price;


        while(in.read_row(ticker, buyid, sell_lim_id, lim_price))
        {
            //j cuz im a little ocnfused tbh and not sure if this case would ever happen and if it does it should be treated this way...
            if (sell_lim_id == "NOT_YET_PLACED" || sell_lim_id == "N/A")
                continue;

            //altho the limit sells will just naturally cancel by end of day if they're not executed
            //best to go thru them all and cancel them now
            auto get_order_response_two = client.getOrder(sell_lim_id);
            auto get_order_response_three = client.getOrder(buyid);

            if (auto status = get_order_response_two.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            auto limsell = get_order_response_two.second;
            if (limsell.status == "new")
            {
                if (limsell.status == "expired")
                {
                    cout << "We're fucked!" << endl;
                    exit(-100);
                }

                client.cancelOrder(sell_lim_id);
                sleep(1);
            }
            else//then its already been sold
            {
                continue;
            }

            //now time to sell these shits
            if (auto status = get_order_response_three.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            auto buyorder = get_order_response_three.second;

            if (buyorder.status == "canceled" || buyorder.status == "rejected")//since its never been bought in the first place then...
                continue;

            stringstream thing(buyorder.qty);
            int  buyorderqtyasint = 0;
            thing >> buyorderqtyasint;
            auto submit_order_response = client.submitOrder(
                    buyorder.symbol,
                    buyorderqtyasint,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            sleep(3);
            if (auto status = submit_order_response.first; !status.ok()) {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
        }

    }
    return 0;
}

//This function will FAIL if the two vector inputs are not the same size --> they are supposed to be and should be
void Archive(string FileToBeArchived, vector<buyorder>& DataCurrentlyInFile, vector<string>& SellOrdersToAdd)
{
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
    ofstream OutputFile(DIRECTORY+"/Archives/"+TodaysDateAsString+".csv");
    OutputFile << "ticker,buyid,sell_lim_id,sellid\n";
    for (int i = 0; i < SellOrdersToAdd.size(); i++)
    {
        OutputFile << DataCurrentlyInFile[i].ticker+","+DataCurrentlyInFile[i].buyid+","+DataCurrentlyInFile[i].sell_lim_id+","+SellOrdersToAdd[i]+"\n";
    }
    OutputFile.close();
    remove(FileToBeArchived.c_str());
    string msg = "Removing of: " + FileToBeArchived + " loggged. Differentiator 3";
    Log(DIRECTORY + "/General_Log.txt", msg);
}

string DateToSellGivenDateToBuy(string DateofBuy)
{
    int i = 0;
    for (; i< datesmarketisopen.size(); i++)
    {
        if (datesmarketisopen[i].date == DateofBuy)
        {
            break;
        }
    }
    string ret = datesmarketisopen[i+5].date;
    return ret;
}

void ExitWarningSystem()
{
    Log(DIRECTORY+"/Emergency_Buy_Log.txt", "SYSTEM_HAD_TO_EXIT! THIS HAPPENED AT: " + to_iso_extended_string(boost::posix_time::second_clock::local_time()) + "\n");
}

vector<StockVolumeInformation> FetchTodaysVolumeInfo(alpaca::Client& client)
{
    vector<StockVolumeInformation> StockVolumeInfo;
    for (auto iter = assets.begin(); iter!=assets.end(); iter++)
    {

        vector<double> ThisAssetsVolumes;

        try
        {
            io::CSVReader<6> in( (DIRECTORY+"/RawData/"+(*iter).symbol+".csv").c_str() );
            in.read_header(io::ignore_extra_column, "Date", "Open", "Close", "High", "Low", "Volume");
            //could maybe add a guard here to c if file is too small/not enuf entries like i did in "backtestingVSEB"
            //but also could low key be unecessary

            std::string Date; double Open; double Close; double Low; double High; double Volume;
            while(in.read_row(Date , Open, Close, Low, High, Volume))
            {
                ThisAssetsVolumes.push_back(Volume);
            }

            vector<double> ThisAssetsVolumeToday;

            StockVolumeInformation ThisTickersInfo;
            ThisTickersInfo.ticker = (*iter).symbol;
            double avgvol = avg(ThisAssetsVolumes);
            ThisTickersInfo.avgvolume = avgvol;
            ThisTickersInfo.stdevofvolume = Stdeviation(ThisAssetsVolumes, avgvol);

            boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
            std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);

            auto bars_response = client.getBars({(*iter).symbol}, TodaysDateAsString+"T09:30:00-04:00", TodaysDateAsString+"T16:00:00-04:00", "", "", "1Min", 10000);
            if (auto status = bars_response.first; !status.ok())
            {
                std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
                continue;
            }

            auto bars = bars_response.second.bars[(*iter).symbol];
            if (bars.size() == 0)
                continue;

            unsigned long long todaysvol = 0;
            for (auto iterator = bars.begin(); iterator!=bars.end(); iterator++)
            {
                todaysvol += (*iterator).volume;
            }
            ThisTickersInfo.todaysvolume = todaysvol;
            StockVolumeInfo.push_back(ThisTickersInfo);
            ThisAssetsVolumes.clear();//isn't really needed as it's redefined/declared every loop but whatever

        }
        catch(...)
        {
            //hopefully there is no infinite loop --> this means there was a problem reading this file
            //or the file doesn't exist -- idk it should never happen tbh
            cout << "FetchTodaysVolInfo function couldn't read/do smthg for a specific ticker for some rzn... ticker was: " << (*iter).symbol << endl;
            continue;
        }
    }

    return StockVolumeInfo;
}

string NTradingDaysAgo(int marketdaysago)
{
    int i = 0;
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);

    for (; i < datesmarketisopen.size(); i++)
    {
        if (datesmarketisopen[i].date == TodaysDateAsString)
        {
            break;
        }
    }
    return datesmarketisopen[i-marketdaysago].date;
}

bool TickerHasGoneUpSinceLastTradingDay(string ticker, alpaca::Client& client)//has morphed into a function that just checks specific conditions that r required by this algo... not re-usable generally
{
    string LastTradingDay = NTradingDaysAgo(1);
    string DayBeforeLastTradingDay = NTradingDaysAgo(2);

    auto bars_response = client.getBars(
            {ticker},
            DayBeforeLastTradingDay+"T09:30:00-04:00",
            LastTradingDay+"T16:00:00-04:00",
            "",
            "",
            "1Day",
            10000
    );

    if (auto status = bars_response.first; status.ok() == false)
    {
        std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
        return false;
    }

    auto bars = bars_response.second.bars[ticker];

    if (bars.size() == 0)//rare and weird error to happen as if we can get minute data on a ticker usually u can get daily data as well, but j in case it returns empty
        return false;

    auto closingprice = bars.back().close_price;

    auto last_trade_response = client.getLastTrade(ticker);
    if (auto status = last_trade_response.first; !status.ok()) {
        std::cerr << "Error getting last trade information: " << status.getMessage() << std::endl;
        return false;
    }

    auto last_trade = last_trade_response.second;
    auto currentprice = last_trade.trade.price;

    if (currentprice > 1.25*closingprice)
        return true;
    else
        return false;

}

//returns double which is amnt to be invested and int which is how many tickers should be invested in
//--- first return.second tickers in list should be invested in
pair<double, int>  CalculateAmntToBeInvested(vector<string>& tickers, alpaca::Client& client)
{
    auto resp = client.getAccount();
    auto account = resp.second;
    stringstream cash_in_account(account.cash);
    double cashinsideaccount;
    cash_in_account >> cashinsideaccount;

    double cash;
    if (TWENTY_FIVE_K_PROTECTION == true)
        cash = cashinsideaccount-TWENTY_FIVE_K_PROTECTION_AMOUNT;
    else
        cash = cashinsideaccount;


    ///determine how much money we've received from shorts and set that money aside (subtract it from cash value)....
//    vector <double> moneysrecievedfromshorts;
    //in case smthg was covered today and so is already in archives
//    if (SomethingWasCoveredToday)
//    {
//        vector<string> files;
//        for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/Archives"))
//        {
//            files.push_back(file.path());
//        }
//        sort(files.begin(), files.end());
//        string CurrentCoverFile = files.back();
//        io::CSVReader<4> in(CurrentCoverFile);
//        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "sellid");
//
//        std::string ticker, buyid, sell_lim_id, sellid;
//        vector<double> Costs;
//        while(in.read_row(ticker, buyid, sell_lim_id, sellid))
//        {
//
//            if (sellid != "N/A")
//            {
//                auto get_lim_response = client.getOrder(sell_lim_id);
//                auto lim_order = get_lim_response.second;
//                int qty = stoi(lim_order.qty);
//                double limitprice = stod(lim_order.stop_price);
//                double moneyrecieved = ((ceil( ( ( limitprice / (1.015) ) )*100 ) )/100)*qty;//calculates money recieved from lim_price and qty, always rounds UP to the nearest cent, j to be careful
//                moneysrecievedfromshorts.push_back( moneyrecieved );
//
//            }
//        }
//    }

//
//    //loop thru files in currently bought...
//    vector<string> files;
//    for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
//    {
//        files.push_back(file.path());
//    }
//    sort(files.begin(), files.end());
//    if ( files.back().substr(DIRECTORY.size()+17, 10) == NTradingDaysAgo(1) )
//        files.pop_back();
//    for (auto& dir : files)
//    {
//        io::CSVReader<4> in(dir);
//        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");
//        std::string ticker, buyid, sell_lim_id, lim_price;
//        while(in.read_row(ticker, buyid, sell_lim_id, lim_price))
//        {
//            auto get_lim_order_response = client.getOrder(sell_lim_id);
//            auto lim_order = get_lim_order_response.second;
//            if (lim_order.status == "filled")
//                continue;
//            else
//            {
//                string Date_Associated_With_File = dir.substr(DIRECTORY.size()+17, 10); //returns date hopefully
//                boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
//                std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
//                if (TodaysDateAsString==Date_Associated_With_File)//tho it shouldn't happen, i don't think... edit: or ig it does! who knows, anyway still does the rt thing by conitnuing...
//                {
//                    if (lim_price != "N/A")//if this happens that'd be very, very weird, honestly would do an assert if i was ballsy enuf
//                    {
//                        string Message = "Weird, case here, look in calculate amnt to be invested func..";
//                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
//                        continue;
//                    }
//                    else
//                        continue;
//                }
//
//                auto get_buy_order_response = client.getOrder(buyid);
//                auto buy_order = get_buy_order_response.second;
//
//                double moneyrecieved = ((ceil( ( ( stod(lim_price) / (1.015) ) )*100 ) )/100)*stod(buy_order.filled_qty);//always rounds UP to the nearest cent, j to be careful
//                moneysrecievedfromshorts.push_back( moneyrecieved );
//
//            }
//        }
//
//
//    }
//
//
//    //sum moneyrecieved vector
//    double totalmoneyrecieved = 0;
//    for (auto iter = moneysrecievedfromshorts.begin(); iter!=moneysrecievedfromshorts.end(); iter++)
//    {
//        totalmoneyrecieved+=(*iter);
//    }
//
//    cash -= totalmoneyrecieved;


    ///then divide up the remaining cash evenly by number of tickers...

    //you can change the 1 in the following line to somehting LOWER (obv. only LOWER) if for whatever rzn
    //you want to decrease the amnt of money invested...
    cash = (cash/tickers.size())*1.00;

    /*Old line was:*/
    //cash = cash*( (1-0.98)/ ( tickers.size()*(1.0205-1) ) );
    /*But that simplifies to the same thing as simply coeffecient of ~0.9756 */

    //check to c if there is too much cash -- so we j do 75k in each ticker...
    if (cash/tickers.size() > LIMIT_TO_INVEST_IN_EACH_TICKER)
    {
        pair<double, int> ret;
        ret.first = LIMIT_TO_INVEST_IN_EACH_TICKER;
        ret.second = tickers.size();
        return ret;
    }
    else
    {
        //otherwise we should have enuf j to divide evenly...
        pair<double, int> ret;
        ret.first = cash;
        //ret.first = cash/tickers.size();
        ret.second = tickers.size();
        return ret;
    }


}


bool func(string current_ticker_in_question)
{
    //check if any match the ticker in the csv file
    io::CSVReader<10> in( (DIRECTORY+"/Earnings_Data.csv").c_str() );
    in.read_header(io::ignore_extra_column, "ticker", "companyshortname", "startdatetime", "startdatetimetype", "epsestimate", "epsactual", "epssurprisepct", "timeZoneShortName", "gmtOffsetMilliSeconds", "quoteType");

    std::string ticker, companyshortname, startdatetime, startdatetimetype, epsestimate,epsactual,epssurprisepct,timeZoneShortName,gmtOffsetMilliSeconds,quoteType;
    while(in.read_row(ticker , companyshortname, startdatetime, startdatetimetype, epsestimate,epsactual,epssurprisepct,timeZoneShortName,gmtOffsetMilliSeconds,quoteType))
    {
        if (current_ticker_in_question == ticker)
            return true;
    }
    return false;
}

bool fucker(string& input_ticker)
{
    auto env = alpaca::Environment();
    auto client = alpaca::Client(env);
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return false;//we'll assume its etb then
    }
    else
    {
        auto assets = get_assets_response.second;
        //filter assets to ETB and tradeable
        erase_if(assets, FilterAssets);
        //make sure ticker in question actually exists somewhere in this asset list
        for (int i = 0; i < assets.size(); i++)
        {
            if (assets[i].symbol == input_ticker)
            {
                return false;//so its not HTB so should not be deleted
            }
        }

        //if you make it out of this for loop without ever returning false then return true as this means the stock is HTB
        return true;
    }
}

int Buy(int RunNumber, alpaca::Client& client)
{
    vector<StockVolumeInformation> TodaysVolInformation = FetchTodaysVolumeInfo(client);
    vector<string> TickersToBeBought;

    for (int i = 0; i < TodaysVolInformation.size(); i++)
    {
        if ( (TodaysVolInformation[i].todaysvolume >= TodaysVolInformation[i].avgvolume+0.01*TodaysVolInformation[i].stdevofvolume) && TickerHasGoneUpSinceLastTradingDay(TodaysVolInformation[i].ticker, client) )//essentially in the end stdev fluctuation didn't play a role at all rly
        {
            TickersToBeBought.push_back( TodaysVolInformation[i].ticker );
        }
    }

    //erase all HTB tickers
    //double check that none of these tickers have gone from ETB to HTB since first run when Init() originally got asset list
    auto get_assets = client.getAssets();
    if (auto status = get_assets.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
    }
    else
    {
        auto assets = get_assets.second;
        //filter assets to ETB and tradeable
        erase_if(assets, FilterAssets);
        vector<string> TickersToBeDeleted;
        bool foundone = false;
        for (auto& ticker : TickersToBeBought)
        {
            foundone = false;
            for (auto& asset : assets)
            {
                if (asset.symbol == ticker)
                    foundone = true;
            }

            if (foundone == false)
                TickersToBeDeleted.push_back(ticker);
        }

        for (auto& x : TickersToBeDeleted)
        {
            TickersToBeBought.erase(std::remove(TickersToBeBought.begin(), TickersToBeBought.end(), x), TickersToBeBought.end());
        }
    }


    if (TickersToBeBought.size() == 0)
    {
        NoLimitSellsToday = true;
        return 0;
    }

    /*
     * NOTE TO FUTURE SELF (IF APPLICABLE)
     * THE CODE BELOW DOES NOT WORK, I REPEAT: IT DOES NOT WORK
     * IT WILL CAUSE A SEGFAULT AFTER THE SECOND RUN
     * DELETE THIS COMMENT AIDAN IF YOU STILL GET SEG FAULTS EVEN AFTER COMMENTING OUT THIS
     * (HOPEFULLY PAST AIDAN LOOKED AT THIS COMMENT AND WOULDVE DELETED IT IF THERE WAS STILL A SEG FAULT EVEN AFTER COMMENTING IT OUT)
     */


    //TickersToBeBought is the vector with the tickers to be bought...
//    try
//    {
//        Py_Initialize();
//        PyObject *pName, *pModule, *pFunc, *pArgs, *pValue;
//        PyObject* sysPath = PySys_GetObject("path");
//        auto pystring = PyUnicode_FromString(DIRECTORY.c_str());
//        PyList_Append(sysPath, pystring);
//        pName = PyUnicode_FromString((char*)"earnings_call_downloader");
//        pModule = PyImport_Import(pName);
//        pFunc = PyObject_GetAttrString(pModule, (char*)"main");
//        pArgs = PyTuple_Pack(1, PyUnicode_FromString(DIRECTORY.c_str()));
//        PyObject_CallObject(pFunc, pArgs);
//        Py_Finalize();
//
//        //remove if earnings call was today...
//        /*Never mind, this hurts the algo's preformance as it turns out..*/
//        //erase_if(TickersToBeBought, func);//good stuff too cuz i dont rly even know if this python stuff works tbh
//    }
//    catch (...)
//    {
//        //do nothing
//        cerr << "python script failed" << endl;
//    }

/*
 * END BLOCK OF COMMENTED CODE
 */
    vector<buyorder> BuyOrders;

    for (auto Iterator = TickersToBeBought.begin(); Iterator!=TickersToBeBought.end(); Iterator++)
    {
        buyorder ThisBuyOrder;
        ThisBuyOrder.ticker = (*Iterator);
        ThisBuyOrder.buyid = "NO_QTY";
        ThisBuyOrder.sell_lim_id = "NOT_YET_PLACED";
        ThisBuyOrder.lim_price = "N/A";
        BuyOrders.push_back(ThisBuyOrder);
    }
    boost::gregorian::date DateToday = boost::gregorian::day_clock::local_day();
    std::string DateTodayAsString = to_iso_extended_string(DateToday);
    RecordBuyOrders(DateTodayAsString, BuyOrders);
    //NeedToPlaceLimOrders = true;
    return 0;
}

void RecordBuyOrders(string date, vector<buyorder>& buyorders)
{
    //first, delete the file that already exists there if it does (i think mostly or only for the case of an asset I hold going from ETB to HTB overnight
    bool doesFileExist = false;
    ifstream file;
    file.open(DIRECTORY+"/CurrentlyBought/"+date+".csv");
    if (file)
        doesFileExist = true;
    else
        doesFileExist = false;


    if (doesFileExist == true)
    {
        remove( (DIRECTORY+"/CurrentlyBought/"+date+".csv").c_str() );
        string msg = "Removing of: " + DIRECTORY+"/CurrentyBought/"+date+".csv" + " logged. Diffeentiator 5";
        Log(DIRECTORY + "/General_Log.txt", msg);
    }


    //then proceed with this stuff
    ofstream OutputFile(DIRECTORY+"/CurrentlyBought/"+date+".csv");
    OutputFile << "ticker,buyid,sell_lim_id,lim_price\n";
    for (auto iter = buyorders.begin(); iter!=buyorders.end(); iter++)
    {
        OutputFile << (*iter).ticker << "," << (*iter).buyid << "," << (*iter).sell_lim_id << "," <<(*iter).lim_price << "\n";
    }
    OutputFile.close();

}

void Log(string InputFile, string Message)
{
    ofstream OutputFile;
    OutputFile.open(InputFile, ios_base::app);
    OutputFile << Message << "\n";
    OutputFile.close();
}

int PlaceLimSellOrders(alpaca::Client& client, string FILENAME)
{


    //go thru currently bought, get ticker from their id and price shorted at, then change set double var "price" to that
    //after submitting limit buy order here (after sleep(2)) change the "NOT_YET_PLACED" to the limid
    //also change func. to void


    string newfilename = FILENAME.substr(DIRECTORY.size()+17, 10)+"-new.csv";//returns YYYY-MM-DD-new.csv
    newfilename = DIRECTORY+"/CurrentlyBought/" + newfilename;
    std::ofstream newFile(newfilename);
    newFile << "ticker,buyid,sell_lim_id,lim_price\n";

    io::CSVReader<4> in( (FILENAME).c_str() );
    in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");

    std::string ticker, buyid, sell_lim_id, lim_price;
    while(in.read_row(ticker , buyid, sell_lim_id, lim_price))
    {

        /*impt this guard stays first*/
        if (sell_lim_id != "NOT_YET_PLACED")
        {
            //first things first... make sure this current stop buy in place hasn't been filled already... --> WHICH SHOULD BE THE CASE IF THE ABOVE CONDITION IS TRUE
            auto currentstopbuyorder_order_response = client.getOrder(sell_lim_id);
            //asssuming no error with that again cuz i rly hope/thing there should not be...
            auto currentstopbuyorder = currentstopbuyorder_order_response.second;
            //the following assertion has been commented out for production version...
            assert(currentstopbuyorder.status == "filled");
            //then leave this line alone...
            string newline = ticker + "," + buyid + "," + sell_lim_id + "," + lim_price;
            newFile << newline + "\n";
            continue;
        }



        //make sure this order has been updated already... if the udpated time does not exist or isn't for today wait until it is
        //as this has to happen to continue... after 15 seconds I abort everything as this is Alpaca's fault at this point for not
        //updating my order faster...

        //check every .1 seconds until 15 seconds has passed because at that point it's alpaca's fault
        std::time_t timestamp_now = std::time(0);
        std::time_t timestamp_end = timestamp_now+15;
        for (std::time_t timestamp_current = std::time(0); timestamp_current <= timestamp_end; timestamp_current = std::time(0))
        {
            auto ORDEr = client.getOrder(buyid);
            if (auto status = ORDEr.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                sleep(3);
                continue;
            }
            auto ORDEr_response = ORDEr.second;
            bool WasNotUpdatedToday = false;
            if (ORDEr_response.updated_at == "")
                WasNotUpdatedToday = true;
            else
            {
                string updated_date_and_time = ORDEr_response.updated_at;
                string updated_date = updated_date_and_time.substr(0,10);
                boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
                std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
                if (updated_date!=TodaysDateAsString)
                {
                    WasNotUpdatedToday=true;
                }
            }

            if (WasNotUpdatedToday == false)
                break;

            if (timestamp_current == timestamp_end)//if last iteration
            {
                //if we get to the end check every second for the next 60 seconds, before giving up entirely on this ticker
                std::time_t timestamp_now = std::time(0);
                std::time_t timestamp_end = timestamp_now+60;
                for (std::time_t timestamp_current = std::time(0); timestamp_current <= timestamp_end; timestamp_current = std::time(0))
                {

                    auto ORDEr = client.getOrder(buyid);
                    if (auto status = ORDEr.first; !status.ok())
                    {
                        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                        sleep(3);
                        continue;
                    }
                    auto ORDEr_response = ORDEr.second;
                    bool WasNotUpdatedToday = false;
                    if (ORDEr_response.updated_at == "")
                        WasNotUpdatedToday = true;
                    else
                    {
                        string updated_date_and_time = ORDEr_response.updated_at;
                        string updated_date = updated_date_and_time.substr(0,10);
                        boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
                        std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
                        if (updated_date!=TodaysDateAsString)
                        {
                            WasNotUpdatedToday=true;
                        }
                    }

                    if (WasNotUpdatedToday == false)
                        break;

                    if (timestamp_current == timestamp_end)
                    {
                        //now it's been 75 seconds total and no update :( (fuck u alpaca)
                        //we'll give up on everything
                        string message = "alpaca failed us cuz they r little shits... it\'s been 75 seconds and still no updates on my order... so we are emergency aborting... This was logged at: " +  to_iso_extended_string(boost::posix_time::second_clock::local_time());
                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", message);
                        EmergencyAbort(client);
                    }

                    sleep(1);
                }
            }

            usleep(100000);
        }


        auto get_order_response = client.getOrder(buyid);
        auto order_response = get_order_response.second;

        //makes sure this morning order is actually placed...
        if (order_response.status == "new" || order_response.status == "partially_filled")
        {
            sleep(5);//hopefully this'll give it enuf time to fill...
            get_order_response = client.getOrder(buyid);
            order_response = get_order_response.second;
        }
        //if this hasnt been filled by a minute and a half in... we are summing it never will. Should alert user j in case tho...
        if (order_response.status != "filled" )//could(or maybe should tbh) be else if but whatever
        {
            if ( order_response.status != "rejected" || order_response.status != "canceled")//cuz if its rejected or canceled its not a problem
            {
                try
                {
                    //cancel it if not on its way to being filled
                    if (order_response.status != "partially_filled")//then we just have to cancel
                        client.cancelOrder(order_response.id);
                    else//then we have to cancel and buy back the shares already shorted
                    {
                        client.cancelOrder(order_response.id);
                        usleep(100000);
                        get_order_response = client.getOrder(buyid);
                        order_response = get_order_response.second;

                        string qtyasstring = order_response.filled_qty;

                        //make sure there isn't any fractional shares being bought... (and if so alert the user)
                        int locationofdot = qtyasstring.find(".");
                        if (locationofdot!=-1)//if it was it means thee is no . within the string and so no fractional shares
                        {
                            for (int i = locationofdot+1; i<qtyasstring.size(); i++)
                            {
                                if (qtyasstring.at(i) != '0')
                                {
                                    string msg = "AN order that wasn\'t filled after half a minute was partially filled with fractional shares! So you may have in your portfolio a partial share of a stock with no stop loss as we truncated qty to int... this was logged at: " + to_iso_extended_string(boost::posix_time::second_clock::local_time());
                                    Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                                    break;
                                }

                            }
                        }

                        int qty = stoi(qtyasstring);
                        //commented out this assert for production
                        assert(qty != 0);//assuming the api works the way i think it does... given that status is partially filled and we're checking "filled_qty" this should NOT be 0...
                        //Buy back this many shares of this ticker...
                        auto submit_buy_back = client.submitOrder(
                                order_response.symbol,
                                qty,
                                alpaca::OrderSide::Buy,
                                alpaca::OrderType::Market,
                                alpaca::OrderTimeInForce::Day
                        );
                        if (auto status = submit_buy_back.first; !status.ok()) {
                            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                            return status.getCode();
                        }

                    }
                }
                catch(...)
                {
                    string Message = "An order wasn\'t filled after half a minute but was also not rejected or canceled. We then tried to cancel it or buy back a partially filled qty but FAILED SOMEHOW. This was logged at: " + to_iso_extended_string(boost::posix_time::second_clock::local_time());
                    Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                }
            }
            //this line won't be copied into the new file and so will be deleted from the file subsequently...
            continue;
        }




        //we get last trade price
        auto last_trade_response = client.getLastTrade(order_response.symbol);
        if (auto status = last_trade_response.first; !status.ok())
        {
            std::cerr << "Error getting last trade information: " << status.getMessage() << std::endl;
            int qty = stoi(order_response.qty);
            auto submit_limit_order_response = client.submitOrder(
                    (order_response.symbol),
                    qty,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            usleep(100000);
            if (auto status = submit_limit_order_response.first; !status.ok())
            {
                string Message = "Strange error.. failed to get last trade price so tried to cover but it didn\t cover...";
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
            }
            continue;
            //return status.getCode();
        }

        auto last_trade = last_trade_response.second;
        int qty = stoi(order_response.qty);

        double limitprice;
        if (lim_price != "N/A")
            limitprice = stod(lim_price);
        else
        {
            double price = stod(order_response.filled_avg_price);//price order filled at
            limitprice = price*1.015;
            limitprice = floor(limitprice*100+0.5)/100;
            if (limitprice == price)//in case we r dealing with a real penny stock...
                limitprice+=0.01;
        }


        if (last_trade.trade.price >= limitprice)
        {
            auto submit_limit_order_response = client.submitOrder(
                    (order_response.symbol),
                    qty,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );

            usleep(100000);//
            // wait for lim sell order to be submitted (or in this case j a full blown cover)

            if (auto status = submit_limit_order_response.first; !status.ok())
            {
                std::cerr
                        << "SOMEHOW THE BUY ORDER (SHORT) COULD BE SUBMITED BUT THERE WAS AN ERROR SUBMITTING THE LIM ORDER... API RESPONSE ERROR WAS: "
                        << status.getMessage() << std::endl;
                string Message = "YOUR SCREWED SOMEHOW THERE IS NO COVER METHOD FOR: " + order_response.symbol + " BECAUSE PRICE>LIMPRICE AND DAY BUY DIDN\'T GO THRU... THIS WAS ON: "  +
                                 to_iso_extended_string(boost::posix_time::second_clock::local_time()) +
                                 " Error message of buy order was: " + status.getMessage();
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                continue;
            }

            //so if there was no error putting in the emer cover then we j delete it from our records and move on
            continue;

        }
        else
        {
            /*
             * shitty ~possible~ solution to the rejected lim sell prob... but who knows, maybe it'll work...
             */

            auto submit_limit_order_response = client.submitOrder(
                    (order_response.symbol),
                    qty,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Stop,
                    alpaca::OrderTimeInForce::Day,
                    "",
                    to_string(limitprice)
            );

            usleep(125000);//wait for lim sell order to be submitted

            auto statusthing = submit_limit_order_response.first;

            bool thisorderisbad = false;
            bool wasrejected = false;

            if (!statusthing.ok())
                thisorderisbad = true;

            if (thisorderisbad == false)
            {
                auto get_order_response = client.getOrder(submit_limit_order_response.second.id);
                auto actual_order_response = get_order_response.second;
                if ( (actual_order_response.status != "new" && actual_order_response.status != "filled" && actual_order_response.status != "partially_filled"))
                {
                    if ( actual_order_response.status == "rejected" || actual_order_response.status == "suspended")
                        wasrejected = true;
                    else if (actual_order_response.status == "pending_new" || actual_order_response.status == "accepted" || actual_order_response.status == "accepted_for_bidding")
                    {
                        //wait up to one second for it to get to a new, filled, or partially filled state before canceling and making "wasrejected" true
                        for (int i = 0; i < 100; i++)
                        {
                            auto get_order_response = client.getOrder(submit_limit_order_response.second.id);
                            auto actual_order_response = get_order_response.second;
                            if (actual_order_response.status == "new" || actual_order_response.status == "filled" || actual_order_response.status == "partially_filled")
                                break;
                            if (i == 99)
                            {
                                wasrejected = true;
                            }
                            usleep(10000);
                        }
                    }
                    else//some unknown state tbh
                    {
                        wasrejected = true;
                    }
                }
            }


            if ( ( thisorderisbad ) || ( wasrejected) )
            {

                auto submit_limit_order_response_two = client.submitOrder(
                        (order_response.symbol),
                        qty,
                        alpaca::OrderSide::Buy,
                        alpaca::OrderType::Market,
                        alpaca::OrderTimeInForce::Day
                );
                if (auto status = submit_limit_order_response_two.first; !status.ok())
                {
                    if (submit_limit_order_response_two.second.status == "")//the order doesn't exist in this case..
                    {
                        usleep(300000);//0.3 seconds
                        //try again getting status of limit sell... if still bad then do thing below...
                        auto get_order_response = client.getOrder(submit_limit_order_response.second.id);
                        auto actual_order_response = get_order_response.second;
                        if (actual_order_response.status != "filled" || actual_order_response.status != "partially_filled" || actual_order_response.status != "new")
                        {
                            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                            string msg = "Error calling api during emer buy log process... ";
                            Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                        }

                    }
                    else
                    {
                        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                        string msg = "Error calling api during emer buy log process... ";
                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                    }
                }

                ifstream EmerBuyLog(DIRECTORY+"/Emergency_Buy_Log.txt");
                vector<string> linesinthefile;
                string currentline;
                for (int i = 0; getline(EmerBuyLog, currentline); i++)
                {
                    linesinthefile.push_back(currentline);
                }
                string lastline;
                if (linesinthefile.size() > 0)
                {
                    if (linesinthefile.back() == "" && linesinthefile.size()>1)
                    {
                        lastline = linesinthefile[linesinthefile.size()-2];
                    }
                    else
                        lastline = linesinthefile.back();
                }
                else
                {
                    lastline = "";
                }
                if (lastline == "Error calling api during emer buy log process... ")//emergency buy log was written to with above text...)
                {
                    usleep(100000);
                    string thislimid;
                    auto limit_order_response_two = submit_limit_order_response_two.second;
                    thislimid = limit_order_response_two.id;


                    string newline = order_response.symbol + "," + order_response.id + "," + thislimid + "," + lim_price;
                    newFile << newline + "\n";

                    //check status of that order, and if it was bad, alert user right away!
                    if (limit_order_response_two.status != "filled" && limit_order_response_two.status != "accepted" && limit_order_response_two.status != "pending_new" && limit_order_response_two.status != "")
                    {
                        string Message = "Failure for Emergency Buy Order Placed For: " + order_response.symbol + " on: " +
                                         to_iso_extended_string(boost::posix_time::second_clock::local_time()) +
                                         " status of emergency buy order : " + limit_order_response_two.status;
                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                    }

                    continue;
                }
                continue;
            }

            //so if there was no error putting in the limit sell...
            string thislimid;
            auto limit_order_response = submit_limit_order_response.second;
            thislimid = limit_order_response.id;


            string newline = order_response.symbol + "," + order_response.id + "," + thislimid + "," + to_string(limitprice);
            newFile << newline + "\n";

        }


    }
    // removing the existing file
    remove( (FILENAME).c_str());
    string msg = "Removing of: " +FILENAME + " logged. Differntiator 6";
    Log(DIRECTORY + "/General_Log.txt", msg);

    // renaming the new file with the existing file name
    rename( newfilename.c_str(), FILENAME.c_str() );

    newFile.close();

    //Now we need to double check that there isn't j a blank file, as if there was j one stock listed and it failed to place a lim sell we'll have a file with no real entries in it
    io::CSVReader<4> in_duplicate( FILENAME.c_str());
    in_duplicate.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");
    string ticker_dup, qty_dup, sell_lim_id_dup, lim_price_dup;
    int counter = 0;
    while (in_duplicate.read_row(ticker_dup,qty_dup,sell_lim_id_dup,lim_price_dup))//qty is: NO_QTY as we haven't yet calculated it... tho we will here
    {
        counter++;
    }
    if (counter == 0)
    {
        remove( (FILENAME).c_str());
        string msg = "Remove of: " + FILENAME + " logged. Differentaitor 7";
        Log(DIRECTORY + "/General_Log.txt", msg);
    }

    NeedToPlaceLimOrders = false;//deprecated, however
    return 0;


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


void EmergencyAbort(alpaca::Client& client)
{

    //Liquidates everything... (and cancels all open orders first)

    int status = LiquidateEverything(client, true);
    if (status == 0)
    {
        string Msg = "Emergency Abort Succeded!";
        Log(DIRECTORY + "/Emergency_Buy_Log.txt", Msg);
        exit(42069);
    }
    else
    {
        string msg = "Emergency Abort somehow failed!";
        Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
        exit(status);
    }
}

/*Description of ChangeUpTheFiles Func (four part func. that should prolly honestly be divided into four seperate functions but oh well)
 *
 * This function
 * A)
 *** finds the temp. fake-buy record placed earlier today if it exists //*yesterday's fake-buy record*
 *** and changes it to a real one with a MOO order
 * B)
 *** changes already pre-existing buy records with MOO orders for tmrw
 *** note this doesn't change the actual record-record which will always remain as its original
 *** But y do we do that... y not update to new
 *** ok thats what were doing now lol
 */

int ChangeUpTheFiles(alpaca::Client& client)
{
    /*
     * PART A)
     */
    vector<string> files;
    for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
    {
        files.push_back(file.path());
    }

    if (files.size() == 0)
        return 0;

    sort(files.begin(), files.end());
    //Most recent file in here will be files.back();
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);


    if (files.back().substr(DIRECTORY.size()+17, 10) == NTradingDaysAgo(1))//check for the existence mentioned in A), but actually last trading day date cuz of new update
    {
        io::CSVReader<4> in_forqty( files.back().c_str() );
        in_forqty.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");

        string ticker, qty, sell_lim_id, lim_price;

        //Update records so that they have a qty
        vector<string> TickersToBeBought;
        while (in_forqty.read_row(ticker,qty,sell_lim_id,lim_price))//qty is: NO_QTY as we haven't yet calculated it... tho we will here
        {
            TickersToBeBought.push_back(ticker);
        }

        pair<double, int> Amnt_Invested;
        Amnt_Invested = CalculateAmntToBeInvested(TickersToBeBought, client);
        double AmntToInvest = Amnt_Invested.first;
        vector<buyorder> ListofBuyOrdersForQtys;
        io::CSVReader<4> in_forqty_two( files.back().c_str() );
        in_forqty_two.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");
        while (in_forqty_two.read_row(ticker,qty,sell_lim_id,lim_price))
        {
            int qty;
            auto last_trade_response = client.getLastTrade(ticker);
            auto last_trade = last_trade_response.second;
            auto price = last_trade.trade.price;
            qty = AmntToInvest/price;
            if (qty == 0)
                continue;
            buyorder thisbuyorder;
            thisbuyorder.ticker = ticker;
            thisbuyorder.lim_price = lim_price;
            thisbuyorder.sell_lim_id = sell_lim_id;
            thisbuyorder.buyid = to_string(qty);
            ListofBuyOrdersForQtys.push_back(thisbuyorder);
        }

        if (ListofBuyOrdersForQtys.size() >= 1)//j to ensure there wasn't j one order that had zero qty
        {
            RecordBuyOrders(NTradingDaysAgo(1), ListofBuyOrdersForQtys);

            //start part A here essentially
            io::CSVReader<4> in( files.back().c_str() );
            in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");

            vector<buyorder> ListofBuyOrders;
            while(in.read_row(ticker, qty, sell_lim_id, lim_price))
            {
                //commenting out asset below for production version
                assert(sell_lim_id == "NOT_YET_PLACED");

                stringstream thing(qty);
                int  qtyasint = 0;
                thing >> qtyasint;
                auto submit_order_response = client.submitOrder(
                        ticker,
                        qtyasint,
                        alpaca::OrderSide::Sell,
                        alpaca::OrderType::Market,
                        alpaca::OrderTimeInForce::Day
                );
                if (auto status = submit_order_response.first; !status.ok()) {
                    std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                    continue;
                }
                usleep(100000); //to let the order go thru

                buyorder currentBuyOrder;
                currentBuyOrder.ticker = ticker;
                currentBuyOrder.buyid = submit_order_response.second.id;
                currentBuyOrder.sell_lim_id = "NOT_YET_PLACED";
                currentBuyOrder.lim_price = "N/A";
                ListofBuyOrders.push_back(currentBuyOrder);
            }
            if(ListofBuyOrders.size() > 0)
            {
                RecordBuyOrders(NTradingDaysAgo(1), ListofBuyOrders);
                //since now we've already updated this record, delete file.back();
            }
            else
            {
                remove(files.back().c_str());
                string msg = "removing of " + files.back() + " logged. Differentiator 8";
                Log(DIRECTORY + "/General_Log.txt", msg);
            }
            files.pop_back();

        }
        else
        {
            remove(files.back().c_str());
            string msg = "removing of" + files.back() + " logged. Differentiator 9";
            Log(DIRECTORY + "/General_Log.txt", msg);
            files.pop_back();
        }
    }

    /*
     * PART B)
     */

    for (int i = 0; i < files.size(); i++)//could use an iterator but whatever
    {
        vector<buyorder> ListofBuyOrders;
        string ThisFilesDate = files[i].substr(DIRECTORY.size()+17, 10);

        io::CSVReader<4> in( files[i].c_str() );
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "lim_price");
        string ticker, buyid, sell_lim_id, lim_price;
        while(in.read_row(ticker, buyid, sell_lim_id, lim_price))
        {
            auto get_order_response = client.getOrder(buyid);
            if (auto status = get_order_response.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }

            auto oldbuyorder = get_order_response.second;
            string oldbuyordersticker = oldbuyorder.symbol;
            string oldbuyordersqty = oldbuyorder.qty;
            stringstream thing(oldbuyordersqty);
            int  oldbuyordersqtyasint = 0;
            thing >> oldbuyordersqtyasint;

            /*
             * Some guards...
             */

            //make sure stop buy hasn't already been filled...
            auto get_limit_order_response = client.getOrder(sell_lim_id);
            //assuming theres no error with get_limit_order_response.first
            auto stopbuy = get_limit_order_response.second;
            if (stopbuy.status == "filled" || oldbuyorder.status == "canceled" || oldbuyorder.status == "rejected")
            {
                //shit stays the same...
                buyorder currentBuyOrder;
                currentBuyOrder.ticker = oldbuyordersticker;
                currentBuyOrder.buyid = oldbuyorder.id;
                currentBuyOrder.sell_lim_id = stopbuy.id;
                currentBuyOrder.lim_price = lim_price;
                ListofBuyOrders.push_back(currentBuyOrder);
                continue;
            }
            //else case effectivley below...

            bool isETB = true;
            auto get_assets_response = client.getAssets();
            if (auto status = get_assets_response.first; status.ok() == false)
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                continue;
                //hope there is never an error getting assets from the api... tho ig if they're is nothing would happen, it j wouldn't update...
            }
            else
            {
                auto tempassets = get_assets_response.second;
                erase_if(tempassets, FilterAssets);//so tempassets only contain ETB assets
                for (auto& a : tempassets)
                {
                    if (a.symbol == oldbuyorder.symbol)//this has to happen at some pt for isETB to be true;
                    {
                        isETB = true;
                        break;
                    }
                    isETB = false;
                }

            }
            if (isETB == false)
                continue;

            auto submit_order_response = client.submitOrder(
                    oldbuyordersticker,
                    oldbuyordersqtyasint,
                    alpaca::OrderSide::Sell,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );
            usleep(100000); //to let the order go thru...

            auto newbuyorder = submit_order_response.second;

            //I do this anyway here tho you'll notice that most values are the same as what they were before,
            //so slightly innefficent but idk other option would be to only change the lim sell id back to
            //"NOT_YET_PLACED" -> j using the RecordBuyOrders func. with a buyorder obj. that has only that change
            //isn't that much more innefficent i suppose

            //comment above no longer rly applicable...
            buyorder currentBuyOrder;
            currentBuyOrder.ticker = oldbuyordersticker;
            currentBuyOrder.buyid = newbuyorder.id;
            currentBuyOrder.sell_lim_id = "NOT_YET_PLACED";
            currentBuyOrder.lim_price = lim_price;
            ListofBuyOrders.push_back(currentBuyOrder);


        }
        if (ListofBuyOrders.size() > 0)
            RecordBuyOrders(ThisFilesDate, ListofBuyOrders);
        else
        {
            remove(files[i].c_str());
            string msg = "Removing of: " + files[i] + " logged. Differntiator 10";
            Log(DIRECTORY + "/General_Log.txt", msg);
        }
    }

    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
    atexit(ExitWarningSystem);

    setenv("APCA_API_KEY_ID", API_PUBLIC_KEY.c_str(), 1);
    setenv("APCA_API_SECRET_KEY", API_PRIVATE_KEY.c_str(), 1);

    if (IS_LIVE == true)
        setenv("APCA_API_BASE_URL", "api.alpaca.markets", 1);

    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);

    //Run init() func. and check for errors
    if (int ret = Init(client); ret != 0)
        return ret;


    if (FirstRun())
    {
        //Get yesterday's date
        boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
        std::string YesterdayTestAsString;
        boost::gregorian::date YesterdayTest;
        for (int i = 1; i <= 3; i++)
        {
            boost::gregorian::days idays(i);
            YesterdayTest = Today - idays;
            YesterdayTestAsString = to_iso_extended_string(YesterdayTest);
            if ( IsGivenDayATradingDay(YesterdayTestAsString, datesmarketisopen) )
                break;
            if (i == 3)
                exit(-2);
        }
        auto Yesterday = YesterdayTest;
        string YesterdayAsString = YesterdayTestAsString;

        //Get 6 months before yesterday date
        boost::gregorian::months sixmonths(6);
        boost::gregorian::date SixMonthsBeforeYesterday = Yesterday - sixmonths;
        std::string SixMonthsBeforeYesterdayAsString = to_iso_extended_string(SixMonthsBeforeYesterday);

        //Run getdata and check for fetching ticker error
        if (int ret = GetData(DIRECTORY, SixMonthsBeforeYesterdayAsString+"T09:30:00-04:00", YesterdayAsString+"T16:00:00-04:00", client); ret != 0)
            return ret;

    }

    //wait till the top of the 15 seconds before you start running...

    WaitTillTopofTheFifteen();

    //Seperate what we do at three different times below
    while (true)
    {
        //check to see if we need an emergencyabort()
        auto account_response = client.getAccount();
        if (auto status = account_response.first; !status.ok())
        {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            sleep(1);
            continue;
        }
        auto account = account_response.second;

        if (stod(account.equity) <= LIMIT_AMOUNT)
            EmergencyAbort(client);

        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

        //regardless of everything else... everyday at 1 am we resetvariables
        if (now.time_of_day().hours() == 1 && now.time_of_day().minutes() == 0)//it's ok if this happens (up to) 4 times in the minute
            ResetVariables();

        //If today is a day that the market was/is open
        boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
        std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);


        if (IsGivenDayATradingDay(TodaysDateAsString, datesmarketisopen))
        {
            HomeMadeTimeObj BuyTime = FetchTimeToBuy(datesmarketisopen);
            if (now.time_of_day().hours() == BuyTime.hours && now.time_of_day().minutes() == BuyTime.minutes && HaveAlreadyPlacedOrders == false)
            {
                int NumberofFilesInCurrentlyBought;
                vector<string> files;
                for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
                {
                    files.push_back(file.path());
                }
                NumberofFilesInCurrentlyBought = files.size();

                if (NumberofFilesInCurrentlyBought == 0)
                {
                    if (int buystatus = Buy(1, client); buystatus!=0)
                    {
                        return buystatus;
                    }


                }
                else//greater than 0
                {
                    int sellstatus = Sell(client);
                    if (sellstatus != 0 && sellstatus !=69)
                        return sellstatus;

                    files.clear();
                    for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
                    {
                        files.push_back(file.path());
                    }
                    NumberofFilesInCurrentlyBought = files.size();

                    if (NumberofFilesInCurrentlyBought == 0)
                    {
                        if (int buystatus = Buy(1, client); buystatus!=0)
                        {
                            return buystatus;
                        }

                    }
                    else if (NumberofFilesInCurrentlyBought == 1)
                    {
                        if (int buystatus = Buy(2, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                    else if (NumberofFilesInCurrentlyBought == 2)
                    {
                        if (int buystatus = Buy(3, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                    else if (NumberofFilesInCurrentlyBought == 3)
                    {
                        if (int buystatus = Buy(4, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                    else if (NumberofFilesInCurrentlyBought == 4)
                    {
                        if (int buystatus = Buy(5, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                    else if (NumberofFilesInCurrentlyBought == 5)
                    {
                        if (int buystatus = Buy(0, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                    //else case should never happen

                }

                ///TO DO's Start here

                ///And end here

                HaveAlreadyPlacedOrders = true;
            }


            //If time is 1100pm -- run Refresh()
            if (now.time_of_day().hours() == 23 && now.time_of_day().minutes() == 0 && HaveAlreadyRunRefreshToday==false)
            {
                Refresh(DIRECTORY, client);//This should take abt 15 mins depending on wifi speed...
                HaveAlreadyRunRefreshToday = true;
            }


            if (now.time_of_day().hours() == 9 && now.time_of_day().minutes() == 30 && TodaysDailyLimSellsPlaced == false)
            {

                /*
                 * Lets wait until all the open orders have been filled (in case it takes more than a min to place all of them,
                 * which is very possible -- or not that's a good idea but i'm lazy, we'll j wait till 35 and catch the exception
                 * in placelimsell orders and wait there if need be...
                 */

                /*
                 * THE ABOVE OF WHICH HAS NOW IMPLEMENTED BELOW... :)
                 */


                if (int i = ChangeUpTheFiles(client); i != 0)
                    return (i);

                //sleep until all ahve been placed or a minute has passed...

                //but first wait half a second just to cut down on unecessary api calls cuz like we still need some time for these things to actually place
                usleep(500000);
                for (int i = 0; i < 60; i++)
                {
                    auto resp = client.getOrders(alpaca::ActionStatus::Open);
                    if (auto status = resp.first; !status.ok())
                    {
                        sleep(3);
                        cout << "Error getting order information: " << status.getMessage();
                        if (i == 599)
                            return status.getCode();
                        else
                            continue;
                    }
                    auto orders = resp.second;
                    if (orders.size() == 0)
                        break;



                    sleep(1);
                }

                /*
                 * Places all Lim. sell orders for everything, in whateever order files is read from CurrentlyBought dir
                 * Some other random variable like hasthishappened once or whatever is deprecated now lol
                 */
                vector<string> files;
                for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
                {
                    files.push_back(file.path());
                }

                //somewhat importnat line -- trust me. doesn't "fix" anything per-se, but makes our algo less-risky. KEEP IT IN!!
                sort(files.rbegin(), files.rend());

                for (int i = 0; i < files.size(); i++)//could use iterator here again, but whatever, fuck optimizing memory or runtime amiright or amiright?
                {
                    PlaceLimSellOrders(client, files[i]);//should always return 0, but idrk know at this pt that fact might have to be double checked lol...

                }

                TodaysDailyLimSellsPlaced = true;
            }

            if (TodaysDailyLimSellsPlaced == true && FinalCheckForDay == false )// runs after above command
            {
                //not a perfect check but will hopefully catch any final remaining cases of error...
                sleep(60);

                //first get all our positions and make a list of those tickers...
                auto get_positions_response = client.getPositions();
                auto positions = get_positions_response.second;
                vector<string> tikers_we_have_positions_in;
                int Total_Shares_Positions = 0;
                for (auto& position : positions)
                {
                    tikers_we_have_positions_in.push_back(position.symbol);
                    int qty = stoi(position.qty);
                    if (qty < 0)
                        qty = qty*-1;
                    Total_Shares_Positions+=qty;
                }

                //get a list of all tickers we have open orders for..
                auto list_orders_response = client.getOrders(alpaca::ActionStatus::Open);
                auto open_orders = list_orders_response.second;
                vector<string> openordertickers;
                int Total_Shares_Open_Orders = 0;
                for (auto& currentorder : open_orders)
                {
                    openordertickers.push_back(currentorder.symbol);
                    int qty = stoi(currentorder.qty);
                    if (qty < 0)
                        qty = qty*-1;
                    Total_Shares_Open_Orders+=qty;
                }

                //make sure total number of shares we have open r the same as the total number we have positions in...
                if (Total_Shares_Open_Orders!=Total_Shares_Positions)
                {
                    string msg = "We have a mis-match between the number of shares in open order and the number in positions! This was recorded at: " + to_iso_extended_string(boost::posix_time::second_clock::local_time());
                    Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                    EmergencyAbort(client);
                }

                //first make sure we don't have an open order for something we don't have a position in...
                //i.e. make sure that there is a corresponding ticker in "tickers we have a pos. in" for every ticker in "openordertickers"
                for (auto& x : openordertickers)
                {
                    //make sure every x has a corresponding y in "tickers we have positions in"
                    bool foundone = false;
                    for (auto& y : tikers_we_have_positions_in)
                    {
                        if (x==y)
                        {
                            foundone = true;
                            break;
                        }
                    }
                    if (foundone == false)
                    {
                        string msg = "We have an open order for smthg we don't have a position in! This was recorded at: " + to_iso_extended_string(boost::posix_time::second_clock::local_time());
                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                        EmergencyAbort(client);
                    }
                }

                //now make sure we don't have a position in smthg we don't also have an open order for...

                for (auto& x : tikers_we_have_positions_in)
                {
                    bool foundone = false;
                    for (auto& y : openordertickers)
                    {
                        if (x == y)
                        {
                            foundone = true;
                            break;
                        }
                    }
                    if (foundone == false)
                    {
                        string msg = "We have a positoin in smthg that we don't also have an open order in! This was recorded at: " + to_iso_extended_string(boost::posix_time::second_clock::local_time());
                        Log(DIRECTORY + "/Emergency_Buy_Log.txt", msg);
                        EmergencyAbort(client);
                    }
                }

                FinalCheckForDay = true;
            }

        }

        if (ClearGeneralLog == false && IsTheFirstofaMonth() && now.time_of_day().hours() == 21 && now.time_of_day().minutes() == 0)
        {
            remove( (DIRECTORY + "/General_Log.txt").c_str() );
            ClearGeneralLog = true;
        }
        boost::gregorian::date Sunday = boost::gregorian::from_string("2000/01/09");
        if (  ( TodaysDate.day_of_week() == Sunday.day_of_week() && now.time_of_day().hours() == 9 && now.time_of_day().minutes() == 0 ) && ( IsPathExist(DIRECTORY+"/RawData") ) )//time is sunday at 9:00))
        {
            //remove it (and all of its conetents naturally)
            std::filesystem::remove_all(DIRECTORY+"/RawData");
            //make it again
            std::filesystem::create_directory(DIRECTORY+"/RawData");
            //update assets var
            UpdateAssets(client);
            //run fetch data again with last trading day as ending day and 6 months prior to that as starting day
            //Get yesterday's date
            boost::gregorian::date Today = boost::gregorian::day_clock::local_day();
            std::string YesterdayTestAsString;
            boost::gregorian::date YesterdayTest;
            for (int i = 1; i <= 3; i++)
            {
                boost::gregorian::days idays(i);
                YesterdayTest = Today - idays;
                YesterdayTestAsString = to_iso_extended_string(YesterdayTest);
                if ( IsGivenDayATradingDay(YesterdayTestAsString, datesmarketisopen) )
                    break;
                if (i == 3)
                    exit(-2);
            }
            auto Yesterday = YesterdayTest;
            string YesterdayAsString = YesterdayTestAsString;

            //Get 6 months before yesterday date
            boost::gregorian::months sixmonths(6);
            boost::gregorian::date SixMonthsBeforeYesterday = Yesterday - sixmonths;
            std::string SixMonthsBeforeYesterdayAsString = to_iso_extended_string(SixMonthsBeforeYesterday);

            //Run getdata and check for fetching ticker error
            if (int ret = GetData(DIRECTORY, SixMonthsBeforeYesterdayAsString+"T09:30:00-04:00", YesterdayAsString+"T16:00:00-04:00", client); ret != 0)
                return ret;


        }

        cout << "Algo is now running... Current date/time is: " << to_iso_extended_string(boost::posix_time::second_clock::local_time()) << endl;
        WaitTillTopofTheFifteen();
    }


    return 0;//tho never run ig...
}

bool IsPathExist(const std::string &s)
{
    struct stat buffer;
    return (stat (s.c_str(), &buffer) == 0);
}
#pragma clang diagnostic pop
