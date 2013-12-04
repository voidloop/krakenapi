#include <iostream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <curl/curl.h>

using namespace std;

//------------------------------------------------------------------------------

class KAPI {
public:  
   // helper types to make requests
   typedef std::map<std::string,std::string> Input;

   KAPI();
   ~KAPI();

   // makes public method to kraken.com
   void publicMethod(const string&, const KAPI::Input&) const;

   // makes private method to kraken.com
   void privateMethod(const string&, const KAPI::Input&) const;

private:
   // builds a query string from KAPI::Input
   string buildQuery(const KAPI::Input&) const;

   CURL *curl_;
   string url_;
   string version_;
};

//------------------------------------------------------------------------------

// initializes libcurl with kraken.com infos:
KAPI::KAPI() 
   :url_("https://api.kraken.com"), version_("0")
{
   curl_ = curl_easy_init();
   if (curl_) {
      curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
      curl_easy_setopt(curl_, CURLOPT_USERAGENT, "Kraken C++ API Client");
      curl_easy_setopt(curl_, CURLOPT_POST, 1L);
   }
   else 
      throw runtime_error("can't create curl handle");
}

//------------------------------------------------------------------------------

KAPI::~KAPI() 
{
   curl_easy_cleanup(curl_);
}

//------------------------------------------------------------------------------
// builds a query string from KAPI::Input:
string KAPI::buildQuery(const KAPI::Input& input) const
{
   ostringstream oss;
   KAPI::Input::const_iterator it = input.begin();
   for (; it != input.end(); ++it) {
      if (it != input.begin()) oss << '&';  // delimiter
      oss << it->first <<'='<< it->second;
   }

   return oss.str();
}

//------------------------------------------------------------------------------

void KAPI::publicMethod(const string& method, 
			const KAPI::Input& input) const
{
   CURLcode res;
   ostringstream oss;

   // build method URL and make request
   oss << url_ << '/' << version_ << "/public/" << method;
   string methodUrl = oss.str();
   cout << methodUrl.c_str() << endl;
   curl_easy_setopt(curl_, CURLOPT_URL, methodUrl.c_str());
   cout << buildQuery(input) << endl;

   curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, buildQuery(input).c_str());
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, NULL);

   res = curl_easy_perform(curl_);
   if (res != CURLE_OK) {
      oss.clear();  
      oss << "curl_easy_perform() failed: "  
	  << curl_easy_strerror(res);
      throw runtime_error( oss.str() );
   }
}

//------------------------------------------------------------------------------

void KAPI::privateMethod(const string& method, 
			 const KAPI::Input& input) const
{
   CURLcode res;
   ostringstream oss;

   // build method URL and make request
   oss << url_ << '/' << version_ << "/public/" << method;
   string methodUrl = oss.str();
   cout << methodUrl.c_str() << endl;
   curl_easy_setopt(curl_, CURLOPT_URL, methodUrl.c_str());
   cout << buildQuery(input) << endl;

   curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, buildQuery(input).c_str());
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, NULL);

   res = curl_easy_perform(curl_);
   if (res != CURLE_OK) {
      oss.clear();  
      oss << "curl_easy_perform() failed: "  
	  << curl_easy_strerror(res);
      throw runtime_error( oss.str() );
   }
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input input; 

      input.insert(make_pair("pair", "XXBTZUSD,XXBTXLTC"));
      kapi.publicMethod("Ticker", input);
   }
   catch(std::exception& e) {
      std::cerr << "error: " << e.what() << std::endl;
   }
   catch(...) {
      std::cerr << "unknow exception" << std::endl;
   }

   curl_global_cleanup();
}
