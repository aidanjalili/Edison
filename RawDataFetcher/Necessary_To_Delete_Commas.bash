#!/bin/bash
for FILE in ./Earnings_Calls_Data/*; do  perl -pe 's/\"(.+?),(.+?)\"/\"$1$2\"/g' $FILE; done

