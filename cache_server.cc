#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <iterator>
#include "cache.hh"
#include "evictor.hh"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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
//
struct bad_args_exception: public std::exception{
    const char *what() {
        return "Bad parameters";
    }
};
//


// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        do_accept();
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
                std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
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
//
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }
        auto const server = boost::asio::ip::make_address(vm["server"].as<std::string>());
        auto const port = vm["port"].as<unsigned short>();
        auto const threads = vm["threads"].as<int>();
        Cache::size_type maxmem = vm["maxmem"].as<Cache::size_type>();

        std::cout << "maxmem: "<<maxmem<< "\n";
        std::cout << "port: " << port << "\n";
        std::cout << "server: " << server<< "\n";
        std::cout <<"threads: " << threads << "\n";
//         // for get(key):
//             // response<string_body> res;
//             // res.version(11);   // HTTP/1.1
//             // res.result(status::ok);
//             // res.set(field::server, "Beast");
//             // val = c.get(value);
//             // if c == ""
//             // error key not found
//             // else
//             // kv_json json;
//             // json.set(key, val);
//             // json.write("output.json");
//             // res.body() = "output.json";
//             // res.prepare_payload();
        if(threads < 0) {
            throw bad_args_exception();
        }
        Cache server_cache(maxmem);
        net::io_context ioc{threads};

        // Create and launch a listening port
        std::make_shared<listener>(
            ioc,
            tcp::endpoint{address, port})->run();

        // Run the I/O service on the requested number of threads
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
    catch(bad_args_exception& e) {
        std::cerr << "bad arguments: " << e.what() << std::endl;
        return 1;
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
