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

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;

      string json_data = kapi.public_method("AssetPairs", KAPI::Input()); 
      JSONNode root = libjson::parse(json_data);

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

	 JSONNode result = root.at("result");
	 
	 for (JSONNode::iterator it = result.begin(); 
	      it != result.end(); ++it)
	 {
	    cout << KAsset_pair(*it) << endl;
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
