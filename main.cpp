#include <iostream>
#include <stdexcept>

#include "kapi.hpp"

using namespace std;
using namespace Kraken;

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input in;

      //cout << kapi.private_method("Balance", in) << endl;
      cout << kapi.public_method("Time", in) << endl;
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
