#include <iostream>
#include "alpaca/alpaca.h"
#include <stdlib.h>
#include "csv.h"
#include <tuple>
#include <cmath>
#include <fstream>
#include <numeric>

#include "stdlib.h"
#include "stdio.h"
#include "string.h"



using namespace std;


//all %returns
vector<double> percentreturns;

vector<double> volumes;
//Date, ticker, buy price, row number
vector<tuple<string, string, double, int>> Buys;

void SetEnvironmentVariables()
{
    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);
}

void Init()
{
    SetEnvironmentVariables();
}


/*TODO*/
// *
// *


double Stdeviation(vector<double>& v, double mean)
{
    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    return std::sqrt(sq_sum / v.size());

}


double avg(std::vector<double> const& v)
{
    return std::accumulate(v.begin(), v.end(), 0LL) / v.size();
}


int main()
{

    Init();

    auto env = alpaca::Environment();
    if (auto status = env.parse(); !status.ok())
    {
        std::cout << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::Client(env);

    //end init

    /*Tickers*/

    auto get_assets_response = client.getAssets();
    if (auto status = get_assets_response.first; !status.ok())
    {
        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
        return -1;//a -1 return to main means there was an error in getting the ticker list
    }
    auto assets = get_assets_response.second;
    /*Loops thru tickers here*/ //--> should put into a seperate function later

    for (const auto& currentticker : assets)
    {
//        io::CSVReader<6> in("/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/RawData/" + currentticker.symbol + ".csv");
//        in.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
        //std::string Date; double Open; double Close; double Low; double High; double Volume;

//        int quickcheck = 0;
//        while(in.read_row(Date , Open, Close, Low, High, Volume))
//        {
//            cout << "Sex" << endl;
//            quickcheck++;
//        }
//        if (quickcheck < 50)//should actually be 505 but we'll be a lil leniant
//            continue;



        //starts here shit
        bool DoneCountingAvgVolumes = false;
        int index =0;
        int currentbuyorder = 0;
        double yesterdaysclose = 0;
        double yesterdaysclosetemp=0;
        Buys.clear();
        Buys.shrink_to_fit();
        volumes.clear();
        volumes.shrink_to_fit();

        io::CSVReader<6> in( ("/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/RawData/" + currentticker.symbol + ".csv").c_str() );
        in.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
        std::string Date; double Open; double Close; double Low; double High; double Volume;

        ifstream in_file(("/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/RawData/" + currentticker.symbol + ".csv").c_str(), ios::binary);
        in_file.seekg(0, ios::end);
        int file_size = in_file.tellg();//file_size in bytes, --> checks for empty/durastically incomplete tickers...
        if (file_size <= 3000)//less than ~3KB
        {
            cout << "**Error/Incomplete Data for " << currentticker.symbol << "**\n";
            continue;
        }

        while(in.read_row(Date , Open, Close, Low, High, Volume))
        {

            if(index == 0)
                yesterdaysclosetemp = Close;
            else
            {
                yesterdaysclose = yesterdaysclosetemp;
                yesterdaysclosetemp = Close;
            }


            double averagevolume = 0;
            double stdev = 0;
            if (Date == "2018-06-04" || volumes.size() > 255)//arbitrary 255 value, but if it gets too big it means that the data didnt have this date for whatever rzn...
            {
                DoneCountingAvgVolumes = true;
            }

            if (DoneCountingAvgVolumes == false)
            {
                volumes.push_back(Volume);
                continue;
            }
            if (DoneCountingAvgVolumes == true)//if date == 2018-06-04 or later
            {
                averagevolume = avg(volumes);
                stdev = Stdeviation(volumes, averagevolume);
                //cout << averagevolume << "   " << stdev << endl;

                if (Buys.size() != 0)
                {
                    //check for sells
                    tuple<string, string, double, int> currentbuy ;
                    currentbuy = make_tuple(get<0>(Buys[currentbuyorder]), get<1>(Buys[currentbuyorder]), get<2>(Buys[currentbuyorder]), get<3>(Buys[currentbuyorder]));
                    if (index <= get<3>(currentbuy)+2)
                    {
                        if (High > 1.35*get<2>(currentbuy))
                        {
                            percentreturns.push_back(0.35);
                            currentbuyorder++;
                        }
                        else if (index == get<3>(currentbuy)+2)
                        {
                            double currentpercentreturn;
                            currentpercentreturn = (Close-get<2>(currentbuy)) /get<2>(currentbuy);
                            percentreturns.push_back(currentpercentreturn);
                            currentbuyorder++;
                        }
                    }

                }
                if (Volume >= averagevolume+ (8*stdev) && Close > yesterdaysclose)
                {
                    //buys
                    Buys.push_back(make_tuple(Date, currentticker.symbol, Close, index));
                }

            }

            volumes.erase(volumes.begin());
            volumes.push_back(Volume);
            index++;
        }

        cout << "just finished looking at: " + currentticker.symbol << endl;
        cout << "Number of trades currently is: " << to_string(percentreturns.size()) << endl;
        //Comment line below when using "currentticker.symbol"
        //break;
    }



    ofstream OutputFile("/Users/aidanjalili03/Desktop/Edison/BackTestingVSEB/percent_returns_real.csv");
    for(auto iter = percentreturns.begin(); iter!=percentreturns.end(); iter++)
        OutputFile << to_string((*iter)) << "\n";

    OutputFile.close();

    return 0;
}