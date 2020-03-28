#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <set>
#include <exception>
#include <iostream>
#include "kv_json.hh"

int main()
{
    try
    {
        kv_json kv("key_one", "val_one");
        std::string json = kv.as_string();
        // boost::property_tree::write_json(json, ds);
        std::cout << json << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
