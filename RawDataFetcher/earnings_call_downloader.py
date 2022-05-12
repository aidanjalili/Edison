import pandas as pd
from datetime import datetime
from datetime import date
from datetime import timedelta
import yahoo_fin.stock_info as si
import dateutil.parser
import csv
import time

DIRECTORY = "/Users/aidanjalili03/Desktop/Edison/RawDataFetcher"
def main(input_date):

    #start 2018-06-05, end 2021-08-06
    # start_date = date(2018, 9, 17)
    # end_date = date(2021, 8, 6)
    # delta = timedelta(days=1)

# while start_date <= end_date:
    # setting the report date
    #report_date = datetime.now().date()
    timestampStr = input_date.strftime("%Y-%m-%d")

    try:
        todays_earnings = si.get_earnings_for_date(timestampStr)
    except:
        time.sleep(45)
        todays_earnings = si.get_earnings_for_date(timestampStr)

    csv_columns = ['ticker','companyshortname','startdatetime','startdatetimetype','epsestimate','epsactual','epssurprisepct','timeZoneShortName','gmtOffsetMilliSeconds','quoteType']

    csv_file = DIRECTORY+"/Earnings_Calls_Data/"+timestampStr+".csv"
    with open(csv_file, 'w') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=csv_columns)
        writer.writeheader()
        for data in todays_earnings:
            writer.writerow(data)

    # start_date += delta
    # time.sleep(2)
    print("printed one file -- file was: " + DIRECTORY+"/Earnings_Calls_Data/"+timestampStr+".csv")
