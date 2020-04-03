#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <iterator>
#include "cache.hh"
#include "evictor.hh"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <sstream>
#include "kv_json.hh"
#include "listener.hh"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//Template function since we may have different types of requests passed in

int main(int ac, char* av[])
{
    try {
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("maxmem", boost::program_options::value<Cache::size_type>() -> default_value(30), "Maximum memory stored in the cache")//had to change from 10000 to 30 for tests to work
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
        auto const server = boost::asio::ip::make_address(vm["server"].as<std::string>());
        unsigned short const port = vm["port"].as<unsigned short>();
        int const threads = vm["threads"].as<int>();
        Cache::size_type maxmem = vm["maxmem"].as<Cache::size_type>();

        std::cout << "maxmem: "<<maxmem<< "\n";
        std::cout << "port: " << port << "\n";
        std::cout << "server: " << server<< "\n";
        std::cout <<"threads: " << threads << "\n";
        if(threads < 0) {
            std::cerr << "args are wrong - must use >0 threads." << std::endl;
            return 1;
        }
        Cache server_cache(maxmem);
        Cache* server_cache_p = &server_cache;
        boost::asio::io_context ioc{threads};

        std::make_shared<listener>(ioc,tcp::endpoint{server, port}, server_cache_p)->run();

        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for(auto i = threads - 1; i > 0; --i)
            v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
        ioc.run();

        return EXIT_SUCCESS;
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }
}
