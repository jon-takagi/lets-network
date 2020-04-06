Jon Takagi and Eli Poppele

## TCP/UDP Server
Our server is implemented in `cache_server.cc`, which builds into `server.bin`.

### Building
I installed boost into `/usr/local/boost_1_72_0`, then used bootstrap to make a copy in `/vagrant/systems/boost`. In our makefile, these are included in lines 2 and 21. If you install boost elsewhere, or don't use the VM provided by Eitan, then change these lines.

Note that in our makefile we only specify `program_options` to the linker. This is because the other boost libraries we used are "header only" and there is no library archive to link to the compiler, simply using the `-I` flag to add the headers to the include path is sufficient.

### Command Line Args

Valid arguments are -m --maxmem, -s --server, -p --port, -u --udp, and -t --threads. Passing --help will list the arguments. All of these arguments are optional, and their default values are:
| Maxmem | Server      | Port  | UDP  | Threads |
|--------|-------------|-------|------|---------|
|     30 | "127.0.0.1" | 42069 | 9001 | 1       |

Maxmem is measured in bytes. The default was selected to match the values we used in previous tests, and must be set for any serious use. However, we expect this code to be tested more than used, so the default matches the tests.

Server is the hostname of the machine being used to host. Using localhost allows us to test it internally, and determining the right IP for a production host is beyond the scope of our program.

Port is the TCP port to listen on. Any port between 1024 and 49151 is a "user" port, and no other common applications use this point, according to [wikipedia](https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers). The precise number was chosen randomly from [1024,49151], and after consulting Eitan, we decided not to use a dynamic port number. 

UDP is the udp port to listen on. 9001 was similarly selected as an unused and entirely arbitrary port to use.

Threads is the number of CPU threads used to run our code. It defaults to 1, and debugging multithreading bugs is outside the scope of this assignment. If a value less than 1 is passed, the server throws an error.
### Architecture
The server consists of a `main` function in its own file, and imports three classes- `tcp_listener`, `udp_handler`, and `request_processor`. The use of classes allows our server to run asynchronously, although in this case we only test and run a single thread. We consulted Boost documentation and example code to understand how to create this basic structure for an async server and synchronous client. 

#### `main`
The main method parses the command line arguments using `boost::program_options`. First, it creates a `Cache` object using the specified max memory (The other cache parameters are left as default). It then creates a single listener for TCP and UDP each on the specified endpoint, and orders it to run. Finally, it prepares the specified number of threads to be used by the io_context, which allows our code to run on multiple threads.
#### `tcp_listener`
The listener waits for new clients to open connections. When it does, it accepts the connection and creates a strand for it, then returns to listening for clients. This allows a single server to communicate with many clients at once. This class is very similar to the example given in the boost examples as `http_server_async.cpp`, but it takes a `Cache*` in its constructor, and has the equivalent `session` function built in implicitly, as we found such an integration to work well for our purposes.
#### `udp_handler`
The `udp_handler` is relatively simple since it doesn't have to handle sessions for different clients. The code listens for a request, processes it into an http request, and then calls `handle_request` on the instance. Afterwards, it converts the http response back into an appropriate UDP response, and flings that out into the void of the internet.
#### `handle_request`
This method takes a beast request object, a function to send them, and a pointer to the cache object created by main. It first validates that the request method is one of GET, PUT, DELETE, HEAD, or POST. If not, it replies with a bad_request response. Given that we do http conversion in our `udp_handler`, this function is unchanged from our TCP implementation and is used in both the TCP and UDP parts of the server.

