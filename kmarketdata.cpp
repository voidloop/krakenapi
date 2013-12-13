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
	 
	 for (JSONNode::const_iterator it = result_node.begin();
	      it != result_node.end(); ++it) {
	    const double price  = (*it)[0].as_float();
	    const double volume = (*it)[1].as_float();
	    const time_t time   = (*it)[2].as_int();
	    const char order    = (*it)[3].as_string()[0];

	    struct tm timeinfo;
	    localtime_r(&time, &timeinfo);

	    char buffer[20];
	    strftime(buffer, 20, "%T", &timeinfo);

	    cout << left << setw(12) << buffer << "   "
		 << left << setw(4) << (order=='b'?"buy":"sell") << "   " 
		 << fixed << right 
		 << setprecision(5) << price << "   " 
		 << setprecision(9) << volume << endl;
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
