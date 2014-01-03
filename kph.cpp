/*

  Kraken Price Histoty (kph) is program to download from kraken.com
  the trade data of the last 24 hours and to display it as 
  Heikin-Ashi candlestiks.
  The candlestiks are printed out in CSV format to stdout as follows:
  
    date,open,high,low,close
    
  date is the candlestick's time period in Unix timestamp format.
  kph take also command line arguments to modify its behavior:
  
    kph <pair> [seconds] [last]

  where: 

    <pair>    - it's the pair to download from kraken.com
    [seconds] - (optional) the seconds of the period (by default 15*60)
    [last]    - (optional) the last seconds to consider from the last 
                trade (by default 24*60*60) 
  
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include "kapi.hpp"
#include "libjson/libjson.h"

using namespace std;
using namespace Kraken;

//------------------------------------------------------------------------------
// deals with Kraken trades:
struct Trade {
   double price, volume; 
   time_t time; 
   char order;

   Trade(JSONNode node) {
      price  = node[0].as_float();
      volume = node[1].as_float();
      time   = node[2].as_int();
      order  = node[3].as_string()[0];
   }
};

//------------------------------------------------------------------------------
// prints out a kraken trade:
ostream& operator<<(ostream& os, const Trade& t) 
{
   return os << '"'
	     << t.time << "\",\""
	     << t.order << "\",\""
	     << fixed
	     << setprecision(5) << t.price << "\",\""
	     << setprecision(9) << t.volume << '"';
}

//------------------------------------------------------------------------------
// helper types to map time to a group of trades:
typedef vector<Trade> Period; 
typedef map<time_t,Period> Period_map;

//------------------------------------------------------------------------------
// deal with candlesticks:
struct Candlestick {
   double open, close, low, high;
   double volume;
   time_t time;
};

//------------------------------------------------------------------------------
struct HA_Candlestick : public Candlestick {

   // create a candlestick from current period
   HA_Candlestick(const Candlestick& curr)
   {
      time = curr.time;
      volume = curr.volume;
      close =  (curr.open + curr.close + curr.low + curr.high) / 4;
      open = (curr.open + curr.close) / 2;
      low = min(curr.low, min(open, close)); 
      high = max(curr.high, max(open, close));
   }

   // create a HA candlestick from current period
   // and WITH a prior HA candlestick
   HA_Candlestick(const Candlestick& curr, const HA_Candlestick& prior) 
   {
      time = curr.time;
      volume = curr.volume;
      close =  (curr.open + curr.close + curr.low + curr.high) / 4;
      open = (prior.open + prior.close) / 2;
      low = min(curr.low, min(open, close)); 
      high = max(curr.high, max(open, close));
   }
};

//------------------------------------------------------------------------------
// prints out a Candlestick
ostream& operator<<(ostream& os, const Candlestick& c) 
{
   struct tm timeinfo;
   localtime_r(&c.time, &timeinfo);
   
   return os << c.time << ','
	     << fixed << setprecision(5) 
	     << c.open << ','
	     << c.high << ','
	     << c.low << ','
	     << c.close << ','
	     << setprecision(9) 
	     << c.volume;
}

//------------------------------------------------------------------------------
// downloads recent trades:
string recent_trades(const KAPI& k, const KAPI::Input& i, vector<Trade>& v)
{
   string json_data = k.public_method("Trades", i);
   JSONNode root = libjson::parse(json_data);
   //cout << json_data << endl;

   // throw an exception if there are errors in the JSON response
   if (!root.at("error").empty()) {
      std::ostringstream oss;
      oss << "Kraken response contains errors: ";
      
      // append errors to output string stream
      for (JSONNode::const_iterator
	      it = root["error"].begin(); it != root["error"].end(); ++it) 
	 oss << endl << " * " << libjson::to_std_string(it->as_string());
      
      throw runtime_error(oss.str());
   }

   // throw an exception if result is empty   
   if (root.at("result").empty()) {
      throw runtime_error("Kraken response doesn't contain result data");
   }

   const string& pair = i.at("pair");

   JSONNode& result = root["result"];
   JSONNode& result_pair = result.at(pair);

   vector<Trade> output;
   for (JSONNode::const_iterator 
	   it = result_pair.begin(); it != result_pair.end(); ++it)
      output.push_back(Trade(*it));
      
   output.swap(v);
   return libjson::to_std_string( result.at("last").as_string() );
}

//------------------------------------------------------------------------------
// fills a candlestick vector grouping trades by time:
void group_by_time(const vector<Trade>& trades, 
		   const time_t step, 
		   vector<Candlestick>& candlesticks)
{
   vector<Trade>::const_iterator it = trades.begin();

   while (it != trades.end()) {
      Candlestick period;	 
      period.volume = 0;
      period.open = it->price;
      period.low = it->price;
      period.high = it->price;
      
      // the period time
      period.time = it->time - (it->time % step); 
      
      while (it != trades.end() && it->time < (period.time+step)) {
	 // the lowest price
	 if (it->price < period.low) 
	    period.low = it->price;

	 // the highest price
	 if (it->price > period.high) 
	    period.high = it->price;

	 // sum volumes
	 period.volume += it->volume;

	 // last price is close time
	 period.close = it->price;
	 
	 // next element
	 it++;
      }

      // store period
      candlesticks.push_back(period);
      
      // next group
   }  
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) 
{ 
   try {
      time_t step = 15*60; // by default 15 minutes
      time_t last = 24*60*60; // by default last 24 hours

      // 
      // usage: kph <pair> [seconds] [last]
      //
      // kph prints out the price history of the <pair> in 
      // the [last] number of seconds. The trade data is 
      // showed as candlesticks grouped in periods of 
      // [seconds] seconds.
      //

      KAPI::Input input;
      input["since"] = "0";

      switch (argc) {
      case 4: 
	 istringstream(argv[3]) >> last;
      case 3:
	 istringstream(argv[2]) >> step;
      case 2:
	 input["pair"] = std::string(argv[1]);
	 break;
      default:
	 throw std::runtime_error("wrong number of arguments");
      };

      // initialize kraken lib's resources:
      Kraken::initialize();
      KAPI kapi;

      std::vector<Trade> trades;
      std::vector<Candlestick> candlesticks;

      recent_trades(kapi, input, trades);

      // group trades by time
      group_by_time(trades, step, candlesticks);
      
      if (!candlesticks.empty()) {
	 // print candlestick after this threshold
	 time_t thresh = candlesticks.back().time - last;

	 std::vector<Candlestick>::const_iterator it = candlesticks.begin();
	 HA_Candlestick ha(*it);
	 if (ha.time > thresh) 
	    cout << ha << endl;
	 
	 for (++it; it != candlesticks.cend(); ++it) {	   
	    ha = HA_Candlestick(*it, ha);
	    if (ha.time >= thresh) 
	       cout << ha << endl;
	 }
      }
   }
   catch(exception& e) {
      cerr << "Error: " << e.what() << endl;
      return 1;
   }
   catch(...) {
      cerr << "Unknow exception." << endl;
      return 1;
   }

   curl_global_cleanup();
   return 0;
}