#### GET
To respond to a GET request, it first parses the key and attempts to `get` it from the cache. If the cache returns a nullptr, then it sends a 404 response. Otherwise, it uses the `kv_json` class to create a json string from the requested key and returned value, sets that as the body of the response, and sends it.
#### PUT
To respond to a PUT request, the method first extracts and parses the specified key and value into a `key_type` and `val_type` respectively. We check if the key is already in the cache, and set the status to either 204 No Content if the key is present or 201 Created if not - the `set` method of the cache is `void`, so we don't need to return any information, but complying with the HTTP standards requires that when we create an object we use the 201 status. After that, we set the key / value pair in the underlying cache, then return a response with the correct status code.
#### DELETE
To respond to a DELETE request, we parse the desired key and attempt to delete it. If the key was there, we send a 200 OK response after deleting it. If it wasn't, the call to `del` doesn't change the cache and we send a 404 Not Found response.
#### HEAD
To respond to a HEAD request, we call the `space_used` function on the cache. We then insert the result into the "Space-Used" field of our response. Also, the response to a HEAD request has no body, so we use the `http::empty_body` template for our response.
#### POST
The POST request is used to reset the cache, so we ensure that the target is `"/reset"`. We then reset the underlying cache, then send a 200 OK response to notify the user that the cache has successfully been reset.
## TCP Client
`cache_client.cc` is an implementation of the API defined in `cache.hh` that connects to a cache_server over both TCP and UDP. It defines the 4 methods defined in the header, as detailed below, as well as a pair of internal `send` methods, which both take an http::request, send it to the server, and return any response it receives. These functions are helpers that reduce code duplication. `send_tcp` is our standard send method for most of the functions, while `send_udp` is used only for GET.
For the issue of pointer ownership in the return on GET in the client, we extracted the JSON data from the http response using the JSON class. This gave us a value as a cstring pointer; however, using that value would not work since the data is owned by the JSON object (which is deleted at the end of the function) and memcpy also doesn't work in the case of a const pointer (which is what val type is). The hacky solution we used was to create a string as a conversion of the JSON value, then create the val_type in one step using that string in the cstring constructor; this results in the return value pointing to its own copy of the data while the JSON value, now a duplicate, is then deleted.
### constructor / destructor
The constructor takes a host address and two port numbers as parameters, all as `std::string`s. It then resolves the address using the beast `resolver` class, and saves the results for later use by `send`.
### send_tcp
This method constructs a `tcp_stream`, then connects it to the endpoints resolved during the construction of the client. It sends the message it was passed. If an error occurs, it returns a response with a 499 error code and the `what` of the error.

Otherwise, it reads in the response, parses it into a `beast::http::response` object. After closing the connection, it returns the response object.

### send_udp
To be most compatible with our existing GET code, and to maintain that code in its current state so that we can easily switch from using UDP or TCP for get, the `send_udp` function has the same parameters and return type as the `send_tcp` function. We process the string request from the parameter and load it into a buffer so that we can send it over the socket, and once we receive a response, we use a parser to turn the response back into an appropriately-formatted http message.
### prep_req
This method takes an `http::verb` and a target, as an `std::string`. It constructs a request object with the given method and target.
The host is the same as the host being connected to, the agent is the `BOOST_BEAST_VERSION_STRING`, and we use HTTP version 1.1
### API defined methods
Each of these methods constructs an `http::request<string_body>` with default host, user_agent and version as set by `prep_req`.
| Method | Target   | Method |
|--------|----------|--------|
| `set`  | /key/val | PUT    |
| `get`  | /key     | GET    |
| `del`  | /key     | DELETE |
| `reset`| /reset   | POST   |  

To avoid the "unused parameter" warning, the `set` method increments the `size` argument it is passed. We considered printing it instead, but that cluttered up our test results.

## TCP/UDP Testing
We included a test file `get_test.cc` which will run 1000 tests of GET and reports the minimum time from 20 trials of the test. The test simply uses the default behavior of the cache/server, so to test TCP we ran the test on the old code without UDP implementation. This yielded average times of about 101 ms for 1000 calls to a locally hosted server. Running the test on our new and functional UDP code, we found times were now about 107 ms for 100 calls. I suspect that things may work somewhat differently in the case of a nonlocal server, as UDP should be faster once ping is a significant factor in the time it takes a request to process.
