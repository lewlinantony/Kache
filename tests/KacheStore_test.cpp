// tests/KacheStore_test.cpp

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "KacheStore.hpp"

// TEST is a macro from Google Test to define a test case.
// First argument: The test suite name.
// Second argument: The specific test name.

// A simple test to check basic SET and GET functionality.
TEST(KacheStoreTest, SetAndGet) {
    KacheStore store;
    store.set("key1", "value1");

    auto val = store.get("key1");
    ASSERT_TRUE(val.has_value()); // Assert that the optional is not empty
    EXPECT_EQ(*val, "value1");    // Assert that the value is correct
}

// Test getting a key that does not exist.
TEST(KacheStoreTest, GetNonExistentKey) {
    KacheStore store;
    auto val = store.get("nonexistent");
    EXPECT_FALSE(val.has_value()); // Assert that the optional is empty
}

// Test deleting a key.
TEST(KacheStoreTest, DeleteKey) {
    KacheStore store;
    store.set("key1", "value1");
    EXPECT_TRUE(store.exists("key1")); // Key should exist

    bool deleted = store.del("key1");
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(store.exists("key1")); // Key should now be gone
}

// This is the most important test for your resume.
// It proves your mutex is working correctly.
TEST(KacheStoreTest, ConcurrentAccess) {
    KacheStore store;
    std::vector<std::thread> threads;
    const int num_threads = 10;
    const int ops_per_thread = 100;

    // Spawn 10 threads, and each thread will write 100 keys.
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&store, i, ops_per_thread, num_threads]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                // Each thread writes to a unique set of keys to avoid overwriting.
                std::string key = "key-" + std::to_string(i) + "-" + std::to_string(j);
                std::string value = "value-" + std::to_string(i);
                store.set(key, value);
            }
        });
    }

    // Wait for all threads to finish.
    for (auto& t : threads) {
        t.join();
    }

    // After all threads are done, verify that all keys were written correctly.
    // If the mutex was broken, some writes would have been lost, and this would fail.
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < ops_per_thread; ++j) {
            std::string key = "key-" + std::to_string(i) + "-" + std::to_string(j);
            auto val = store.get(key);
            ASSERT_TRUE(val.has_value());
            EXPECT_EQ(*val, "value-" + std::to_string(i));
        }
    }
}
