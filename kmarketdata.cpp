#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include "kapi.hpp"
#include "libjson/libjson.h"

using namespace std;
using namespace Kraken;

//------------------------------------------------------------------------------
// deals with Candlesticks
struct Record {
   vector<double> prices;
   double volume;
   time_t timestamp;
   
   Record(time_t t) 
      :timestamp(t) {}
   
   // add a record
   void add(double p, double v) { 
      prices.push_back(p); 
      volume += v;
   }

   double low() const { return *min_element(prices.begin(), prices.end()); }
   double high() const { return *max_element(prices.begin(), prices.end()); }
   
   double open() const { return prices.front(); }
   double close() const { return prices.back(); }

private:
   // disable default constructor
   Record();
};

//------------------------------------------------------------------------------
// prints out a vector of records:
void print_records(const vector<Record>& records) 
{
   double ha_open, ha_close, ha_low, ha_high;  

   // print out records
   for (int i=0; i<records.size(); ++i) {
      // http://stockcharts.com/help/doku.php?id=chart_school:
      // chart_analysis:heikin_ashi

      // The Heikin-Ashi Open is the average of the prior Heikin-Ashi 
      // candlestick open plus the close of the prior Heikin-Ashi 
      // candlestick.
      if (i == 0)
	 ha_open = (records[0].open() + records[0].close()) / 2; 
      else
	 ha_open = (ha_open + ha_close) / 2;

      // The Heikin-Ashi Close is simply an average of the open, 
      // high, low and close for the current period. 
      ha_close = (records[i].open() + records[i].high() + 
		  records[i].low() + records[i].close()) / 4;

      // The Heikin-Ashi High is the maximum of three data points: 
      // the current period's high, the current Heikin-Ashi 
      // candlestick open or the current Heikin-Ashi candlestick close.
      ha_high = max(records[i].high(), max(ha_open, ha_close));

      // The Heikin-Ashi low is the minimum of three data points: 
      // the current period's low, the current Heikin-Ashi 
      // candlestick open or the current Heikin-Ashi candlestick close.
      ha_low = min(records[i].low(), min(ha_open, ha_close));

      //struct tm timeinfo;
      //localtime_r(&records[i].timestamp, &timeinfo);
	    
      //char buffer[20];
      //strftime(buffer, 20, "%T %z", &timeinfo);
      cout << records[i].timestamp << "   " 
	   << fixed << setprecision(9) 
	   << records[i].volume << "   "
	   << fixed << right << setprecision(5)
	   << ha_open  << "   " 
	   << ha_low   << "   "
	   << ha_high  << "   " 
	   << ha_close << endl;
   } 
	 
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input in;

      // get recent trades 
      in.insert(make_pair("pair", "XXBTZEUR"));

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
	 JSONNode result_node = root.at("result")[0];
	 
	 JSONNode::const_iterator it = result_node.begin(); 
	 time_t last_hour = 0;

	 vector<Record> records;

	 // now group prices by hour
	 for (; it != result_node.end(); ++it) {
	    const double price  = (*it)[0].as_float();
	    const double volume = (*it)[1].as_float();
	    const time_t time   = (*it)[2].as_int();
	    const char order    = (*it)[3].as_string()[0];

	    // a new group if needed
	    time_t hour = time - time % 3600;
	    if (hour != last_hour) {
	       last_hour = hour;
	       records.push_back(Record(hour));
	    }
	     
	    records.back().add(price, volume);
	 }

	 print_records(records);
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
