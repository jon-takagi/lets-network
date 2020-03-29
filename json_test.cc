#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <exception>
#include <sstream>
#include <iostream>
#include "kv_json.hh"
//
int main()
{
    // key_type key = "key_one";
    // Cache::val_type val = "val_one";
    // kv_json kv(key, val);
    // std::string json = kv.as_string();
    // // std::string json = "hello, world!";
    // // boost::property_tree::write_json(json, kv);
    // std::cout << kv.as_string() << std::endl;
    std::string str = "{ \"key\": \"key_one\", \"value\": \"val_one\"}";
    kv_json kv2(str);
    // std::cout << kv2.key_ << ": " << kv2.value_ << std::endl;
    return 0;
}
