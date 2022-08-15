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
#include <unistd.h>


#include "stdlib.h"
#include "stdio.h"
#include "string.h"

using namespace std;


struct ShortOrder{
    string ticker;
    string date;
    double price_at_short;
    unsigned long long index;
};

struct LineOfOutput{
    string date;
    string ticker;
    double percent_return;
};


vector<string> files;
vector<alpaca::Asset> assets;
const string DIR = "/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/RawData";
const int DAYSLATER = 5;
vector<ShortOrder> AllShorts;
vector<LineOfOutput> Outputs;
unsigned long long INDEX = 0;
std::string Date, Datetwo, DateThree;
double Open, Opentwo, OpenThree;
double Close, Closetwo, CloseThree;
double Low, Lowtwo, LowThree;
double High, Hightwo, HighThree;
double Volume, Volumetwo, VolumeThree;

double avg(const std::vector<double>& v)
{
    unsigned long long sum = 0;
    for (int i = 0; i < v.size(); i++)
    {
        sum+=v[i];
    }
    return sum/v.size();
}

double Stdeviation(const vector<double>& v, double mean)
{
    vector<unsigned long long> squareofdifferencestomean;
    for (int i = 0; i < v.size(); i++)
    {
        squareofdifferencestomean.push_back( (v[i] - mean)*(v[i] - mean) );
    }
    auto sumofsquares = std::accumulate(squareofdifferencestomean.begin(), squareofdifferencestomean.end(), 0.00);
    unsigned long long variance = sumofsquares/squareofdifferencestomean.size();
    return std::sqrt(variance);

}

void PrintDates(alpaca::Client& client)
{
    auto get_calendar_response = client.getCalendar("2021-03-15", "2021-08-06");
    if (auto status = get_calendar_response.first; !status.ok()) {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
    }
    auto datesmarketisopen = get_calendar_response.second;
    bool isO4 = true;
    for (auto& x : datesmarketisopen)
    {
        if (x.date == "2018-11-05")
            isO4 = false;
        if (x.date == "2019-03-11")
            isO4 = true;
        if (x.date == "2019-11-04")
            isO4 = false;
        if (isO4)
            cout << x.date << "T04:00:00Z" << endl;
        else
            cout << x.date << "T05:00:00Z" << endl;
    }
}

bool tester(ShortOrder CurrentShort)
{
//    try
//    {
        io::CSVReader<9> in(("/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/Earnings_Calls_Data/" + CurrentShort.date.substr(0,10) +".csv").c_str());
        in.read_header(io::ignore_extra_column, "ticker", "startdatetime", "startdatetimetype", "epsestimate", "epsactual", "epssurprisepct", "timeZoneShortName", "gmtOffsetMilliSeconds", "quoteType");
        std::string ticker, startdatetime, startdatetimetype, epsestimate,epsactual,epssurprisepct,timeZoneShortName,gmtOffsetMilliSeconds,quoteType;
        while(in.read_row(ticker , startdatetime, startdatetimetype, epsestimate,epsactual,epssurprisepct,timeZoneShortName,gmtOffsetMilliSeconds,quoteType))
        {
            //currently in typical order
            if (ticker == CurrentShort.ticker)
            {
                //cout << "cocky" << endl;
                return true;
            }
        }
       // cout << "cock" << endl;
        return false;
//    }
//    catch (...)
//    {
//        cout << CurrentShort.date << endl;
//        cout << "***** FUCKITY FUCK FUCK FUCK " << endl;
//        cout << '\a' << endl;
//        return false;
//    }
}

double MySpecialRandFunc()//note that the weights are pretty arbitrary as no real data to go of...
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1,10000); // distribution in range [1, 6]
//double weights[] =  {0.40, 0.1, 0.15, 0.1, 0.08,0.07,0.05,0.035,0.0125,0.0025};

    int classifier = dist6(rng);
    if (classifier >= 1 &&classifier <= 4000)
        return 0.001;
    else if (classifier >= 4001 && classifier <= 5000 )
        return 0.002;
    else if (classifier >= 5001 && classifier <=6500)
        return 0.003;
    else if (classifier >= 6501 && classifier <=7500)
        return 0.004;
    else if (classifier >= 7501 && classifier <= 8300)
        return 0.005;
    else if (classifier >= 8301 && classifier <= 9000)
        return 0.006;
    else if (classifier>=9001 && classifier <= 9500)
        return 0.007;
    else if (classifier>=9501 && classifier <= 9850)
        return 0.008;
    else if (classifier >= 9851 && classifier <= 9975)
        return 0.009;
    else if (classifier >= 9976 && classifier <= 10000)
        return 0.01;
}

