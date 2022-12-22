# Edison

An algo. The strategy – "VSEB" – which originally stood for "Volume Spike 
End of Day Buy" has gone through so many different iterations that now the 
actual strategy does not resemble this at all. In fact, it is now a 
relatively simple short strategy. I've backtested so many different 
variations of this strategy, with all sorts of different parameters, that 
I'm convinced this is close to if not the optimal variant. I have not a 
done a multiple regression to better test this hypothesis, however.

As mentioned earlier, this project changed so much and is so different 
from what it was at different stages, it would be impossible to even 
summarize such changes here. Instead, this is a summary of how the current 
strategy works. Going through past commits as well as looking at the 
commented out code can give you an insight into previous versions of the 
project.

## The strategy
The basic strategy is now simply that VSEB detects when a certain stock 
(whose price is less than 20 dollars) has a very modest (statistically 
speaking) volume increase on the day (only 1% of the running 6 month 
standard deviation above it's mean) but has a significant price increase 
(I think it's currently set for about 20% higher than yesterday close, or 
something around there anyway). We then short that stock with a stop loss 
of 1.5% and no take profit. Assuming the stock doesn't hit that stop loss 
price, we cover 5 market days later at end of day.

In order to avoid getting margin called over the maintenance margin amount 
alpaca demands (which can be very high) this strategy is actually a day 
trading strategy. Any asset we intend to hold "overnight" I simply sell 
(or cover in this case) at market close and re-buy (or re-short in this 
case) at market open. Although this means we miss out on after-hours and 
pre-market gains/losses this usually doesn't amount to much and actually 
makes the entire strategy much safer as the stop-losses will trigger (and 
usually execute) pretty quickly.

Further, there are various mechanisms in place to make sure this algorithm 
continues running at all costs and alerts me immediately should it 
encounter an error or is forced to stop running. The algo will 
automatically adjust to failed trades, cancelled orders, price spikes 
which exceed the speed at which we can place a stop loss, etc. I get a 
phone call, an email, and a text message should the algorithm encounter an 
error which it can fix, as well as when it encounters one it cannot fix 
itself. In the ladder case, however, it automatically liquidates 
everything and cancels all open orders before shutting itself off.

## Final Notes
Has only been tested with clang++ on mac and linux (ubuntu). Best way to 
compile is with bazel. Edison is the name of the entire project (inlcuding 
the program which backtests, the one that cleans and fetches the raw data, 
etc.), VSEB is this strategy specifically. Has only been backtested with 
data spanning  ~2018 – ~2020 due to data limitations and data accuracy 
limitations. Ophelia will be a much more robust and improved algo (in 
various respects) and is what I'm currently working on (have not pushed to 
a repo yet). VSEB is currently running (and placing trades) on
my personal server. **Since this wasn't originally intended to be 
published to a public repo, there is no security at all. My private api 
keys in this pushed version have been changed. The keys you see hard-coded 
in in the code are not the one's which are currently active to place 
trades on my brokerage account.** 
