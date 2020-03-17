#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "cache.hh"
#include "fifo_evictor.h"

//each comment line becomes one test
//each test will be its own section, and the cache will be reset at the end of each
//a new cache will be made for each test case
//14 total tests; maybe cut a few?


//We make a test cache to use for all the tests instances;
//Since we are creating it now, we also add the hash function that we will be testing.
//The function is defined below, followed by the cache;
//we do not use an evictor with the cache since we test the basic rejection
//behavior of the cache here, and then the evictor separately.

//Hash function here is a lambda that always returns 2
//It also records all of its inputs,
//So we can check that it is actually getting called
std::vector<key_type> in_vec;
std::function<std::size_t(key_type)> bad_hash = [&](key_type k) {
    in_vec.push_back(k);
    return 2;
};

auto test_cache = Cache(30, 0.75, nullptr, bad_hash);

Cache::size_type size; //Need this so we can pass size for the get calls; also is checked in some tests

//Test null entry
//test set/get with multiple entries
//test overwrite
TEST_CASE("Set and Get")
{

    SECTION("Nonentry Get"){
        REQUIRE(test_cache.get("key_one", size) == nullptr);

        test_cache.reset();//reset at the end of each section;
        //this assumes reset is working, which isn't tested until later...
    }

    SECTION("Normal Set/Get"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.set("key_two", "value_2", 8);
        test_cache.set("key_three", "value_3", 8);
        Cache::val_type val = "value_2"; //prepare a char pointer for comparison

        REQUIRE(*test_cache.get("key_two", size) == *val);

        test_cache.reset();
    }

    SECTION("Overwrite and Get"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.set("key_one", "value_9", 8); //overwrite
        Cache::val_type val = "value_9";

        REQUIRE(*test_cache.get("key_one", size) == *val);

        test_cache.reset();
    }

}

    //test total size after adding a few things
    //test size on get returning a size
    //test rejection on set for a full cache
    //Test that size changes after an overwrite
TEST_CASE("Size and Capacity") //uses no evictor and therefore should reject once full
{
    SECTION("Total Size"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.set("key_two", "value_20", 9);
        test_cache.set("key_three", "value_300", 10);

        REQUIRE(test_cache.space_used() == 27);

        test_cache.reset();
    }

    SECTION("Get Size"){
        size = 0; //reset/initialize size
        test_cache.set("key_one", "value_1", 8);
        test_cache.get("key_one", size);//should write into size var

        REQUIRE(size == 8);

        test_cache.reset();
    }

    SECTION("Rejection"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.set("key_two", "value_2", 8);
        test_cache.set("key_three", "value_3", 8);
        test_cache.set("key_four", "value_4", 8);//should be blocked since we're already at 24/30 capacity

        REQUIRE(test_cache.get("key_four", size) == nullptr);

        test_cache.reset();
    }

    SECTION("Overwrite Size"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.set("key_one", "value_one", 10);

        REQUIRE(test_cache.space_used() == 10);

        test_cache.reset();
    }

}

//test delete will return nullptr after get
//test delete will change total size
TEST_CASE("Delete")
{

    SECTION("Delete and Get"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.del("key_one");

        REQUIRE(test_cache.get("key_one", size) == nullptr);

        test_cache.reset();
    }

    SECTION("Delete and Size"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.del("key_one");

        REQUIRE(test_cache.space_used() == 0);

        test_cache.reset();
    }

}

//reset for nullptr
//reset for size
TEST_CASE("Reset")
{

    SECTION("Reset and Get"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.reset();

        REQUIRE(test_cache.get("key_one", size) == nullptr);

        test_cache.reset();
    }

    SECTION("Reset and Size"){
        test_cache.set("key_one", "value_1", 8);
        test_cache.reset();

        REQUIRE(test_cache.space_used() == 0);

        test_cache.reset();
    }
}

//test using custom hash function results in func being used
TEST_CASE("Hash Functions")
{

    SECTION("Hash Use"){
        test_cache.set("key_one", "value_1", 8);

        REQUIRE(in_vec.back() == "key_one");

        test_cache.reset();
    }

}

//Touch a few things with the evictor, then test that evict returns the first of those
//add multiple, evict multiple, check 3rd evict gives 3rd entered
TEST_CASE("Fifo Evictor")
{


    SECTION("Evict Returns First"){
        FifoEvictor evictor = FifoEvictor();//no reset for Evictor so we make a new one each time
        evictor.touch_key("key_1");
        evictor.touch_key("key_2");
        evictor.touch_key("key_3");

        REQUIRE(evictor.evict() == "key_1");//Should be key_type now, which is just string, so no need to make/deref a char pointer

    }

    SECTION("Evict Removes"){
        FifoEvictor evictor = FifoEvictor();//no reset for Evictor so we make a new one each time
        evictor.touch_key("key_1");
        evictor.touch_key("key_2");
        evictor.touch_key("key_3");

        evictor.evict();
        evictor.evict();
        REQUIRE(evictor.evict() == "key_3");

    }

    SECTION("Evict on empty returns null"){
        FifoEvictor evictor = FifoEvictor();
        REQUIRE(evictor.evict() == "");
    }

}
