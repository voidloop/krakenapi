#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include <chrono>
#include <thread>

#include "kraken/kclient.hpp"
#include "libjson/libjson.h"

using namespace std;
using namespace Kraken;

//------------------------------------------------------------------------------
// deals with Kraken assets:
struct KAsset {
   string name;
   string altname;
   string aclass;
   int decimals;
   int display_decimals;

   // construct a KAssets from an JSONNode:
   KAsset(const JSONNode& node) {
      name = libjson::to_std_string(node.name());
      altname = node["altname"].as_string();
      aclass = node["aclass"].as_string();
      decimals = node["decimals"].as_int();
      display_decimals = node["display_decimals"].as_int();
   }
};

//------------------------------------------------------------------------------
// prints a KAsset:
ostream& operator<<(ostream& os, const KAsset& a)
{
   return os << '"' 
	     << a.name << "\",\""
	     << a.altname << "\",\""
	     << a.aclass << "\",\""
	     << a.decimals << "\",\""
	     << a.display_decimals 
	     << '"';
}

//------------------------------------------------------------------------------
// deals with Kraken assets:
struct KAsset_pair {
   string name;
   string altname;
   string aclass_base;
   string base;
   string aclass_quote; 
   string quote;
   string lot;
   int pair_decimals;
   int lot_decimals;
   int lot_multiplier;
   // leverage ...
   string fees; 
   string fee_volume_currency;
   int margin_call; 
   int margin_stop;

   // construct a KAssets from an JSONNode:
   KAsset_pair(const JSONNode& node) {
      name = libjson::to_std_string( node.name() );
      altname = libjson::to_std_string( node["altname"].as_string() );
      aclass_base = libjson::to_std_string( node["aclass_base"].as_string() );
      base = libjson::to_std_string( node["base"].as_string() );
      aclass_quote = libjson::to_std_string( node["aclass_quote"].as_string() );
      quote = libjson::to_std_string( node["quote"].as_string() );
      lot = libjson::to_std_string( node["lot"].as_string() );

      pair_decimals = node["pair_decimals"].as_int();
      lot_decimals = node["lot_decimals"].as_int();
      lot_multiplier = node["lot_multiplier"].as_int();
      fees = libjson::to_std_string( node["fees"].as_string() );
      
      fee_volume_currency 
	 = libjson::to_std_string( node["fee_volume_currency"].as_string() );
      
      margin_call = node["margin_call"].as_int();
      margin_stop = node["margin_stop"].as_int();
   }
};

//------------------------------------------------------------------------------
// prints a KAsset_pair:
ostream& operator<<(ostream& os, const KAsset_pair& a)
{
   return os << '"' 
	     << a.name << "\",\""
	     << a.altname << "\",\""
	     << a.aclass_base << "\",\""
	     << a.base << "\",\""
	     << a.aclass_quote << "\",\""
	     << a.quote << "\",\""
	     << a.lot << "\",\""
	     << a.pair_decimals << "\",\""
	     << a.lot_decimals << "\",\""
	     << a.lot_multiplier << "\",\""
	     << a.fees << "\",\""
	     << a.fee_volume_currency << "\",\""
	     << a.margin_call << "\",\""
	     << a.margin_stop 
	     << '"';
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) 
{
   try {

      // initialize kraken lib's resources:
      Kraken::initialize();

      //
      // command line argument handling:
      //
      // usage:
      //     krt <pair> [interval] [since]
      // 

      string pair;
      string last = "0"; // by default: the oldest possible trade data
      int interval = 0;   // by default: krt exits after download trade data

      switch (argc) {
      case 4:
	 last = string(argv[3]);
      case 3:
	 istringstream(argv[2]) >> interval;      
      case 2:
	 pair = string(argv[1]);
	 break;
      default: 
	 throw runtime_error("wrong number of arguments");
      };
      
      chrono::seconds dura(interval);

      KClient kc;
      vector<KTrade> vt;

      while (true) {
	 // store and print trades
	 last = kc.trades(pair, last, vt);
	 for (int i = 0; i < vt.size(); ++i) 
	    cout << vt[i] << endl;
	    
	 // exit from the loop if interval is 0
	 if (interval == 0) break;
	 
	 // sleep
	 this_thread::sleep_for(dura);
      }

      // terminate kraken lib's resources
      Kraken::terminate();
   }
   catch(exception& e) {
      cerr << "Error: " << e.what() << endl;
      exit(EXIT_FAILURE);
   }
   catch(...) {
      cerr << "Unknow exception." << endl;
      exit(EXIT_FAILURE);
   }

   return 0;
}
