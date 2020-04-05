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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void fail(boost::beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

http::response<http::string_body> server_error(http::request<http::string_body> req, std::string message) {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + message + "'";
    res.prepare_payload();
    return res;
}
http::response<http::string_body> bad_request(http::request<http::string_body> req) {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Unknown HTTP-method";
    res.prepare_payload();
    return res;
}
http::response<http::string_body> not_found(http::request<http::string_body> req, std::string target) {
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + target + "' was not found.\n";
    res.prepare_payload();
    return res;
}
http::response<http::string_body> handle_request(http::request<http::string_body>&& req, Cache* server_cache) {
    if( req.method() != http::verb::get &&
        req.method() != http::verb::put &&
        req.method() != http::verb::delete_ &&
        req.method() != http::verb::head &&
        req.method() != http::verb::post)
        return bad_request(req);

    http::response<http::string_body> res;
    res.version(11);   // HTTP/1.1
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.keep_alive(req.keep_alive());
    if(req.method() == http::verb::get)
    {
      key_type key = std::string(req.target()).substr(1); //make a string and slice off the "/"" from the target
      Cache::size_type size;
      std::cout << "getting..." << key << std::endl;

      Cache::val_type val = server_cache->get(key, size);
      if(val == nullptr){
          std::cout << "not found" << std::endl;
          return not_found(req, key);
      } else {
          std::cout << "cache["<<key<<"]=" << val << std::endl;
          res.result(boost::beast::http::status::ok);
          kv_json kv(key, val);
          std::string json = kv.as_string();
          res.body() = json;
          res.set(boost::beast::http::field::content_type, "json");
          res.content_length(json.size() + 1);
          res.prepare_payload();
          return res;
      }
    }

    //Handles a PUT request
    //Maybe add error checking to ensure the things are of the correct types? or not necessary?
    if(req.method() == http::verb::put)
    {
        //First we extract the key and the value from the request target
        // std::cout << "target: " << req.target() << std::endl << std::endl;
        std::stringstream target_string(std::string(req.target()).substr(1)); //Slice off the first "/" then make a sstream for further slicing
        std::string key_str;
        std::string val_str;
        std::getline(target_string, key_str, '/');
        std::getline(target_string, val_str, '/');
        //And now we need to convert the value into a char pointer so we can insert into the cache
        key_type key = key_str;
        Cache::val_type val = const_cast<char*>(val_str.c_str());
        bool key_created = false;
        Cache::size_type size = 0;
        //We then check if the key is already in the Cache (need for status code) and then set the value
        if(server_cache->get(key_str, size) == nullptr){
            key_created = true;
        }
        size = val_str.length()+1;
        std::cout << "setting...";
        server_cache->set(key, val, size);
        //Now we can create and send the response
        res.set(boost::beast::http::field::content_location, "/" + key_str);
        //set the appropriate status code based on if a new entry was created
        if(key_created){
            res.result(201);
        } else {
            res.result(204);
        }
        res.keep_alive(true);
        res.prepare_payload();
        // std::cout << "cache[" << key << "] now equals: " << server_cache -> get(key, size) << std::endl;
        return res;
    }

    //Will send a response for if key was deleted or not found; same effects either way
    if(req.method() == http::verb::delete_) {
        std::cout << "deleting ";
        key_type key = std::string(req.target()).substr(1);
        std::cout << key << "...";
        auto success = server_cache->del(key);
        if(success){
            std::cout << "done" << std::endl;
            res.result(boost::beast::http::status::ok);
        } else {
            std::cout << "error: not found" << std::endl;
            return not_found(req, key);
        }
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    }

    if(req.method() == http::verb::head)
    {
        res.result(http::status::ok);
        res.set(boost::beast::http::field::content_type, "application/json");
        res.set(boost::beast::http::field::accept, "application/json");
        res.insert("Space-Used", server_cache->space_used());
        res.prepare_payload();
        return res;
    }

    if(req.method() == http::verb::post) {
        if(std::string(req.target()) != "/reset"){
            return not_found(req, std::string(req.target()));
        }
        //Resets the cache and sends back a basic response with string body
        server_cache->reset();
        std::cout << "resetting the cache" << std::endl;
        http::response<boost::beast::http::string_body> res;
        res.result(boost::beast::http::status::ok);
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.body() = "The Cache has been reset";
        res.set(boost::beast::http::field::content_type, "text");
        res.prepare_payload();
        return res;
    }
    return bad_request(req);

}

class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct helper
    {
        session& self_;

        explicit helper(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void send(http::message<isRequest, Body, Fields>&& msg) const
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
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    helper helper_;
    Cache*server_cache_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,
        Cache* cache)
        : stream_(std::move(socket))
        , helper_(*this)
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
        if(ec == http::error::end_of_stream){
            std::cout << "end of stream; closing" << std::endl;
            return do_close();
        }

        if(ec)
            return fail(ec, "read");

        // Send the response
        http::response<http::string_body> res = handle_request(std::move(req_), server_cache_);
        helper_.send(std::move(res));
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
            std::cout << "Trying to close" << std::endl;
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
