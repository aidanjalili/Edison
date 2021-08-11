/*
 * ALGO DOES THE FOLLOWING....
 * Volume >= averagevolume+ 3*stdev && Close > 1.2*yesterdaysclose && Open <= 20 and days later = 5 and stop loss at 1 percent
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

#include "csv.h"
#include "alpaca/alpaca.h"

using namespace std;


/*Constants*/
const string DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/VSEB";//should eventually change to just echoing a pwd command
const bool TWENTY_FIVE_K_PROTECTION = true;

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


//These varaibles are "reset" every day...
bool HaveAlreadyRunRefreshToday = false;
bool HaveAlreadyPlacedOrders = false;


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
int PlaceLimSellOrders(alpaca::Client& client);

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
    if (auto status = get_calendar_response.first; !status.ok()) {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    datesmarketisopen = get_calendar_response.second;

   UpdateAssets(client);

    return 0;

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
    //filter out inactive assets...
    for (int i = 0; i<assets.size(); i++)
    {
        if (assets[i].tradable != true || assets[i].easy_to_borrow == false)//if it's not tradable and shortable delete it
        {
            assets.erase(assets.begin() + i);
            continue;
        }
        else//go thru all the bannedtickers and c if it matches to this ticker.. if it does delete it
        {
            for (auto iterator = bannedtickers.begin(); iterator!=bannedtickers.end(); iterator++)
            {
                if (assets[i].symbol == (*iterator))
                {
                    assets.erase(assets.begin() + i);
                    break;
                }
            }
        }
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
        DeleteRecord(InputDir+"/RawData/"+(*iter).symbol, 1);

        //Then add today's data
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

        //TO DO: If any stocks that I currently bought have changed from easy to borrow
        //To hard to borrow... and then do something to like idk cover as soon as possible
    }

    UpdateAssets(client);

    PlaceLimSellOrders(client);//note that its return value is discarded...

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
            auto timetobuy = closingtime - boost::posix_time::minutes(15);//subtract approx. 48 mins for algo to run so that it finishes before 350 to place MOC orders
            string timetobuyasstring = to_simple_string(timetobuy);
            ret.hours = stoi(timetobuyasstring.substr(0,2));
            ret.minutes = stoi(timetobuyasstring.substr(3,2));
            return ret;
        }
    }
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
    if (DateToSellGivenDateToBuy(TodaysDateAsString) != TodaysDateAsString)
        return 69;

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
    boost::gregorian::date BuyDate = boost::gregorian::from_simple_string(DateofBuy);
    boost::gregorian::date SellDate;

    string BuyDateAsString = to_iso_extended_string(SellDate);
    int i = 0;
    for (; i< datesmarketisopen.size(); i++)
    {
        if (datesmarketisopen[i].date == BuyDateAsString)
        {
            break;
        }
    }
    string ret = datesmarketisopen[i+5].date;
    return ret;
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
        cash = cashinsideaccount-25000;
    else
        cash = cashinsideaccount;

    if (RunNumber != 0)//Then cash should be split into two -- cuz at any given time we have two seperate days worth of orders out...
    {
        if (RunNumber == 1)
            cash = cash/5;
        else if (RunNumber == 2)
            cash = cash/4;
        else if (RunNumber == 3)
            cash = cash/3;
        else if (RunNumber == 4)
            cash = cash/2;
        else
            cash = cash;//do nothing...
    }

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
        return 0;

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
        if (qty == 0)//should never happn but alwyas good to check
        {
            cout << "qty was 0 for some rzn" << endl;
            continue;
        }


        auto submit_order_response = client.submitOrder(
                (*Iterator),
                qty,
                alpaca::OrderSide::Sell,
                alpaca::OrderType::Market,
                alpaca::OrderTimeInForce::CLS
        );
        if (auto status = submit_order_response.first; !status.ok()) {
            std::cerr << "Error calling API: " << status.getMessage() << std::endl;
            //some error buying this ticker...
            continue;
        }
        auto order_response = submit_order_response.second;
        string thisbuyid = order_response.id;

        sleep(8);//wait for buy order to go thru before you place next one...

        string thislimid = "NOT_YET_PLACED";//"N/A" for now as it will be placed later with REFRESH function...

        buyorder ThisBuyOrder;
        ThisBuyOrder.ticker = (*Iterator);
        ThisBuyOrder.buyid = thisbuyid;
        ThisBuyOrder.sell_lim_id = thislimid;
        BuyOrders.push_back(ThisBuyOrder);
    }
    boost::gregorian::date DateToday = boost::gregorian::day_clock::local_day();
    std::string DateTodayAsString = to_iso_extended_string(DateToday);
    RecordBuyOrders(DateTodayAsString, BuyOrders);
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