bool filtertoetb(alpaca::Asset& asset)
{
    if (asset.easy_to_borrow==false)//if its not etb erase it
        return true;
    else//else (i.e. it is etb) then don't
        return false;
}


int main()
{
    /*INIT*/
    setenv("APCA_API_KEY_ID", "AKUL7PSSDDM0UW4BXKH8", 1);
    setenv("APCA_API_SECRET_KEY", "BJSzEiXaZxaMExzV8iWj8bc3akKiSNC3QDr8vP0s", 1);
    setenv("APCA_API_BASE_URL", "api.alpaca.markets", 1);
    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);


    /*INIT Over*/

    //Get all files
    for (const auto& file : filesystem::directory_iterator(DIR))
    {
        files.push_back(file.path());
    }

    /*get all assets*/
    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; status.ok() == false)
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return -1;//a -1 return means there was an error in getting the ticker list
    }
    assets = get_assets_response.second;
    erase_if(assets, filtertoetb);
    //filter out non-shortable


    //end of making assets list


    //loop thru all files
    for (auto& file_path : files)
    {

        vector<double> volumes;
        bool DoneCountingAvgVolumes = false;

        //get currentasset name
        string currentasset = file_path.substr(DIR.size() + 1, ((file_path.size() - DIR.size()) - 5));

        io::CSVReader<6> in(file_path);
        in.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");

        ifstream in_file(file_path);
        in_file.seekg(0, ios::end);
        int file_size = in_file.tellg();//file_size in bytes, --> checks for empty/durastically incomplete tickers...
        if (file_size <= 3000)//less than ~3KB
        {
            cout << "**Error/Incomplete Data for " << currentasset << "**\n";
            continue;
        }

        INDEX = 0;
        double yesterdaysclose, yesterdaysclosetemp;
        while (in.read_row(Date, Open, Close, Low, High, Volume))
        {


            if(INDEX == 0)
                yesterdaysclosetemp = Close;
            else
            {
                yesterdaysclose = yesterdaysclosetemp;
                yesterdaysclosetemp = Close;
            }

            double averagevolume = 0;
            double stdev = 0;
            if (Date == "2018-06-04T04:00:00Z" || volumes.size() > 255)//arbitrary 255 value, but if it gets too big it means that the data didnt have this date for whatever rzn...
            {
                DoneCountingAvgVolumes = true;
            }

            if (DoneCountingAvgVolumes == false)
            {
                volumes.push_back(Volume);
                INDEX++;
                continue;
            }
            if (DoneCountingAvgVolumes == true)//if date == 2018-06-04 or later
            {
                averagevolume = avg(volumes);
                stdev = Stdeviation(volumes, averagevolume);
            }

            if (Volume >= averagevolume+ 0.01*stdev && Close > 1.25*yesterdaysclose /*&& High <= 20 */ && DoneCountingAvgVolumes == true)
            {
                for (auto& x : assets)//checks to c if it's easy to borrow
                {
                    if (x.symbol == currentasset)
                    {
                        ShortOrder ThisShort;
                        ThisShort.ticker = currentasset;
                        ThisShort.date = Date;
                        double TomorrowsOpen = 0;
                        unsigned long long countertwo = 0;
                        io::CSVReader<6> inthree(file_path);
                        inthree.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
                        while (inthree.read_row(DateThree, OpenThree, CloseThree, LowThree, HighThree, VolumeThree))
                        {
                            if (countertwo<=INDEX)
                            {
                                countertwo++;
                                continue;
                            }
                            else if (countertwo == INDEX+1)
                            {
                                TomorrowsOpen = OpenThree;
                                countertwo++;
                                continue;
                            }
                            else
                            {
                                countertwo++;
                                break;
                            }
                        }
                        ThisShort.price_at_short = TomorrowsOpen;
                        ThisShort.index = INDEX;
                        AllShorts.push_back(ThisShort);
                    }
                }
            }
//            if (Open < 3) {
//                ShortOrder ThisShort;
//                ThisShort.ticker = currentasset;
//                ThisShort.date = Date;
//                ThisShort.price_at_short = Close;
//                ThisShort.index = INDEX;
//                AllShorts.push_back(ThisShort);
//            }
            INDEX++;
        }
    }
    cout << "cunt" << endl;
    //erase_if(AllShorts, tester);

        for (auto& currentshort : AllShorts)
        {
            INDEX = 0;

            try
            {
                io::CSVReader<6> intest( DIR+"/"+currentshort.ticker+".csv" );
            }
            catch (...)
            {
                break;
            }
            io::CSVReader<6> intwo( DIR+"/"+currentshort.ticker+".csv" );
            intwo.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
            double yesterdaysclose, yesterdaysclosetemp;
            while (intwo.read_row(Datetwo, Opentwo, Closetwo, Lowtwo, Hightwo, Volumetwo))
            {
                if(INDEX == 0)
                    yesterdaysclosetemp = Closetwo;
                else
                {
                    yesterdaysclose = yesterdaysclosetemp;
                    yesterdaysclosetemp = Closetwo;
                }



                if (INDEX >= currentshort.index+1  &&  INDEX <= currentshort.index+DAYSLATER)
                {

                    cout << "INDEX is: " << INDEX << endl;
                    cout << "buy order\'s INDEX is: " << currentshort.index << endl;

                    /*Percent returns are calculated as if it were a buy... in results you need to multiply all percent_returns by -1*/
                    cout << "high currently si: " << Hightwo << "$" << endl;
                    cout << "But we shorted at: " << currentshort.price_at_short << "$"<< endl;
                    //make sure it didn't move more than 1% up from where we bought it during pre/after hours and if it did
                    //push back percent return of diff...
//                    if (Opentwo > 1.01*yesterdaysclose)
//                    {
//                        double percent_return = (Opentwo-yesterdaysclose)/yesterdaysclose;
//                        if (percent_return != 0)
//                        {
//                            LineOfOutput ThisLineOfOutput;
//                            ThisLineOfOutput.date = Datetwo;
//                            ThisLineOfOutput.ticker = currentshort.ticker;
//                            ThisLineOfOutput.percent_return = percent_return;//percent_return;
//                            Outputs.push_back(ThisLineOfOutput);
//                            INDEX++;//tho this doesn't rly do much as it's reset
//                            break;
//                        }
//                        else
//                        {
//                            INDEX++;//again tho, this is unecessary
//                            break;
//                        }
//                    }
                    //assumign it didn't move more than 1% over night at any pt, one of the following will run...
                    /*above comment deprecated*/
                    if (Hightwo >= 1.02*currentshort.price_at_short) //stop loss
                    {
                        cout << "current date in doc: " << Datetwo << endl;
                        cout << "date of short: " << currentshort.date << endl;
                        cout << "ticker: " << currentshort.ticker << endl;
                        LineOfOutput ThisLineOfOutput;
                        ThisLineOfOutput.date = Datetwo;
                        ThisLineOfOutput.ticker = currentshort.ticker;
                        ThisLineOfOutput.percent_return = 0.02;//MySpecialRandFunc();
                        //assert(ThisLineOfOutput.percent_return >= 0.001 && ThisLineOfOutput.percent_return <= 0.01);
                        Outputs.push_back(ThisLineOfOutput);
                        INDEX++;//tho this doesn't rly do much as it's reset
                        break;
                    }
//                    else if (Lowtwo <= 0.95*currentshort.price_at_short) //5% stop loss
//                    {
//                        cout << "current date in doc: " << Datetwo << endl;
//                        cout << "date of short: " << currentshort.date << endl;
//                        cout << "ticker: " << currentshort.ticker << endl;
//                        LineOfOutput ThisLineOfOutput;
//                        ThisLineOfOutput.date = Datetwo;
//                        ThisLineOfOutput.ticker = currentshort.ticker;
//                        ThisLineOfOutput.percent_return = -0.05;
//                        Outputs.push_back(ThisLineOfOutput);
//                        INDEX++;//tho this doesn't rly do much as it's reset
//                        break;
//                    }
                    else if (INDEX == currentshort.index+DAYSLATER)
                    {
                        double percent_return = (Closetwo-currentshort.price_at_short)/currentshort.price_at_short;
                        if (percent_return != 0)
                        {
                            LineOfOutput ThisLineOfOutput;
                            ThisLineOfOutput.date = Datetwo;
                            ThisLineOfOutput.ticker = currentshort.ticker;
                            ThisLineOfOutput.percent_return = percent_return;
                            Outputs.push_back(ThisLineOfOutput);
                            INDEX++;//tho this doesn't rly do much as it's reset
                            break;
                        }
                    }

                }
                INDEX++;
            }
        }


        
    ofstream OutputFile("/Users/aidanjalili03/Desktop/Edison/BackTestingShortie/percent_returns.csv");
    OutputFile << "Date, Ticker, Percent Return" << "\n";
    for(auto iter  = Outputs.begin(); iter!=Outputs.end(); iter++)
        OutputFile << (*iter).date << "," << (*iter).ticker << "," << (*iter).percent_return << "\n";

    OutputFile.close();

    return 0;
}

