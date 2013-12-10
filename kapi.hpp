#ifndef _KRAKEN_KAPI_HPP_
#define _KRAKEN_KAPI_HPP_

#include <map>
#include <string>
#include <vector>
#include <curl/curl.h>

//------------------------------------------------------------------------------

namespace Kraken {

//------------------------------------------------------------------------------

class KAPI {
public:  
   // helper type to make requests
   typedef std::map<std::string,std::string> Input;

   // constructor with all explicit parameters
   KAPI(const std::string& key, const std::string& secret, 
        const std::string& url, const std::string& version);

   // default API base URL and API version
   KAPI(const std::string& key, const std::string& secret);
   
   // constructor with empty API key and API secret
   KAPI();

   // distructor
   ~KAPI();

   // makes public method to kraken.com
   void public_method(const std::string& method,
		      const KAPI::Input& input) const;

   // makes private method to kraken.com
   void private_method(const std::string& method,
		       const KAPI::Input& input) const;

   // TODO: public market data
   //void time();
   //void assets();

private:
   // init CURL and other stuffs
   void init();

   // create signature for private requests
   void message_signature(const std::string& path, const std::string& nonce, 
			  const std::string& postdata, std::string& sign) const;

   // helper function to build a query string from KAPI::Input
   static std::string build_query(const KAPI::Input&);

   // creates a nonce
   static std::string create_nonce(); 

   // decodes a base64 string to a vector of bytes
   static void b64_decode(const std::string& input,
			  std::vector<unsigned char>& output);

   // encodes a vector of bytes to a base64 string
   static void b64_encode(const std::vector<unsigned char>& input, 
                          std::string& output);
   
   // hashs data with SHA256
   static void sha256(const std::string& input, std::vector<unsigned char>& output);

   // encrypts data with HMAC and SHA512
   static void hmac_sha512(const std::vector<unsigned char>& input, 
			   const std::vector<unsigned char>& key, 
			   std::vector<unsigned char>& output);

   std::string key_;     // API key
   std::string secret_;  // API secret
   std::string url_;     // API base URL
   std::string version_; // API version
   CURL*  curl_;    // CURL handle
};

//------------------------------------------------------------------------------

}; // namespace Kraken

//------------------------------------------------------------------------------

#endif 


