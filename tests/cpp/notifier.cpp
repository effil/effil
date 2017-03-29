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
    Notifier n1;
    Notifier n2;
    bool flag1 = false;
    bool flag2 = false;

    auto t = std::thread([&] {
        n1.notify();
        n2.wait();
        n2.reset();

        flag1 = true;
        n1.notify();

        EXPECT_FALSE(flag2);
        n2.wait();
        EXPECT_TRUE(flag2);
    });

    n1.wait();
    n1.reset();
    n2.notify();

    EXPECT_FALSE(flag1);
    n1.wait();
    EXPECT_TRUE(flag1);

    flag2 = true;
    n2.notify();

    t.join();
}