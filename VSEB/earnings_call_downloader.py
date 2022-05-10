import pandas as pd
from datetime import datetime
from datetime import timedelta
import yahoo_fin.stock_info as si
import dateutil.parser
import csv


def main(DIRECTORY):

    print("Hello from file")

    # setting the report date
    report_date = datetime.now().date()
    timestampStr = report_date.strftime("%Y-%m-%d")


    todays_earnings = si.get_earnings_for_date(timestampStr)

    csv_columns = ['ticker','companyshortname','startdatetime','startdatetimetype','epsestimate','epsactual','epssurprisepct','timeZoneShortName','gmtOffsetMilliSeconds','quoteType']

    csv_file = DIRECTORY+"/Earnings_Data.csv"
    with open(csv_file, 'w') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=csv_columns)
        writer.writeheader()
        for data in todays_earnings:
            writer.writerow(data)
