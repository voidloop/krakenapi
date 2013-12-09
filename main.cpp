#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <ctime>
#include <cerrno>
#include <curl/curl.h>

#include <openssl/buffer.h>
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

   // create signature for private requests
   void messageSignature(const string& path, const string& nonce, 
			 const string& postdata, string& sign) const;

   // helper function to build a query string from KAPI::Input
   static string buildQuery(const KAPI::Input&);

   // creates a nonce
   static string createNonce(); 

   // functions to decode and encode string with base64
   static void b64dec(const string& input, vector<unsigned char>& output);
   static void b64enc(const vector<unsigned char>& input, string& output);
   
   // functions to encrypt data
   static void sha256(const string& input, vector<unsigned char>& output);
   static void hmac(const vector<unsigned char>& input, 
		    const vector<unsigned char>& key, 
		    vector<unsigned char>& output);

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
// helper function to create a nonce:
string KAPI::createNonce()
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
// helper function to compute SHA256:
void KAPI::sha256(const string& input, vector<unsigned char>& output)
{
   vector<unsigned char> digest(SHA256_DIGEST_LENGTH);

   SHA256_CTX ctx;
   SHA256_Init(&ctx);

   SHA256_Update(&ctx, input.c_str(), input.length());
   SHA256_Final(digest.data(), &ctx);

   output.swap(digest);  
}

//------------------------------------------------------------------------------
// helper function to decode a base64 string to a vector of bytes:
void KAPI::b64dec(const string& input, vector<unsigned char>& output) 
{
   BIO* b64 = BIO_new(BIO_f_base64());
   BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

   BIO* bmem = BIO_new_mem_buf((void*)input.c_str(), input.length());
   bmem = BIO_push(b64, bmem);
   
   vector<unsigned char> result(input.length());
   int decodedSize = BIO_read(bmem, result.data(), result.size());
   BIO_free_all(bmem);

   if (decodedSize < 0)
      throw runtime_error("failed while decoding base64.");
   
   output.swap(result);
   output.resize(decodedSize);
}

//------------------------------------------------------------------------------
// helper function to encode a vector of bytes to a base64 string:
void KAPI::b64enc(const vector<unsigned char>& input, string& output) 
{
   BIO* b64 = BIO_new(BIO_f_base64());
   BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

   BIO* bmem = BIO_new(BIO_s_mem());
   b64 = BIO_push(b64, bmem);
   
   BIO_write(b64, input.data(), input.size());
   BIO_flush(b64);

   BUF_MEM* bptr = NULL;
   BIO_get_mem_ptr(b64, &bptr);
   
   string result(bptr->data, bptr->length);
   BIO_free_all(b64);

   output.swap(result);
}

//------------------------------------------------------------------------------
// helper function to hash with HMAC algorithm:
void KAPI::hmac(const vector<unsigned char>& input, 
		const vector<unsigned char>& key,
		vector<unsigned char>& output)
{   
   unsigned int len = EVP_MAX_MD_SIZE;
   vector<unsigned char> digest(len);

   HMAC_CTX ctx;
   HMAC_CTX_init(&ctx);

   HMAC_Init_ex(&ctx, key.data(), key.size(), EVP_sha512(), NULL);
   HMAC_Update(&ctx, input.data(), input.size());
   HMAC_Final(&ctx, digest.data(), &len);
   
   HMAC_CTX_cleanup(&ctx);
   
   output.swap(digest);
}

//------------------------------------------------------------------------------
// creates message signature in 'sign' generated from a URI path, a nonce 
// and postdata, message signature is created as a follows:
// 
//   hmac_sha512(path + sha256(nonce + postdata), b64decode(secret)) 
//
// and the result is converted in a base64 string: 
void KAPI::messageSignature(const string& path, const string& nonce, 
			    const string& postdata, string& sign) const
{
   // add path to data to encrypt
   vector<unsigned char> data(path.begin(), path.end());

   // merge nonce and postdata (nonce + postdata) and compute SHA256
   vector<unsigned char> nonce_postdata;
   sha256(nonce + postdata, nonce_postdata);

   // merge path and nonce_postdata (path + sha256(nonce, postdata))
   data.insert(data.end(), nonce_postdata.begin(), nonce_postdata.end());

   // decode Kraken secret
   vector<unsigned char> decodedSecret;
   b64dec(secret_, decodedSecret);

   // and compute HMAC
   vector<unsigned char> digest;  
   hmac(data, decodedSecret, digest);

   // put result in 'sign'
   b64enc(digest, sign);
}

//------------------------------------------------------------------------------
// deals with public API methods:
void KAPI::publicMethod(const string& method, 
			const KAPI::Input& input) const
{
   // build path and postdata 
   string path = "/" + version_ + "/public/" + method;
   string postdata = buildQuery(input);

   // build method URL and set up CURL
   string methodUrl = url_ + path;

   cout << "methodUrl==" << methodUrl << endl
	<< "postdata==" << postdata << endl;

   curl_easy_setopt(curl_, CURLOPT_URL, methodUrl.c_str());
   curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postdata.c_str());
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, NULL);
   
   // perform CURL request
   CURLcode result = curl_easy_perform(curl_);
   if (result != CURLE_OK) {
      ostringstream oss;  
      oss << "curl_easy_perform() failed: "  
	  << curl_easy_strerror(result);
      throw runtime_error(oss.str());
   }
}

//------------------------------------------------------------------------------
// deals with private API methods:
void KAPI::privateMethod(const string& method, 
			 const KAPI::Input& input) const
{   
   // build path
   string path = "/" + version_ + "/private/" + method;

   // create a nonce and add it to postdata 
   string nonce = createNonce();
   string postdata = "nonce=" + nonce;

   // if 'input' is not empty generate other postdata
   if (!input.empty())
      postdata = postdata + "&" + buildQuery(input);

   // generate message signature
   string sign;
   messageSignature(path, nonce, postdata, sign);

    // build method URL and make request
   string methodUrl = url_ + path;

   curl_easy_setopt(curl_, CURLOPT_URL, methodUrl.c_str());
   curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postdata.c_str());

   // add custom header
   curl_slist* chunk = NULL;

   string header = "API-Key: " + key_;
   chunk = curl_slist_append(chunk, header.c_str()); 
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, chunk);

   header = "API-Sign: " + sign;
   chunk = curl_slist_append(chunk, header.c_str()); 
   curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, chunk);
   
   // perform CURL request
   CURLcode result = curl_easy_perform(curl_);

   // free the custom headers
   curl_slist_free_all(chunk);
  
   // check perform result
   if (result != CURLE_OK) {
      ostringstream oss;
      oss << "curl_easy_perform() failed: "  
	  << curl_easy_strerror(result);
      throw runtime_error(oss.str());
   }
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi("",
		"");
      KAPI::Input in; 

      // kapi.publicMethod("Ticker", in);
      kapi.privateMethod("Balance", in);
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
