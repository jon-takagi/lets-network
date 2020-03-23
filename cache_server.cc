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

template<
    class Body, class Allocator,
    class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, Cache server_cache)
{

    //-------------------------------------------------------
    //Some lambda nonsense for handling errors
    //Will fiugre this out later
    // Returns a bad request response
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

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
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
    //------------------------------------

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Cache the size since we need it after the move
    // auto const size = body.size();

    // Respond to HEAD request
    // if(req.method() == http::verb::head)
    // {
    //     http::response<http::empty_body> res{http::status::ok, req.version()};
    //     res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    //     res.set(http::field::content_type, mime_type(path));
    //     res.content_length(size);
    //     res.keep_alive(req.keep_alive());
    //     return send(std::move(res));
    // }

    // Respond to GET request
    // http::response<http::file_body> res{
    //     std::piecewise_construct,
    //     std::make_tuple(std::move(body)),
    //     std::make_tuple(http::status::ok, req.version())};
    // res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    // res.set(http::field::content_type, mime_type(path));
    // res.content_length(size);
    // res.keep_alive(req.keep_alive());
    // return send(std::move(res));
    //------------------------------------
    //My code
    if(req.method() == http::verb::get)
    {
      key_type key = std::string(req.target()).substr(1); //make a string and slice off the "/"" from the target
      Cache::size_type size;
      Cache::val_type val = server_cache.get(key, size);
      if(strcmp(val, "")){
          return send(not_found(req.target()));
      } else {
          http::response<boost::beast::http::string_body> res;
          res.version(11);   // HTTP/1.1
          res.result(boost::beast::http::status::ok);
          res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

          // kv_json json;
          // json.set(key, val);
          // json.write("output.json");
          res.body() = "{ \"key\": " + key + ", \"value\":" + val + "}";
          res.set(boost::beast::http::field::content_type, "json");
          res.content_length(key.length() + size + 19);
          res.keep_alive(req.keep_alive());
          res.prepare_payload();
          return send(std::move(res));
      }
    }
    if(req.method() == http::verb::put)
    {
        //working on this; do later
        //res.set(http::field::content_location, /with key here;
        /*
        get key and value from target
        check if key already in Cache; remember This
        add key to Cache
        check for errors?
        create a response
        set version
        set content_location
        set status code based on new key or notify
        send
        */


        bool key_created = false;
        http::response<boost::beast::http::string_body> res;
        res.version(11);   // HTTP/1.1
        if(key_created){
            res.result(201);
        } else {
          res.result(204);
        }
        return send(std::move(res));
    }
    if(req.method() == http::verb::delete_) {
        // do delete-y stuff
    }

    //Check with Eitan on this; assignment says return only header but also wants a space-used pair which then must be in the body?
    //Also check: Accept is supposed to be a request header; also what types go there? don't we want to allow text as well?
    if(req.method() == http::verb::head)
    {
      http::response<boost::beast::http::string_body> res;
      res.version(11);   // HTTP/1.1
      res.result(http::status::ok);
      res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, "application/json");
      res.set(http::field::accept, "text/application/json");
      //add the thing to the body here
      //add the accept header
      // res.content_length(size);
      res.keep_alive(req.keep_alive());
      return send(std::move(res));
    }

    if(req.method() == http::verb::post) {
        //do POSTy stuff
    }
    return send(bad_request("Unknown HTTP-method"));

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
    Cache server_cache_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,
        Cache cache)
        : stream_(std::move(socket))
        , lambda_(*this)
        , server_cache_(cache)
    {
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
    Cache server_cache_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        Cache cache)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc)
        , server_cache_(cache))
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
        boost::asio::io_context ioc{threads};

        // Create and launch a listening port
        std::make_shared<listener>(
            ioc,
            tcp::endpoint{server, port}, server_cache)->run();

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
