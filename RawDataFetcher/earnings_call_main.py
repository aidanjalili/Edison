import pandas as pd
from datetime import datetime
from datetime import date
from datetime import timedelta
import yahoo_fin.stock_info as si
import dateutil.parser
import csv
import time
from earnings_call_downloader import main
#RMB: NEED TO DELETE ALL COMMAS BETWEEN QUOATATIONS IN ALL CSV FILES AFTER YOU RUN THIS (VIA THAT TERMINAL COMMAND)
#start 2018-06-05, end 2021-08-06
start_date = date(2018, 6, 4)
end_date = date(2018, 6, 4)
delta = timedelta(days=1)
while start_date <= end_date:
    # setting the report date

    main(start_date)
    start_date += delta
    time.sleep(30)
