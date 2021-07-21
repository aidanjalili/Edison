#include "Init.h"

/*
 * My attempt at trying to put all init into seperate file
 */

#include <string>
std::string EffectiveWorkingDirectory = "/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/";

//#include <iostream>
//#include <stdlib.h>
//#include "alpaca/alpaca.h"
//
//using namespace std;
//
//
//vector<string> GetListofTickers()
//{
//    auto get_assets_response = client.getAssets();
//
//    //Error catching
//    if (auto status = get_assets_response.first; !status.ok())
//    {
//        std::cerr << "Error calling API: " << status.getMessage() << std::endl;
//        return vector<string> vect{"Error"};
//    }
//
//    for (const auto& asset : assets)
//    {
//        cout<< asset.symbol << endl;
//    }
//}
//
//int SetEnvironmentVariables()
//{
//    setenv("APCA_API_KEY_ID", "PKLT0ZT8YQSKQNL1Q1CM", 1);
//    setenv("APCA_API_SECRET_KEY", "Grf0tl7aXs6qdt2EbmoEV8llmUAMeHTGLjk8JgJR", 1);
//
//    return 0;
//
//}
//
//
//int Init()
//{
//    if (SetEnvironmentVariables() != 0)
//        return SetEnvironmentVariables();
//    else if (GetListofTickers()[0] == "Error")//-1 is the error code for somehow not getting the list of tickers
//        return -1;
//    else
//        return 0;
//}
