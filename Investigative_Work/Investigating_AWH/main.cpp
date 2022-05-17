/*
 * THIS DID NOT WORK
 */

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


#include "stdlib.h"
#include "stdio.h"
#include "string.h"

using namespace std;


//ive tested for the last two thousand days
//ive had one failure:
//with a loss on that day of:

//every other day has either been a 0% gain (x times)
//or a 1% gain ( times)

//when we start and end our analysis
//string startdate = "2016-11-24T09:30:00-04:00";
//string enddate = "2019-08-21T09:30:00-04:00";
string startdate = "2020-06-12T09:30:00-04:00";
string enddate = "2022-05-17T09:30:00-04:00";

string FILEPATH = "/Users/aidanjalili03/Desktop/Edison/Investigative_Work/Investigating_AWH/AWH.csv";

//The only data we care about is date, open, close, and volume
void WriteToCsvOutputs(string filename, vector<alpaca::Bar>& InputBar)
{
    //cout << "sex" << endl;
    ofstream OutputFile(filename);
    //cout << "sexy" << endl;

    //Write the columns
    OutputFile << "Date, Open, Close, High, Low, Volume\n";

    //Write the rows
    for(auto iter = InputBar.begin(); iter!=InputBar.end(); iter++)
    {
        OutputFile << (*iter).time  << "," << to_string((*iter).open_price) << "," << to_string((*iter).close_price) << "," << to_string((*iter).high_price) << "," << to_string((*iter).low_price) << "," << to_string((*iter).volume) << "\n";
    }

    OutputFile.close();

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


    if (FirstRun())
    {
        //First... get data
        auto bars_response = client.getBars({"AWH"}, startdate, enddate, "", "", "1Day", 1000);
        //NEED TO REMOVE "[]" characters for json IsArray() to work
        if (auto status = bars_response.first; !status.ok())
        {
            std::cerr << "Error getting bars information: " << status.getMessage() << std::endl;
            exit(-1);
        }

        auto bars = bars_response.second.bars["AWH"];

        WriteToCsvOutputs(FILEPATH, bars);
        //cout << bars << endl;
    }
    //cout << "sex" << endl;

    //Now analyze...

    //For now simply tell me how many times the high was lower than the open for any given day...
    unsigned long long NumberofTimes = 0;
    unsigned long long  NumberofTimesItWasnt = 0;
    io::CSVReader<6> in(FILEPATH);in.read_header(io::ignore_extra_column, "Date", "Open", "Close", "Low", "High", "Volume");
    double Open, Close, Low, High, Volume;
    string Date;
    while (in.read_row(Date, Open, Close, Low, High, Volume))
    {
        double stopbuyprice = Open+0.01;//floor(Open*1.01*100)/100;
        if  (High >= stopbuyprice)
            NumberofTimes++;
        else
        {
            double diff = Close - stopbuyprice;
            cout << "by EOD difference in our stop buy and close was..." << diff << endl;
            double percentloss = ((Close-Open)/Open)*100;
            cout << "diff in close price vs our open was: " << Close-Open << "which means we would've had a percent loss of: " << percentloss << "%" << endl;
            cout << "Date was: " << Date << endl;
            NumberofTimesItWasnt++;
        }

    }
    cout << "Number of times our stop buy hit : " << NumberofTimes << endl;
    cout << "Number of times it wasn't: " << NumberofTimesItWasnt << endl;

    return 0;
}

