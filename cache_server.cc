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

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

//
struct bad_args_exception: public std::exception{
    const char *what() {
        return "Bad parameters";
    }
};


//Template function since we may have different types of requests passed in
template< class Body, class Allocator,class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, Cache* server_cache)
{
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };


    //Lambda for handling errors, borrowed from async beast server example
    //and changed here to better fit our needs
    // Returns a not found response; used when GET and POST go wrong
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.\n";
        res.prepare_payload();
        return res;
    };
    if( req.method() != http::verb::get &&
        req.method() != http::verb::put &&
        req.method() != http::verb::delete_ &&
        req.method() != http::verb::head &&
        req.method() != http::verb::post)
        return send(bad_request("Unknown HTTP-method"));

    if(req.method() == http::verb::get)
    {
      key_type key = std::string(req.target()).substr(1); //make a string and slice off the "/"" from the target
      Cache::size_type size;
      std::cout << "getting..." << key << std::endl;
      Cache::val_type val = server_cache->get(key, size);
      std::cout << "cache["<<key<<"]=" << val << std::endl;
      std::cout << server_cache->get("key_one", size) << std::endl;

      std::cout << server_cache->space_used() << std::endl;
      if(server_cache->get(key, size) == nullptr){
          return send(not_found(key));
      } else {
          http::response<boost::beast::http::string_body> res;
          res.version(11);   // HTTP/1.1
          res.result(boost::beast::http::status::ok);
          res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
          res.body() = "{ \"key\": " + key + ", \"value\":" + val + "}";
          res.set(boost::beast::http::field::content_type, "json");
          res.content_length(key.length() + size + 19);
          res.keep_alive(req.keep_alive());
          res.prepare_payload();
          return send(std::move(res));
      }
    }

    //Handles a PUT request
    //Maybe add error checking to ensure the things are of the correct types? or not necessary?
    if(req.method() == http::verb::put)
    {
        //First we extract the key and the value from the request target
        std::cout << "target: " << req.target() << std::endl << std::endl;
        std::stringstream target_string(std::string(req.target()).substr(1)); //Slice off the first "/" then make a sstream for further slicing
        std::string key_str;
        std::string val_str;
        std::getline(target_string, key_str, '/');
        std::getline(target_string, val_str, '/');
        //And now we need to convert the value into a char pointer so we can insert into the cache
        key_type key = key_str;
        Cache::val_type val = const_cast<char*>(val_str.c_str()) ;
        bool key_created = false;
        Cache::size_type size = 0;
        //We then check if the key is already in the Cache (need for status code) and then set the value

        if(server_cache->get(key_str, size) == nullptr){
            key_created = true;
            size = val_str.length()+1;
        }
        std::cout << "setting...";
        server_cache->set(key, val, size);
        if(server_cache -> get(key, size)[0] == '\0') {
            std::cout << "\n error." << std::endl;
            return send(server_error("auuuuuugh"));
        } else {
            std::cout <<"done" << std::endl;
        }

        //Now we can create and send the response
        http::response<boost::beast::http::string_body> res;
        res.set(boost::beast::http::field::content_location, "/" + key_str);
        res.version(11);   // HTTP/1.1
        //set the appropriate status code based on if a new entry was created
        if(key_created){
            res.result(201);
        } else {
            res.result(204);
        }
        std::cout << "cache[" << key << "] now equals: " << server_cache -> get(key, size);
        return send(std::move(res));
    }

    //Will send a response for if key was deleted or not found; same effects either way
    if(req.method() == http::verb::delete_) {
        key_type key = std::string(req.target()).substr(1);
        auto success = server_cache->del(key);
        http::response<boost::beast::http::string_body> res;
        res.version(11);   // HTTP/1.1
        res.result(boost::beast::http::status::ok);
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        if(success){
            res.body() = "The key " + key + " was deleted from the cache\n";
            res.content_length(key.length() + 36);
        } else {
            res.body() = "The key " + key + " was not found in the cache\n";
            res.content_length(key.length() + 36);
        }
        res.set(boost::beast::http::field::content_type, "text");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return send(std::move(res));
    }

    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res;//No body for head response
        res.version(11);   // HTTP/1.1
        res.result(http::status::ok);
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "application/json");
        res.set(boost::beast::http::field::accept, "application/json");
        res.content_length(server_cache->space_used());
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    if(req.method() == http::verb::post) {
        if(std::string(req.target()) != "/reset"){
            return send(not_found(req.target()));
        }
        //Resets the cache and sends back a basic response with string body
        server_cache->reset();
        http::response<boost::beast::http::string_body> res;
        res.version(11);   // HTTP/1.1
        res.result(boost::beast::http::status::ok);
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.body() = "The Cache has been reset";
        res.set(boost::beast::http::field::content_type, "text");
        res.content_length(25);//counting null terminator
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return send(std::move(res));
    }
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string("Unknown HHTP Request Method");
    res.prepare_payload();
    return send(std::move(res));

}





// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        session& self_;

        explicit
        send_lambda(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            http::async_write(
                self_.stream_,
                *sp,
                beast::bind_front_handler(
                    &session::on_write,
                    self_.shared_from_this(),
                    sp->need_eof()));
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;
    Cache*server_cache_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,
        Cache* cache)
        : stream_(std::move(socket))
        , lambda_(*this)
    {
        server_cache_ = cache;
    }

    // Start the asynchronous operation
    void
    run()
    {
        do_read();
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(stream_, buffer_, req_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // Send the response
        handle_request(std::move(req_), lambda_, server_cache_);
    }

    void
    on_write(
        bool close,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res_ = nullptr;

        // Read another request
        do_read();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    Cache* server_cache_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        Cache* cache)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
    {
        server_cache_ = cache;
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
                std::move(socket), server_cache_)->run();
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
        boost::asio::io_context ioc{threads};

        Cache* server_cache_p = &server_cache;
        // Create and launch a listening port
        std::make_shared<listener>(
            ioc,
            tcp::endpoint{server, port}, server_cache_p)->run();

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
