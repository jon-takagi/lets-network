#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <iterator>
#include "cache.hh"
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
