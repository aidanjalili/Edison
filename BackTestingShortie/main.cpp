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
std::string Date, Datetwo;
double Open, Opentwo;
double Close, Closetwo;
double Low, Lowtwo;
double High, Hightwo;
double Volume, Volumetwo;

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

int main()
{
    /*INIT*/
    setenv("APCA_API_KEY_ID", "PK79IPR80TSVA4WMS2Y8", 1);
    setenv("APCA_API_SECRET_KEY", "oYVySwg0kD3wA2x8wMiUW5ArNJY7eYrMLx6dk17h", 1);
    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);

//    PrintDates(client);
//    exit(0);


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
    //filter out non-shortable
    for (int i = 0; i<assets.size(); i++)
    {
        if (assets[i].easy_to_borrow == false)//if it's not tradable delete it
        {
            assets.erase(assets.begin() + i);
            continue;
        }
    }
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

            if (Volume >= averagevolume+ 3*stdev && Close > 1.2*yesterdaysclose && Open <= 20 && DoneCountingAvgVolumes == true)
            {
                for (auto& x : assets)//checks to c if it's easy to borrow
                {
                    if (x.symbol == currentasset)
                    {
                        ShortOrder ThisShort;
                        ThisShort.ticker = currentasset;
                        ThisShort.date = Date;
                        ThisShort.price_at_short = Close;
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

        for (auto& currentshort : AllShorts)
        {
            INDEX = 0;

            io::CSVReader<6> intwo( DIR+"/"+currentshort.ticker+".csv" );
            intwo.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
            while (intwo.read_row(Datetwo, Opentwo, Closetwo, Lowtwo, Hightwo, Volumetwo))
            {
                if (INDEX >= currentshort.index+1  &&  INDEX <= currentshort.index+DAYSLATER)
                {

                    cout << "INDEX is: " << INDEX << endl;
                    cout << "buy order\'s INDEX is: " << currentshort.index << endl;

                    /*Percent returns are calculated as if it were a buy... in results you need to multiply all percent_returns by -1*/
                    cout << "high currently si: " << Hightwo << "$" << endl;
                    cout << "But we shorted at: " << currentshort.price_at_short << "$"<< endl;
                    if (Hightwo >= 1.01*currentshort.price_at_short) //stop loss
                    {
                        cout << "current date in doc: " << Datetwo << endl;
                        cout << "date of short: " << currentshort.date << endl;
                        cout << "ticker: " << currentshort.ticker << endl;
                        LineOfOutput ThisLineOfOutput;
                        ThisLineOfOutput.date = Datetwo;
                        ThisLineOfOutput.ticker = currentshort.ticker;
                        ThisLineOfOutput.percent_return = 0.01;
                        Outputs.push_back(ThisLineOfOutput);
                        INDEX++;//tho this doesn't rly do much as it's reset
                        break;
                    }
//                    else if (Lowtwo <= 0*currentshort.price_at_short) //stop loss
//                    {
//                        cout << "current date in doc: " << Datetwo << endl;
//                        cout << "date of short: " << currentshort.date << endl;
//                        cout << "ticker: " << currentshort.ticker << endl;
//                        LineOfOutput ThisLineOfOutput;
//                        ThisLineOfOutput.date = Datetwo;
//                        ThisLineOfOutput.ticker = currentshort.ticker;
//                        ThisLineOfOutput.percent_return = -0.2;
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