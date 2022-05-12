import os

import csv
# assign directory
directory = '/Users/aidanjalili03/Desktop/Edison/RawDataFetcher/Earnings_Calls_Data'

# iterate over files in
# that directory
for filename in os.listdir(directory):
    f = os.path.join(directory, filename)
    # checking if it is a file
    if os.path.isfile(f):
        #do stuff with file here
        print(f)
        with open(f,"rt") as source:
            rdr= csv.reader( source )
            with open(f+"-new.csv","wt") as result:
                wtr= csv.writer( result )
                for r in rdr:
                    wtr.writerow( (r[0], r[2], r[3], r[4], r[5], r[6], r[7],r[8],r[9]) )

        os.remove(f)
        os.rename(f+"-new.csv", f)
        #then delete f and rename f-new to f
