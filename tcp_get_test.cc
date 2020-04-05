#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "cache.hh"
#include "fifo_evictor.h"
#include <chrono>

//File to time 1000 GET requests to local server using tcp
//Server needs to be started first in a separate terminal

int main{
    //set up the cache and stuff
    auto test_cache = Cache("127.0.0.1", "42069");
    Cache::size_type size;
    test_cache.set("key_two", "value_2", 8);
    double elapsed = 0;
    //start clock and loop
    std::clock_t start = std::clock();

    std::clock_t end = std::clock();
    elapsed = (end - start ) / (double) CLOCKS_PER_SEC;
    elapsed = (1000000000.0 * elapsed); //convert to nanoseconds (test)
    std::cout << elapsed << std::endl;


    return 0;
}
