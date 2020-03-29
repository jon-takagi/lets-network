#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "cache.hh"
#include "evictor.hh"
#include <sstream>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

using key_type = std::string;
struct kv_json {
    key_type key_;
    Cache::val_type value_;
    kv_json(std::string data) {
        std::stringstream stream;
        stream << data.c_str();
        boost::property_tree::ptree tree;
        boost::property_tree::read_json(stream, tree);
        std::cout << tree.get<std::string>("key") << ": " << tree.get<std::string>("value") << std::endl;
        key_ = tree.get<key_type>("key");
        value_ = tree.get<std::string>("value").c_str();
    }
    kv_json(key_type k, Cache::val_type val) {
        key_ = k;
        value_ = val;
    }
    std::string as_string() {
        std::ostringstream oss;
        boost::property_tree::ptree tree;
        tree.put("key", key_);
        tree.put("value", value_);
        boost::property_tree::write_json(oss, tree);
        return oss.str();
    }
};
