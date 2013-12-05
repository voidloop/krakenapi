#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <map>
#include <ctime>
#include <cerrno>
#include <curl/curl.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>

using namespace std;

//------------------------------------------------------------------------------

class KAPI {
public:  
   // helper type to make requests
   typedef std::map<std::string,std::string> Input;

   // constructor with all explicit parameters
   KAPI(const string& key, const string& secret, 
	const string& url, const string& version);

   // default API base URL and API version
   KAPI(const string& key, const string& secret);
   
   // constructor with empty API key and API secret
   KAPI();

   // distructor
   ~KAPI();

   // makes public method to kraken.com
   void publicMethod(const string& method, const KAPI::Input& input) const;

   // makes private method to kraken.com
   void privateMethod(const string& method, const KAPI::Input& input) const;

   // TODO: public market data
   //void time();
   //void assets();

private:
   // init CURL and other stuffs
   void init(); 

   // helper function to build a query string from KAPI::Input
   static string buildQuery(const KAPI::Input&);

   // helper private functions: 
   static string nonce();                   // generates a nonce
   static string sha256(const string&);     // hashs with SHA256
   static string b64dec(const string&);     // decodes base64 strings 
   static string hmac(const string&, const string&);  // hmac function (SHA512)


   string key_;     // API key
   string secret_;  // API secret
   string url_;     // API base URL
   string version_; // API version
   CURL*  curl_;    // CURL handle
};

//------------------------------------------------------------------------------
// constructor with all explicit parameters
KAPI::KAPI(const string& key, const string& secret, 
	   const string& url, const string& version)
   :key_(key), secret_(secret), url_(url), version_(version) 
{ 
   init(); 
}

//------------------------------------------------------------------------------
// default API base URL and API version
KAPI::KAPI(const string& key, const string& secret)
   :key_(key), secret_(secret), url_("https://api.kraken.com"), version_("0") 
{ 
   init(); 
}

//------------------------------------------------------------------------------
// constructor with empty API key and API secret
KAPI::KAPI() 
   :key_(""), secret_(""), url_("https://api.kraken.com"), version_("0") 
{ 
   init(); 
}

//------------------------------------------------------------------------------
// initializes libcurl:
void KAPI::init()
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
// distructor:
KAPI::~KAPI() 
{
   curl_easy_cleanup(curl_);
}

//------------------------------------------------------------------------------
// builds a query string from KAPI::Input:
string KAPI::buildQuery(const KAPI::Input& input)
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
// helper function to generate a nonce:
string KAPI::nonce()
{
   ostringstream oss;

   timeval tp;
   if (gettimeofday(&tp, NULL) != 0) {
      oss << "gettimeofday() failed: "
	  << strerror(errno); 
      throw runtime_error(oss.str());
   }
   else {
      // format output string 
      oss << std::setfill('0') 
	  << std::setw(10) << tp.tv_sec 
	  << std::setw(6)  << tp.tv_usec;
   }
   return oss.str();
} 

//------------------------------------------------------------------------------
// helper function to calculate SHA256:
string KAPI::sha256(const string& s)
{
   unsigned char result[SHA256_DIGEST_LENGTH];
   SHA256_CTX ctx;

   SHA256_Init(&ctx);
   SHA256_Update(&ctx, s.c_str(), s.size());
   SHA256_Final(result, &ctx);

   // create digest string
   ostringstream oss;
   for (int i=0; i<SHA256_DIGEST_LENGTH; ++i)
      oss << hex << setfill('0') << setw(2) << (unsigned int) result[i]; 

   return oss.str();
}

//------------------------------------------------------------------------------
// helper function to decode an ecoded Base64 string:
string KAPI::b64dec(const string& s) 
{
   BIO* b64 = BIO_new(BIO_f_base64());
   BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

   BIO* bmem = BIO_new_mem_buf((void*)s.c_str(), s.length());
   bmem = BIO_push(b64, bmem);
   
   char buffer[s.length() + 1];
   int decodedSize = BIO_read(bmem, buffer, s.length());
   BIO_free_all(bmem);

   if (decodedSize < 0)
      throw runtime_error("failed while decoding base64.");

   return string(buffer, decodedSize);
}

//------------------------------------------------------------------------------
// helper function to hash with HMAC hashing
string KAPI::hmac(const string& data, const string& key)
{
   unsigned char result[EVP_MAX_MD_SIZE];
   
   HMAC_CTX ctx;
   HMAC_CTX_init(&ctx);

   HMAC_Init_ex(&ctx, key.c_str(), key.length(), EVP_sha512(), NULL);
   HMAC_Update(&ctx, (unsigned char*) data.c_str(), data.length());
   
   unsigned int len;
   HMAC_Final(&ctx, result, &len);
   HMAC_CTX_cleanup(&ctx);

   // create digest string
   ostringstream oss;
   for (unsigned int i=0; i<len; ++i)
      oss << hex << setfill('0') << setw(2) << (unsigned int) result[i]; 

   return oss.str();
}

//------------------------------------------------------------------------------
// deals with public API methods:
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
// deals with private API methods:
void KAPI::privateMethod(const string& method, 
			const KAPI::Input& input) const
{   
   // create path
   string path = "/" + version_ + "/public/" + method;
   string postdata = buildQuery(input);

   // generate message signature
   string sign = hmac(path + sha256(nonce() + postdata), b64dec(secret_)); 

    // build method URL and make request
   string methodUrl = url_ + path;
   cout << methodUrl << endl;
   curl_easy_setopt(curl_, CURLOPT_URL, methodUrl.c_str());
   cout << buildQuery(input) << endl;

   curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, buildQuery(input).c_str());
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, NULL);

   CURLcode result = curl_easy_perform(curl_);
   if (result != CURLE_OK) {
      ostringstream oss;
      oss << "curl_easy_perform() failed: "  
	  << curl_easy_strerror(result);
      throw runtime_error( oss.str() );
   }
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input in; 

      in.insert(make_pair("pair", "XXBTZUSD,XXBTXLTC"));
      kapi.privateMethod("Ticker", in);
   }
   catch(std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
   }
   catch(...) {
      std::cerr << "Unknow exception." << std::endl;
   }

   curl_global_cleanup();
   return 0;
}
