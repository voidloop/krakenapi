#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include <chrono>
#include <thread>

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
// deals with Kraken trades:
struct KTrade {
   double price, volume; 
   time_t time; 
   char order;

   KTrade(JSONNode node) {
      price  = node[0].as_float();
      volume = node[1].as_float();
      time   = node[2].as_int();
      order  = node[3].as_string()[0];
   }
};

//------------------------------------------------------------------------------
// prints out a kraken trade:
ostream& operator<<(ostream& os, const KTrade& t) 
{
   return os << '"'
	     << t.time << "\",\""
	     << t.order << "\",\""
	     << fixed
	     << setprecision(5) << t.price << "\",\""
	     << setprecision(9) << t.volume << '"';
}

//------------------------------------------------------------------------------
// throws an runtime_error if there are errors in the JSON response 
void check_kraken_errors(const JSONNode& root)
{
   if (!root.at("error").empty()) {
      std::ostringstream oss;
      oss << "Kraken response contains errors: ";
      
      // append errors to output string stream
      for (JSONNode::const_iterator it = root["error"].begin(); 
	   it != root["error"].end(); ++it) 
	 oss << endl << " * " << libjson::to_std_string(it->as_string());
      
      throw runtime_error(oss.str());
   }
} 

//------------------------------------------------------------------------------
// downloads recent trades:
string recent_trades(const KAPI& k, const KAPI::Input& i, vector<KTrade>& v)
{
   string json_data = k.public_method("Trades", i);
   JSONNode root = libjson::parse(json_data);
   //cout << json_data << endl;

   // throw an exception if there are errors in the JSON response
   check_kraken_errors(root);

   // throw an exception if result is empty   
   if (root.at("result").empty()) {
      throw runtime_error("Kraken response doesn't contain result data");
   }

   const string& pair = i.at("pair");

   JSONNode& result = root["result"];
   JSONNode& result_pair = result.at(pair);

   vector<KTrade> output;
   for (JSONNode::iterator it = result_pair.begin(); 
	it != result_pair.end(); ++it)
      output.push_back(KTrade(*it));
      
   output.swap(v);
   return libjson::to_std_string( result.at("last").as_string() );
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input input;
      
      std::chrono::seconds dura(30);
      std::vector<KTrade> trades;

      input["pair"] = "XXBTZEUR";
      input["since"] = "0";

      while (true) {
	 // store and print trades
	 const string last = recent_trades(kapi, input, trades);
	 for (int i = 0; i < trades.size(); ++i) 
	    cout << trades[i] << endl;
	    
	 // next "since" is the "last" value
	 input["since"] = last;
	 
	 // sleep
	 std::this_thread::sleep_for(dura);
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
