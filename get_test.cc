#define CATCH_CONFIG_MAIN
#include <iostream>
#include "cache.hh"
#include "fifo_evictor.h"
#include <chrono>

//File to time 1000 GET requests to local server
//Server needs to be started first in a separate terminal

int main(){
    //set up the cache and stuff
    double min = 999; //can ignore larger values as outliers
    for(int i = 0; i < 20; i++){
        auto test_cache = Cache("127.0.0.1", "42069");
        Cache::size_type size;
        test_cache.set("key_two", "value_2", 8);
        double elapsed = 0;
        //start clock and loop
        std::clock_t start = std::clock();
        test_cache.get("key_two", size);
        std::clock_t end = std::clock();
        elapsed = (end - start ) / (double) CLOCKS_PER_SEC;
        elapsed = (1000000.0 * elapsed); //convert to microseconds (test)
        if(elapsed < min) min = elapsed;
    }
    std::cout << min << std::endl;


    return 0;
}
