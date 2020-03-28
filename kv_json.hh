#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "cache.hh"
#include <sstream>
#include <exception>
#include <iostream>
#include <string>
class kv_json {
public:
    key_type key_;
    Cache::val_type value_;
    kv_json(std::string data) {
        boost::property_tree::ptree tree;
        boost::property_tree::read_json(data, tree);
        key_ = tree.get<std::string>("key");
        value_ = tree.get<Cache::val_type>("value");
    }
    kv_json(key_type k, Cache::val_type val) {
        key_ = k;
        value_ = val;
    }
    std::string as_string() {
        // std::ostringstream oss;
        // boost::property_tree::ptree tree;
        // tree.put("key", key_);
        // tree.put("value", value_);
        // boost::property_tree::write_json(oss, tree);
        // return oss.str();
        return "hello, world";
    }
};
