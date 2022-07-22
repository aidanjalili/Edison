# Edison

Makes money

## TO DO
Make it so that we updateassets/filterassets every day so that we always r having an updated list of those that are tradable and shortable.
--Right now all we do is anticipate canceled order/rejected orders as things that have become unshortable/untradeable we try to trade anyway.
But this is kind of a stupid fix.

## Important Notes
This program should only be run the morning of the first day you'd like to make buys. So if you want the first day you trade stocks today make sure to check that:
A) the market is open today
and B) that you run it sometime in the morning before ~3pm. OR on a weekend sometime during the day.

Also after you pull you need to create your own two directories "CurrentlyBought" and "RawData" and "Archives" (the latter only if it's your first run) inside of VSEB.

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

## For the future...

Presumably you'll get to the point where you're hitting the 75000/ticker price pt. At that point up it to like 100k or 125 even, maybe more. But then start working on adding a feature that fetches each tickers market cap before you invest. Then take the smallest and limit ur investment to some n% of that for all tickers. (Or if you rly want to be fancy n% of each tickers corresponding market cap.) Then maybe keep a log of how much that is every day. Then once you realize your hitting that on avg everyday, make sure you have enuf money in alpaca for the max  margin reqs you c feesible (given the data you've been collecting on avg. market cap of what you're investing in.) Then change back to pre-margin release as that will make more money, I think anyway (holding overnight will that is.) But anyway that's j an idea.

## Final quick notes

This is my masterpiece. So far anyway. My magnum opus. So... yeah.

## PS

The backtesting which verified the effectiveness of this algo is a little sketchy, but oh well got to live life on the edge
I guess. Who knows if it'll work, we'll c I suppose. If I lose all my money, I'll lose all my money, but I'll track it for sure.

## Protocls
––––
Protocol for in general receiving a call from ifttt:
-check dashboard immediately or very soon
-if not possible, call abort number, dial 1 + 000, and it will abort.

––––
Protocol for hikes (ADD TO README)
-turn on ifttt abort on emergency buy log log applet

–––
Assuming eventually emergency_buy_log is very rarely logged, then can turn on that ifttt applet indefinitely
–––
For when it actually works (extension/update to "for the future")
Instead of capping at 75k cap at a percentage of the smallest marketcap that we r going to short for the day. Best way to do This may be writing a seperate python script that takes in standard input that you call via a bash command via c++ piping in
standard input into the "python3 __progeramname__" comand. the python script writes market caps to seperate file. idk j an idea
