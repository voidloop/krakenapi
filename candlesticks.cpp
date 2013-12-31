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
      for (auto it = root["error"].begin(); it != root["error"].end(); ++it) 
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
   for (JSONNode::iterator it = result_pair.begin(); 
	it != result_pair.end(); ++it)
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
   for (int open=0; open < trades.size();) {	 
      Candlestick period;	 
      period.volume = 0;
      period.open = trades[open].price;
      period.low = trades[open].price;
      period.high = trades[open].price;

      // the period time
      period.time = trades[open].time - (trades[open].time % step); 

      int close = open;
      for (; close < trades.size() && trades[close].time < (period.time+step); ++close) {
	 // the lowest price
	 if (trades[close].price < period.low) 
	    period.low = trades[close].price;

	 // the highest price
	 if (trades[close].price > period.high) 
	    period.high = trades[close].price;

	 // sum volumes
	 period.volume += trades[close].volume; 
      }
      
      // the last trade of the period
      period.close = trades[close-1].price;
      
      // store period
      candlesticks.push_back(period);

      // go to next period
      open = close;
   }
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) 
{ 
   try {
      time_t step = 600; // by default 10 minutes

      switch (argc) {
      case 2:
	 istringstream(argv[1]) >> step;
	 break;
      default:
	 throw std::runtime_error("wrong number of arguments");
      };

      // initialize kraken lib's resources:
      Kraken::initialize();
      
      KAPI kapi;
      KAPI::Input input;

      // get recent trades 
      input["pair"] = "XLTCZEUR";
      input["since"] = "0";

      std::vector<Trade> trades;
      std::vector<Candlestick> candlesticks;

      string last = recent_trades(kapi, input, trades);

      // group trades by time
      group_by_time(trades, step, candlesticks);
      
      if (!candlesticks.empty()) {
	 auto it = candlesticks.cbegin();
	 HA_Candlestick ha(*it);
	 cout << ha << endl;
	 
	 for (++it; it != candlesticks.cend(); ++it) {
	    ha = HA_Candlestick(*it, ha);
	    cout << ha << endl;
	 }
      }
   }
   catch(exception& e) {
      cerr << "Error: " << e.what() << endl;
   }
   catch(...) {
      cerr << "Unknow exception." << endl;
   }

   curl_global_cleanup();
   return 0;
}
