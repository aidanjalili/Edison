/*
 * ALGO DOES THE FOLLOWING....
 * Volume >= averagevolume+ 3*stdev && Close > 1.2*yesterdaysclose && Open <= 20 and days later = 5 and stop loss at 1 percent
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

#include "csv.h"
#include "alpaca/alpaca.h"

using namespace std;


/*Constants to modify as necessary by the user*/ //(in green is what to do when ur giving dad bak his 1,500)
const string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";//should eventually change to just echoing a pwd command
const bool TWENTY_FIVE_K_PROTECTION = true;///simply change this to false before the afternoon/buying time of the day the funds were transfered out of alpaca
const int TWENTY_FIVE_K_PROTECTION_AMOUNT = 1500+25000;//rn it's actually much less than 25k lol
const double LIMIT_AMOUNT = 27000.00;///change this back to 500 (subtravt 1,500 from it)
const string API_PUBLIC_KEY = "AKUL7PSSDDM0UW4BXKH8";
const string API_PRIVATE_KEY = "BJSzEiXaZxaMExzV8iWj8bc3akKiSNC3QDr8vP0s";
const bool IS_LIVE = true;

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
};



vector<alpaca::Date> datesmarketisopen;
vector<alpaca::Asset> assets;
char* arr[] = {"ABCDEONETWOTHREE"};//{"AFINO", "SWAGU", "AGNCO", "MTAL.U", "AJAX.U", "IMAQU", "RVACU"};
vector<string> bannedtickers(arr, arr + sizeof(arr)/sizeof(arr[0]));
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
pair<double, int> CalculateAmntToBeInvested(vector<string>& tickers, int RunNumber, alpaca::Client& client);
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
        for (auto iterator = bannedtickers.begin(); iterator!=bannedtickers.end(); iterator++)
        {
            if (asset.symbol == (*iterator))
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
            auto timetobuy = closingtime - boost::posix_time::minutes(25);//subtract 25 mins to allow ample time to place orders before 3:50
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
    sort(files.begin(), files.end());//Now we only work the most recent one -- so the first one in the vector
    //Double check today is correct DateToSell...
    string DateofBuy = files[0].substr (DIRECTORY.size()+17, 10);//Date of buy in iso format e.g. 1900-12-30
    boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
    std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);
    cout << DateofBuy << endl;
    if (DateToSellGivenDateToBuy(DateofBuy) != TodaysDateAsString)
    {
        SellTwo(client);
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
        buyorders.push_back(currentbuyorder);
    }
    InputFile.close();
    vector<string> sellorderids;

    for (int i = 0; i < buyorders.size(); i++)
    {
        sleep(1);
        auto get_order_response = client.getOrder(buyorders[i].sell_lim_id);
        if (auto status = get_order_response.first; !status.ok()) {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            return status.getCode();
        }
        auto order = get_order_response.second;
        if (order.status == "new")//which means it hasn't been filled yet
        {
            //then we'll fill it ourselves...
            client.cancelOrder(order.id);
            sleep(2);

            //need to cast order.qty as int as all(/most) order object members are strings ig...
            stringstream qty(order.qty);
            int orderquantity;
            qty >> orderquantity;

            auto submit_order_response = client.submitOrder(
                    order.symbol,
                    orderquantity,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::CLS
            );
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
        io::CSVReader<3> in( files.back().c_str() );
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");
        string ticker, buyid, sell_lim_id;


        while(in.read_row(ticker, buyid, sell_lim_id))
        {
            //altho the limit sells will just naturally cancel by end of day if they're not executed
            //best to go thru them all and cancel them now
            auto get_order_response_two = client.getOrder(sell_lim_id);
            if (auto status = get_order_response_two.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            auto limsell = get_order_response_two.second;
            if (limsell.status == "new")
            {
                client.cancelOrder(sell_lim_id);
                sleep(1);
            }
            else//then its already been sold
            {
                continue;
            }

            //now time to sell these shits
            auto get_order_response_three = client.getOrder(buyid);
            if (auto status = get_order_response_three.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            auto buyorder = get_order_response_three.second;
            stringstream thing(buyorder.qty);
            int  buyorderqtyasint = 0;
            thing >> buyorderqtyasint;
            auto submit_order_response = client.submitOrder(
                    buyorder.symbol,
                    buyorderqtyasint,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::CLS
            );
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

    if (currentprice > 1.2*closingprice && currentprice <= 20)
        return true;
    else
        return false;

}

//returns double which is amnt to be invested and int which is how many tickers should be invested in
//--- first return.second tickers in list should be invested in
pair<double, int> CalculateAmntToBeInvested(vector<string>& tickers, int RunNumber, alpaca::Client& client)
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

    //If we just placed some covers today we want to change the cash amnt to roughly what it will be
    //after I pay to cover...
    if (SomethingWasCoveredToday)//this if statement is ineffecient at doing what it's supposed to do but oh well
    {
        vector<string> files;
        for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/Archives"))
        {
            files.push_back(file.path());
        }
        sort(files.begin(), files.end());
        string CurrentCoverFile = files.back();
        io::CSVReader<4> in(CurrentCoverFile);
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id", "sellid");

        std::string ticker, buyid, sell_lim_id, sellid;
        vector<double> Costs;
        while(in.read_row(ticker, buyid, sell_lim_id, sellid))
        {
            //trying to find out abt how much this cover day will cost me...
            if (sellid != "N/A")
            {
                auto get_sell_response = client.getOrder(sellid);
                auto sell_order = get_sell_response.second;
                int qty = stoi(sell_order.qty);
                string symbol = sell_order.symbol;
                //should prolly assert that this equals ticker but whatever...

                //now get the last trade price of this symbol...
                auto last_trade_response = client.getLastTrade(symbol);
                auto last_trade = last_trade_response.second;
                auto price = last_trade.trade.price;
                //increase by 7.5% in case it does do that in the last few mins of trading here...
                price = 1.075*price;

                //calculate money needed to cover...
                double MoneyNeededToCover = qty*price;
                Costs.push_back(MoneyNeededToCover);

            }
        }
        double Sum = 0;
        for (auto iter = Costs.begin(); iter != Costs.end(); iter++)
            Sum+=(*iter);
        cash-=Sum;
        SomethingWasCoveredToday = false;
    }

    /* FOR SHORTING STARTS HERE */
    //We can ignore runnumber (except for 1st) when shorting...

    double EmergencyTrigger = 1.00; //the extra .25 cuz im nerotic and rly wanna make sure i'll always have enuf cash //update 2: changed it to 1.00 since we r only trading within the day...
    //EmergencyTrigger-=0.03;//cuz we account for that in the 1% stop loss
    if (RunNumber == 1)
    {
        cash = (cash)/(5*1.015+EmergencyTrigger-5);//this will actually slightly overdo it as we'll prolly invest less as share prices don't divide evenly
        //Note i changed 1.01 to 1.015 to accnt for a possible increase in an astounding 50% of the asset before we short
        //so that even in that extreme case we'd be able to cover the extra 1/2% of losses

    }
    else
    {
        //loop thru files in currently bought...
        vector<string> files;
        for (const auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
        {
            files.push_back(file.path());
        }
        vector <double> moneysrecievedfromshorts;
        for (auto& dir : files)
        {
            io::CSVReader<3> in(dir);
            in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");
            //could maybe add a guard here to c if file is too small/not enuf entries like i did in "backtestingVSEB"
            //but also could low key be unecessary

            std::string ticker, buyid, sell_lim_id;
            while(in.read_row(ticker, buyid, sell_lim_id))
            {
                auto get_lim_order_response = client.getOrder(sell_lim_id);
                auto lim_order = get_lim_order_response.second;
                if (lim_order.status == "filled")
                    continue;
                else
                {
                    auto get_buy_order_response = client.getOrder(buyid);
                    auto buy_order = get_buy_order_response.second;
                    double moneyrecieved = stod(buy_order.filled_avg_price)*stod(buy_order.filled_qty);
                    moneysrecievedfromshorts.push_back( moneyrecieved );
                }

            }
        }

        //sum moneyrecieved vector
        double totalmoneyrecieved = 0;
        for (auto iter = moneysrecievedfromshorts.begin(); iter!=moneysrecievedfromshorts.end(); iter++)
        {
            totalmoneyrecieved+=(*iter);
        }

        cash = cash - totalmoneyrecieved;
        cash = (cash)/(5*1.015+EmergencyTrigger-5);


    }


    /* AND ENDS HERE */


    /*FOR BUYING STARTS HERE*/
//    if (RunNumber != 0)
//    {
//        if (RunNumber == 1)
//            cash = cash/5;
//        else if (RunNumber == 2)
//            cash = cash/4;
//        else if (RunNumber == 3)
//            cash = cash/3;
//        else if (RunNumber == 4)
//            cash = cash/2;
//        else
//            cash = cash;//do nothing...
//    }

    /*AND ENDS HERE */

    //Check to c if there is too little cash to split evenly amongst all tickers...
    if (cash / tickers.size() < 20 )
    {
        int NumberOfTickers;
        NumberOfTickers = cash/25;
        double amnttobeinvested = cash/NumberOfTickers;

        pair<double, int> ret;
        ret.first = amnttobeinvested;
        ret.second = NumberOfTickers;
        return ret;
    }
        //check to c if there is too much cash -- so we j do 75k in each ticker...
    else if (cash/tickers.size() > 75000)
    {
        pair<double, int> ret;
        ret.first = 75000;
        ret.second = tickers.size();
        return ret;
    }
    else
    {
        //otherwise we should have enuf j to divide evenly...
        pair<double, int> ret;
        ret.first = cash/tickers.size();
        ret.second = tickers.size();
        return ret;
    }


}

int Buy(int RunNumber, alpaca::Client& client)
{
    vector<StockVolumeInformation> TodaysVolInformation = FetchTodaysVolumeInfo(client);
    vector<string> TickersToBeBought;
    for (auto iter = TodaysVolInformation.begin(); iter!=TodaysVolInformation.end(); iter++)
    {
        //to check if price is less than or equal to 20...
        if ( ((*iter).todaysvolume >= (*iter).avgvolume+3*(*iter).stdevofvolume) && TickerHasGoneUpSinceLastTradingDay((*iter).ticker, client) )
        {
            TickersToBeBought.push_back( (*iter).ticker );
        }
    }

    if (TickersToBeBought.size() == 0)
    {
        NoLimitSellsToday = true;
        return 0;
    }

    //double check that none of these tickers have gone from ETB to HTB since first run when Init() originally got asset list
    //for now, don't question making the new variavle tempassets to do this, for reasons it is necessary...
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        //hope there is never an error getting assets from the api... tho ig if they're is nothing would happen, it j wouldn't update...
    }
    else
    {
        auto tempassets = get_assets_response.second;
        bool foundone = false;
        erase_if(tempassets, FilterAssets);//filters them down to ETB assets
        vector<int> indexestobedeleted;
        for (int i = 0; i<TickersToBeBought.size(); i++)
        {
            for (auto& x : tempassets)
            {
                //should always find the matching symbol in the tempassets list
                if (TickersToBeBought[i] == x.symbol)
                {
                    foundone = true;
                    break;
                }
            }

            //if that didn't happen then the stock for some reason is not in the list and should be removed from the to_be_bought list
            if (foundone == false)
            {
                indexestobedeleted.push_back(i);
            }

            //reset foundone
            foundone = false;
        }

        if (indexestobedeleted.size() != 0)
        {
            for (int& i : indexestobedeleted)
                TickersToBeBought.erase(TickersToBeBought.begin() + i);
        }
    }

    pair<double, int> Amnt_Invested;
    Amnt_Invested = CalculateAmntToBeInvested(TickersToBeBought, RunNumber, client);
    vector<string> temp_tickers_to_be_bought; //maybe could use some "slice" func. here instead idk -- but the vector shouldn't be that big anyway (tho technically is kind of slow)
    for (int i = 0; i<Amnt_Invested.second; i++)
    {
        temp_tickers_to_be_bought.push_back(TickersToBeBought[i]);
    }
    //copy temp to actual...
    TickersToBeBought.clear();
    TickersToBeBought = temp_tickers_to_be_bought;
    double AmntToInvest = Amnt_Invested.first;

    vector<buyorder> BuyOrders;

    for (auto Iterator = TickersToBeBought.begin(); Iterator!=TickersToBeBought.end(); Iterator++)
    {

        int qty;
        auto last_trade_response = client.getLastTrade((*Iterator));
        auto last_trade = last_trade_response.second;
        auto price = last_trade.trade.price;
        qty = AmntToInvest/price;

        qty-=1;//in case price goes up by 1 share price
        if (qty == 0)//should never happn but alwyas good to check
        {
            cout << "qty was 0 for some rzn" << endl;
            continue;
        }

        string thislimid = "NOT_YET_PLACED";//"N/A" for now as it will be placed later with REFRESH function...

        buyorder ThisBuyOrder;
        ThisBuyOrder.ticker = (*Iterator);
        ThisBuyOrder.buyid = to_string(qty);
        ThisBuyOrder.sell_lim_id = thislimid;
        BuyOrders.push_back(ThisBuyOrder);
    }
    boost::gregorian::date DateToday = boost::gregorian::day_clock::local_day();
    std::string DateTodayAsString = to_iso_extended_string(DateToday);
    RecordBuyOrders(DateTodayAsString, BuyOrders);
    NeedToPlaceLimOrders = true;
    return 0;
}

void RecordBuyOrders(string date, vector<buyorder>& buyorders)
{
    ofstream OutputFile(DIRECTORY+"/CurrentlyBought/"+date+".csv");
    OutputFile << "ticker,buyid,sell_lim_id\n";
    for (auto iter = buyorders.begin(); iter!=buyorders.end(); iter++)
    {
        OutputFile << (*iter).ticker << "," << (*iter).buyid << "," << (*iter).sell_lim_id << "\n";
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
    newFile << "ticker,buyid,sell_lim_id\n";

    io::CSVReader<3> in( (FILENAME).c_str() );
    in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");

    std::string ticker, buyid, sell_lim_id;
    while(in.read_row(ticker , buyid, sell_lim_id))
    {

        auto get_order_response = client.getOrder(buyid);
        auto order_response = get_order_response.second;

        //makes sure this morning order is actually placed...
        if (order_response.status == "new" || order_response.status == "partially_filled")
        {
            sleep(45);//hopefully this'll give it enuf time to fill...
        }

        //we get last trade price
        auto last_trade_response = client.getLastTrade(order_response.symbol);
        if (auto status = last_trade_response.first; !status.ok())
        {
            std::cerr << "Error getting last trade information: " << status.getMessage() << std::endl;
            return status.getCode();
        }

        auto last_trade = last_trade_response.second;
        auto priceofstonk = last_trade.trade.price;
        double price = stod(order_response.filled_avg_price);//price order filled at
        double limitprice = price*1.01;
        int qty = stoi(order_response.qty);

        if (last_trade.trade.price >= limitprice)
        {
            auto submit_limit_order_response = client.submitOrder(
                    (order_response.symbol),
                    qty,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::Day
            );

            sleep(2);//wait for lim sell order to be submitted

            if (auto status = submit_limit_order_response.first; !status.ok())
            {
                std::cerr
                        << "SOMEHOW THE BUY ORDER COULD BE SUBMITED BUT THERE WAS AN ERROR SUBMITTING THE LIM ORDER... API RESPONSE ERROR WAS: "
                        << status.getMessage() << std::endl;
                string Message = "YOUR SCREWED SOMEHOW THERE IS NO COVER METHOD FOR: " + order_response.symbol + " BECAUSE PRICE>LIMPRICE AND DAY BUY DIDN\'T GO THRU... THIS WAS ON: "  +
                                 to_iso_extended_string(boost::posix_time::second_clock::local_time()) +
                                 " Error message of buy order was: " + status.getMessage();
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);
                continue;
            }

            //so if there was no error putting in the limit sell...
            string thislimid;
            auto limit_order_response = submit_limit_order_response.second;
            thislimid = limit_order_response.id;


            string newline = order_response.symbol + "," + order_response.id + "," + thislimid;
            newFile << newline + "\n";

        }
        else
        {

            auto submit_limit_order_response = client.submitOrder(
                    (order_response.symbol),
                    qty,
                    alpaca::OrderSide::Buy,
                    alpaca::OrderType::Stop,
                    alpaca::OrderTimeInForce::Day,
                    "",
                    to_string(limitprice)
            );

            sleep(2);//wait for lim sell order to be submitted

            if (auto status = submit_limit_order_response.first; !status.ok())
            {
                std::cerr
                        << "SOMEHOW THE BUY ORDER COULD BE SUBMITED BUT THERE WAS AN ERROR SUBMITTING THE LIM ORDER... API RESPONSE ERROR WAS: "
                        << status.getMessage() << std::endl;
                string Message = "Emergency Buy Order Placed for: " + order_response.symbol + " on: " +
                                 to_iso_extended_string(boost::posix_time::second_clock::local_time()) +
                                 " Error message of lim sell was: " + status.getMessage();
                Log(DIRECTORY + "/Emergency_Buy_Log.txt", Message);

                auto submit_limit_order_response_two = client.submitOrder(
                        (order_response.symbol),
                        qty,
                        alpaca::OrderSide::Buy,
                        alpaca::OrderType::Market,
                        alpaca::OrderTimeInForce::Day
                );

                string thislimid;
                auto limit_order_response = submit_limit_order_response_two.second;
                thislimid = limit_order_response.id;


                string newline = order_response.symbol + "," + order_response.id + "," + thislimid;
                newFile << newline + "\n";

                sleep(2);//wait for order to be put in...
                continue;
            }

            //so if there was no error putting in the limit sell...
            string thislimid;
            auto limit_order_response = submit_limit_order_response.second;
            thislimid = limit_order_response.id;


            string newline = order_response.symbol + "," + order_response.id + "," + thislimid;
            newFile << newline + "\n";

        }


    }
    // removing the existing file
    remove( (FILENAME).c_str());

    // renaming the new file with the existing file name
    rename( newfilename.c_str(), FILENAME.c_str() );

    newFile.close();
    NeedToPlaceLimOrders = false;//deprecated, however
    return 0;


}


void EmergencyAbort(alpaca::Client& client)
{
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

    exit(42069);
}

/*Description of ChangeUpTheFiles Func (four part func. that should prolly honestly be divided into four seperate functions but oh well)
 *
 * This function
 * A)
 *** finds the temp. fake-buy record placed earlier today if it exists
 *** and changes it to a real one with a MOO order
 * B)
 *** changes already pre-existing buy records with MOO orders for tmrw
 *** note this doesn't change the actual record-record which will always remain as its original
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

    if (files.back().substr(DIRECTORY.size()+17, 10) == TodaysDateAsString)//check for the existence mentioned in A)
    {
        io::CSVReader<3> in( files.back().c_str() );
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");

        string ticker, qty, sell_lim_id;
        vector<buyorder> ListofBuyOrders;
        while(in.read_row(ticker, qty, sell_lim_id))
        {
            assert(sell_lim_id == "NOT_YET_PLACED");

            stringstream thing(qty);
            int  qtyasint = 0;
            thing >> qtyasint;
            auto submit_order_response = client.submitOrder(
                    ticker,
                    qtyasint,
                    alpaca::OrderSide::Sell,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::OPG
            );
            if (auto status = submit_order_response.first; !status.ok()) {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }
            sleep(2); //to let the order go thru

            buyorder currentBuyOrder;
            currentBuyOrder.ticker = ticker;
            currentBuyOrder.buyid = submit_order_response.second.id;
            currentBuyOrder.sell_lim_id = "NOT_YET_PLACED";
            ListofBuyOrders.push_back(currentBuyOrder);
        }

        RecordBuyOrders(TodaysDateAsString, ListofBuyOrders);

        //since now we've already updated this record, delete file.back();
        files.erase(files.end() - 1);//or could use files.pop_back();

    }

    /*
     * PART B)
     */

    for (int i = 0; i < files.size(); i++)//could use an iterator but whatever
    {
        vector<buyorder> ListofBuyOrders;
        string ThisFilesDate = files[i].substr(DIRECTORY.size()+17, 10);

        io::CSVReader<3> in( files.back().c_str() );
        in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");
        string ticker, buyid, sell_lim_id;
        while(in.read_row(ticker, buyid, sell_lim_id))
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

            auto submit_order_response = client.submitOrder(
                    oldbuyordersticker,
                    oldbuyordersqtyasint,
                    alpaca::OrderSide::Sell,
                    alpaca::OrderType::Market,
                    alpaca::OrderTimeInForce::OPG
            );
            sleep(2); //to let the order go thru...
            if (auto status = submit_order_response.first; !status.ok())
            {
                std::cerr << "Error calling API: " << status.getMessage() << std::endl;
                return status.getCode();
            }

            //I do this anyway here tho you'll notice that most values are the same as what they were before,
            //so slightly innefficent but idk other option would be to only change the lim sell id back to
            //"NOT_YET_PLACED" -> j using the RecordBuyOrders func. with a buyorder obj. that has only that change
            //isn't that much more innefficent i suppose
            buyorder currentBuyOrder;
            currentBuyOrder.ticker = oldbuyordersticker;
            currentBuyOrder.buyid = oldbuyorder.id;
            currentBuyOrder.sell_lim_id = "NOT_YET_PLACED";
            ListofBuyOrders.push_back(currentBuyOrder);


        }

        RecordBuyOrders(ThisFilesDate, ListofBuyOrders);
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
    auto client = alpaca::Client(env);

    //Run init() func. and check for errors
    if (int ret = Init(client); ret != 0)
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
        if (int ret = GetData(DIRECTORY, SixMonthsBeforeYesterdayAsString+"T09:30:00-04:00", YesterdayAsString+"T16:00:00-04:00", client); ret != 0)
            return ret;

    }

    //Seperate what we do at three different times below
    while (true)
    {
        //check to see if we need an emergencyabort()
        auto account_response = client.getAccount();
        if (auto status = account_response.first; !status.ok())
        {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            sleep(15);
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
                else
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


            //shit here is to avoid margin calls --> implements the constant intraday trading on MOO and MOC
            if (now.time_of_day().hours() == 22 && now.time_of_day().minutes() == 30 && HasShitGoneDown == false)//shit goes down
            {
                if (int i = ChangeUpTheFiles(client); i != 0)
                    return (i);

                HasShitGoneDown = true;
            }

            if (now.time_of_day().hours() == 9 && now.time_of_day().minutes() == 35 && TodaysDailyLimSellsPlaced == false)
            {

                /*
                 * Lets wait until all the open orders have been filled (in case it takes more than a min to place all of them,
                 * which is very possible -- or not that's a good idea but i'm lazy, we'll j wait till 35 and catch the exception
                 * in placelimsell orders and wait there if need be...
                 */



                /*
                 * Places all Lim. sell orders for everything, in whateever order files is read from CurrentlyBought dir
                 * Some other random variable like hasthishappened once or whatever is deprecated now lol
                 */
                vector<string> files;
                for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
                {
                    files.push_back(file.path());
                }

                for (int i = 0; i < files.size(); i++)//could use iterator here again, but whatever, fuck optimizing memory or runtime amiright or amiright?
                {
                    PlaceLimSellOrders(client, files[i]);

                }

                TodaysDailyLimSellsPlaced = true;
            }



        }

        cout << "Algo is now running... Current date/time is: " << to_iso_extended_string(boost::posix_time::second_clock::local_time()) << endl;
        sleep(15);
    }


    return 0;//tho never run ig...
}
#pragma clang diagnostic pop