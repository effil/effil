#include <gtest/gtest.h>

#include "notifier.h"

#include <thread>
#include <atomic>

using namespace effil;

TEST(notifier, wait) {
    Notifier n;
    bool done = false;
    auto t = std::thread([&]{
        done = true;
        n.notify();
    });

    n.wait();
    EXPECT_TRUE(done);
    t.join();
}

TEST(notifier, waitMany) {
    const size_t nfutures = 32;
    std::vector<std::thread> vt;
    std::atomic<size_t> counter(0);
    Notifier n;

    for(size_t i = 0; i < nfutures; i++)
        vt.emplace_back(std::thread([&]{
            n.wait();
            counter++;
        }));

    EXPECT_EQ(counter.load(), (size_t)0);

    n.notify();
    for(auto& t : vt) t.join();
    EXPECT_EQ(counter.load(), nfutures);
}

TEST(notifier, waitFor) {
    Notifier n;
    auto t = std::thread([&] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        n.notify();
    });

    EXPECT_FALSE(n.waitFor(std::chrono::seconds(1)));
    EXPECT_TRUE(n.waitFor(std::chrono::seconds(2)));
    t.join();
}

TEST(notifier, reset) {
    const size_t iterations = 1024;
    Notifier readyToProcess;
    Notifier needNew;
    size_t resource = 0;

    std::thread producer([&]() {
        for (size_t i = 0; i < iterations; i++) {
            resource++;
            readyToProcess.notify();
            needNew.wait();
            needNew.reset();
        }
    });

    std::thread consumer([&](){
        for (size_t i = 0; i < iterations; i++) {
            readyToProcess.wait();
            readyToProcess.reset();
            EXPECT_EQ(resource, i + 1);
            needNew.notify();
        }
    });

    producer.join();
    consumer.join();
}