int PlaceLimSellOrders(alpaca::Client& client)
{

    //go thru currently bought, get ticker from their id and price shorted at, then change set double var "price" to that
    //after submitting limit buy order here (after sleep(2)) change the "NOT_YET_PLACED" to the limid
    //also change func. to void

    vector<string> files;
    for (auto& file : filesystem::directory_iterator(DIRECTORY+"/CurrentlyBought"))
    {
        files.push_back(file.path());
    }
    sort(files.begin(), files.end());//Now we only work the most recent one -- so the first one in the vector

    string newfilename = files[-1].substr(DIRECTORY.size()+17, 10)+"-new.csv";//returns YYYY-MM-DD-new.csv
    newfilename = DIRECTORY+"/CurrentlyBought/" + newfilename;
    std::ofstream newFile(newfilename);
    newFile << "ticker,buyid,sell_lim_id\n";

    io::CSVReader<3> in( (files[-1]).c_str() );
    in.read_header(io::ignore_extra_column, "ticker", "buyid", "sell_lim_id");

    std::string ticker, buyid, sell_lim_id;
    while(in.read_row(ticker , buyid, sell_lim_id))
    {

        auto get_order_response = client.getOrder(buyid);
        auto order_response = get_order_response.second;
        double price = stod(order_response.filled_avg_price);
        double limitprice = price*1.01;
        bool checksecondlimitsell = false;

        int qty = stoi(order_response.qty);

        auto submit_limit_order_response = client.submitOrder(
                (order_response.symbol),
                qty,
                alpaca::OrderSide::Buy,
                alpaca::OrderType::Limit,
                alpaca::OrderTimeInForce::GoodUntilCanceled,
                to_string(limitprice)
        );

        std::pair<alpaca::Status, alpaca::Order> submit_order_again;

        if (auto status = submit_limit_order_response.first; !status.ok())
        {
            if (status.getMessage() != "asset " + order_response.symbol + " cannot be sold short")//cuz this happens a lot for some rzn...
            {
                std::cerr << "SOMEHOW THE BUY ORDER COULD BE SUBMITED BUT THERE WAS AN ERROR SUBMITTING THE LIM ORDER... API RESPONSE ERROR WAS: " << status.getMessage() << std::endl;
                return 666;
            }
            else
            {
                checksecondlimitsell = true;
                while (true)
                {
                    sleep(1);


                    auto submit_limit_order_response = client.submitOrder(
                            (order_response.symbol),
                            qty,
                            alpaca::OrderSide::Buy,
                            alpaca::OrderType::Limit,
                            alpaca::OrderTimeInForce::GoodUntilCanceled,
                            to_string(limitprice)
                    );

                    auto status = submit_order_again.first;
                    if (status.ok())
                    {
                        break;
                    }

                    if (!status.ok() && status.getMessage() == "asset " + order_response.symbol + " cannot be sold short")
                    {
                        continue;
                    }
                    else if (!status.ok())
                    {
                        std::cerr << "SOMEHOW THE BUY ORDER COULD BE SUBMITED BUT THERE WAS AN ERROR SUBMITTING THE LIM ORDER... API RESPONSE ERROR WAS: " << status.getMessage() << std::endl;
                        return 666;
                    }
                }
            }
        }
        string thislimid;
        if (checksecondlimitsell == false)
        {
            auto limit_order_response = submit_limit_order_response.second;
            thislimid = limit_order_response.id;
        }
        else
        {
            auto limit_order_response = submit_order_again.second;
            thislimid = limit_order_response.id;
        }


        string newline = order_response.symbol + "," + order_response.id + "," + thislimid;
        newFile << newline + "\n";

        sleep(2);//wait for lim sell order to be submitted before submitting another one...

    }
    // removing the existing file
    remove( (files[-1]).c_str());

    // renaming the new file with the existing file name
    rename( newfilename.c_str(), files[-1].c_str() );

    newFile.close();


}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
    setenv("APCA_API_KEY_ID", "PKNPM7HFBCX963JDQY7H", 1);
    setenv("APCA_API_SECRET_KEY", "Hu0wQlwPrbEUTOO0EHmAdLD7lQkpWz3UD7rsynMy", 1);

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
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

        //If today is a day that the market was/is open
        boost::gregorian::date TodaysDate = boost::gregorian::day_clock::local_day();
        std::string TodaysDateAsString = to_iso_extended_string(TodaysDate);

        if (IsGivenDayATradingDay(TodaysDateAsString, datesmarketisopen))
        {
            HomeMadeTimeObj BuyTime = FetchTimeToBuy(datesmarketisopen);
            if (now.time_of_day().hours() == BuyTime.hours && now.time_of_day().minutes() == BuyTime.minutes)
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
                    int Sell_Status_code = Sell(client);
                    if (Sell_Status_code != 0)
                    {
                        //Problems...
                        ///eventually -- Make ALERT() function and place here -- potentially with Iftt
                        return Sell_Status_code;
                    }
                    else
                    {
                        if (int buystatus = Buy(0, client); buystatus!=0)
                        {
                            return buystatus;
                        }
                    }
                }
                //else case should never happen

                ///TO DO's Start here

                //Onaug. 23rd -- add to 18th bday list or whatever, ensure PDT protection is on for "both"
                ///And end here

                HaveAlreadyPlacedOrders = true;
            }

            //If time is 1130 -- run Refresh()
            if (now.time_of_day().hours() == 23 && now.time_of_day().minutes() == 30)
            {
                Refresh(DIRECTORY, client);//This should take abt 15 mins depending on wifi speed...
                HaveAlreadyRunRefreshToday = true;
            }

            //at midnight reset all variables .. doesn't matter if this is done (up to) 4 times during the min.
            if (now.time_of_day().hours() == 21 && now.time_of_day().minutes() == 59)
                ResetVariables();
        }

        cout << "Algo is now running... Current date/time is: " << to_iso_extended_string(boost::posix_time::second_clock::local_time()) << endl;
        sleep(15);
    }


    return 0;//tho never run ig...
}
#pragma clang diagnostic pop