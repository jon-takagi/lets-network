#include <boost/program_options.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <iterator>
#include "cache.hh"
#include "evictor.hh"

struct kv_json {
    key_type key_;
    Cache::val_type value_;
    void load(key_type k, Cache::val_type val) {
        key_ = k;
        value_ = val;
    }
    void write(const std::string &filename) {
        boost::property_tree::ptree tree;
        tree.put("key", key_);
        tree.put("value", value_);
        boost::property_tree::write_json(filename, tree);
    }
};


int main(int ac, char* av[])
{
    try {
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("maxmem", boost::program_options::value<Cache::size_type>() -> default_value(10000), "Maximum memory stored in the cache")
            ("port,p", boost::program_options::value<unsigned short>() -> default_value(42069),"Port number")
            ("server,s", boost::program_options::value<std::string>() ->default_value("127.0.0.1"),"IPv4 address of the server in dotted decimal")
            ("threads,t", boost::program_options::value<int>()->default_value(1),"Ignored for now")
        ;
        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(ac, av, desc), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }
        std::cout << "maxmem: "<<vm["maxmem"].as<Cache::size_type>()<< "\n";
        std::cout << "port: " << vm["port"].as<unsigned short>()<< "\n";
        std::cout << "server: " << vm["server"].as<std::string>() << "\n";
        std::cout <<"threads: " << vm["threads"].as<int>() << "\n";

        Cache c(vm["maxmem"].as<Cache::size_type>());
        // for get(key):
            // response<string_body> res;
            // res.version(11);   // HTTP/1.1
            // res.result(status::ok);
            // res.set(field::server, "Beast");
            // val = c.get(value);
            // if c == ""
            // error key not found
            // else
            // kv_json json;
            // json.set(key, val);
            // json.write("output.json");
            // res.body() = "output.json";
            // res.prepare_payload();
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }

    return 0;
}
