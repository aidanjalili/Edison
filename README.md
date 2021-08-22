# Edison

Makes money

## Important Notes
This program should only be run the morning of the first day you'd like to make buys. So if you want the first day you trade stocks today make sure to check that:
A) the market is open today
and B) that you run it sometime in the morning before ~3pm. OR on a weekend sometime during the day.

Also after you pull you need to create your own two directories "CurrentlyBought" and "RawData" (the latter only if it's your first run) inside of VSEB.

Finally, it might be a good idea to do a "hard" refresh of this program every three months or so
or as the user desires to update its ticker list.

## A couple real quick technical notes

The dates in the "CurrentlyBought" directory refer to the date that each of those
assets were bought.

The dates in the "Archives"
directory refer to the date archived (which sometimes is the date sold in the case of a manual sell at closing.
If the asset was sold through a limit sell it is simply the date archived/the date the asset it would have been 
sold had the limit order not been filled).

## What to do when rich/if this actually starts to work

There isn't much. First figure out how much (either as a func. of market cap, or outstanding shares, etc.) Go thru my backtesting, find the max number of tickers invested in in a week. Add 2 (j for securituy). then go thru each ticker listed their, find their market cap/outstanding shares, or whatever it is AT THAT TIME of the hypothetical trade. Make a list of those, avg it out, use that and multiply by max number of tickers in a weeek found earlier + 2. Can also double check that hopefully the stdev of the market caps/outstanding shares or whatever isn't too high. Every amnt of money I have in my accnt after that i can trasfer to a savings accnt. (Update the 75000 if-statement in code to reflect the avg market cap/whatever * acceptable proportion.)

