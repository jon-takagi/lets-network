#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <iterator>
#include "cache.hh"

struct kv_json {
    keytype key;
    Cache::valtype value;
    void load(key_type k, Cache::valtype val) {
        key = k;
        value_ = val;
    }
    void write(const std::string &filename) {
        boost::propertytree::ptree tree;
        tree.put("key", key);
        tree.put("value", value_);
        boost::property_tree::write_json(filename, tree);
    }
};

void process_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, Cache server_cache)
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

    // Make sure we can handle the method
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == beast::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
    //------------------------------------
    //My code
    if(req.method() == http::verb::get)
    {
      string value = std::string(req.target()).substr(1) //make a string and slice off the "/"" from the target
      val = server_cache.get(value);
      if(val == ""){
          return send(not_found(req.target()));
      } else {
          http::response<string_body> res;
          res.version(11);   // HTTP/1.1
          res.result(status::ok);
          res.set(field::server, BOOST_BEAST_VERSION_STRING);
          kv_json json;
          json.set(key, val);
          json.write("output.json");
          res.body() = "output.json";
          res.prepare_payload();
          res.set(http::field::content_type, "json");
          res.content_length(value.length());
          res.keep_alive(req.keep_alive());
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
        //insert in cache
        http::response<string_body> res;
        res.version(11);   // HTTP/1.1
        if(key_created){
            res.result(201);
        } else {
          res.result(204);
        }
        return send(std::move(res));
    }

    //Check with Eitan on this; assignment says return only header but also wants a space-used pair which then must be in the body?
    //Also check: Accept is supposed to be a request header; also what types go there? don't we want to allow text as well?
    if(req.method() == http::verb::head)
    {
      http::response<string_body> res;
      res.version(11);   // HTTP/1.1
      res.result(status::ok);
      res.set(field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, "application/json");
      res.set(http::field::accept, "text/application/json");
      //add the thing to the body here
      //add the accept header
      res.content_length(size);
      res.keep_alive(req.keep_alive());
      return send(std::move(res));
    }

}

// Handles an HTTP server connection; based on beast example http code from boost.org
void session(tcp::socket& socket, std::shared_ptr<std::string const> const& doc_root)
{
    bool close = false; //Tracking if the connection is closed
    beast::error_code ec;
    beast::flat_buffer buffer; //buffer for reading reqs that will persist across reads

    // This lambda is used to send messages(?)
    send_lambda<tcp::socket> lambda {socket, close, ec};

    while(true)
    {
        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if(ec == http::error::end_of_stream){
            break;
        }
        if(ec){
            return fail(ec, "read");
        }

        // Here we actually hand the stuff; our real work is done here
        //first want to find out what `req` is
        //process_request(*doc_root, std::move(req), lambda);

        if(ec){
            return fail(ec, "write");
        }
        if(close){
            break;
        }
    }

    // shutdown properly after break from close
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

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

    //Once we get a connection, we call the session function on it
    //example server suggests this should be an infinite loop at the end of the try above?

    return 0;
}
