#include <iostream>
#include <stdexcept>

#include "../kapi.hpp"

using namespace std;
using namespace Kraken;

constexpr char api_key[] = "your API key here";
constexpr char api_private_key[] = "your API private key here";

int main()
{
    curl_global_init(CURL_GLOBAL_ALL);

    try {
        KAPI kapi(api_key, api_private_key);
        KAPI::Input in;

        in.insert(make_pair("pair", "XXBTZUSD"));
        in.insert(make_pair("type", "buy"));
        in.insert(make_pair("ordertype", "limit"));
        in.insert(make_pair("price", "45000.1"));
        in.insert(make_pair("volume", "2.1234"));
        in.insert(make_pair("leverage", "2:1"));
        in.insert(make_pair("close[ordertype]", "stop-loss-limit"));
        in.insert(make_pair("close[price]", "38000"));
        in.insert(make_pair("close[price2]", "36000"));

        cout << kapi.private_method("AddOrder", in) << endl;
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
