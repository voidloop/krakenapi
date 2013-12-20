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
// deals with Trades:
struct Trade {
   double price, volume; 
   time_t time; 
   char order;
};

//------------------------------------------------------------------------------
// prints a Trade
ostream& operator<<(ostream& os, const Trade& t) 
{
   struct tm timeinfo;
   gmtime_r(&t.time, &timeinfo);
   
   char buffer[20];
   strftime(buffer, 20, "%T", &timeinfo);

   return os << buffer << ','
	     << t.order << ','
	     << fixed
	     << setprecision(5) << t.price << ','
	     << setprecision(9) << t.volume;
}

//------------------------------------------------------------------------------
// helper function to load a Trade from a JSONNode:
Trade get_trade(const JSONNode& node) 
{
   Trade t; 
   t.price  = node[0].as_float();
   t.volume = node[1].as_float();
   t.time   = node[2].as_int();
   t.order  = node[3].as_string()[0];
   return t;
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

   // create a HA candlestick from a period of trades 
   // WITHOUT a prior HA candlestick
   Candlestick(time_t t, const Period& p) 
      : time(t), volume(0)
   {
      double p_open  = p.front().price;
      double p_close = p.back().price;
      init(p, p_open, p_close); 
   }

   // create a HA candlestick from a period of trades
   // WITH a prior HA candlestick
   Candlestick(time_t t, const Period& p, const Candlestick& prior) 
      : time(t), volume(0)
   { 
      // initialize using prior HA candlestick's open-close values
      init(p, prior.open, prior.close); 
   }

private:
   // initialize the HA candlestick
   void init(const Period& p, double ha_open, double ha_close) {

      // return if the Period is empty
      if (p.empty()) return;

      // initialize period values
      double p_open   = p.front().price;
      double p_close  = p.back().price;
      double p_low    = min(p_open, p_close);
      double p_high   = max(p_open, p_close);

      // find low price and high price of the current period
      for(Period::const_iterator it = p.begin(); it != p.end(); ++it) {
	 if (it->price < p_low) p_low = it->price;
	 if (it->price > p_high) p_high = it->price;
	 volume += it->volume;
      }

      // compute Heikin-Ashi values
      close = (p_open + p_close + p_low + p_high) / 4;
      open  = (ha_open + ha_close) / 2; 
      low   = min(p_low, min(open, close));
      high  = max(p_high, max(open, close));
   }

};

//------------------------------------------------------------------------------
// prints out a Candlestick
ostream& operator<<(ostream& os, const Candlestick& c) 
{
   struct tm timeinfo;
   localtime_r(&c.time, &timeinfo);
   
   char buffer[20];
   strftime(buffer, 20, "%T", &timeinfo);

   return os << buffer << ','
	     << fixed << setprecision(5) 
	     << c.open << ','
	     << c.high << ','
	     << c.low << ','
	     << c.close << ','
	     << setprecision(9) 
	     << c.volume;
}

//------------------------------------------------------------------------------
// creates candlesticks from a Period_map
void append_candlesticks(const Period_map& pm, vector<Candlestick>& c) 
{
   // if there are no periods do nothing
   if (pm.empty()) return;
   
   Period_map::const_iterator it = pm.begin();

   if (c.empty())
      c.push_back(Candlestick(it->first, it->second));
      
   for (++it; it != pm.end(); ++it)
      c.push_back(Candlestick(it->first, it->second, c.back()));
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input in;

      // get recent trades 
      in.insert(make_pair("pair", "XLTCZEUR"));

      string json_trades = kapi.public_method("Trades", in); 
      JSONNode root = libjson::parse(json_trades);

      // check for errors, if they're present an 
      // exception will be thrown
      if (!root.at("error").empty()) {
	 ostringstream oss;
	 oss << "Kraken response contains errors: ";

	 // append errors to output string stream
	 for (JSONNode::iterator it = root["error"].begin(); 
	      it != root["error"].end(); ++it) 
	    oss << endl << " * " << libjson::to_std_string(it->as_string());
	 
	 throw runtime_error(oss.str());
      }
      else {
	 // format the output in columns: time, order, price and volume
	 JSONNode result = root.at("result")[0];
	 
	 time_t step = 3600;
	 Period_map periods;

	 // group results by base time
	 for (JSONNode::const_iterator it = result.begin();
	      it != result.end(); ++it) {
	    Trade t = get_trade(*it);
	    time_t x = t.time - (t.time % step);
	    periods[x].push_back(t);
	 }
	
	 vector<Candlestick> candlesticks;

	 // create candlesticks
	 append_candlesticks(periods, candlesticks);

	 // print candlesticks
	 for (int i = 0; i<candlesticks.size(); ++i)
	    cout << candlesticks[i] << endl;
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
