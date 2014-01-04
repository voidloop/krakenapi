#include <iomanip>
#include "ktrade.hpp"

//------------------------------------------------------------------------------

namespace Kraken { 

//------------------------------------------------------------------------------
// construct from a JSONNode:
KTrade::KTrade(JSONNode node) 
{
   price  = node[0].as_float();
   volume = node[1].as_float();
   std::istringstream( libjson::to_std_string(node[2].as_string()) ) >> time;
   
   order = static_cast<Order_t>
      ( libjson::to_std_string(node[3].as_string())[0] );
   otype = static_cast<Otype_t>
      ( libjson::to_std_string(node[4].as_string())[0] );
   
   misc = libjson::to_std_string(node[5].as_string()); 
}

//------------------------------------------------------------------------------
// prints out a KTrade:
std::ostream& operator<<(std::ostream& os, const KTrade& kt)
{
   return os << '"' << kt.time << "\",\""
	     << static_cast<char>(kt.order) << "\",\""
	     << static_cast<char>(kt.otype) << "\",\""
	     << std::fixed
	     << std::setprecision(5) << kt.price << "\",\""
	     << std::setprecision(9) << kt.volume << '"';
}

//------------------------------------------------------------------------------

}; // namespace Kraken
