#include <gtest/gtest.h>

#include "notifier.h"

#include <future>
#include <atomic>

using namespace effil;

TEST(notifier, wait) {
    Notifier n;
    bool done = false;
    auto f = std::async([&]{
        done = true;
        n.notify();
    });

    n.wait();
    EXPECT_TRUE(done);
    f.wait();
}

TEST(notifier, waitMany) {
    const size_t nfutures = 32;
    std::vector<std::future<void>> vf;
    std::atomic<size_t> counter(0);
    Notifier n;

    for(size_t i = 0; i < nfutures; i++)
        vf.emplace_back(std::async([&]{
            n.wait();
            counter++;
        }));

    EXPECT_EQ(counter.load(), (size_t)0);

    n.notify();
    for(auto& f : vf) f.wait();
    EXPECT_EQ(counter.load(), nfutures);
}

TEST(notifier, waitFor) {
    Notifier n;
    auto f = std::async([&] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        n.notify();
    });

    EXPECT_FALSE(n.waitFor(std::chrono::seconds(1)));
    EXPECT_TRUE(n.waitFor(std::chrono::seconds(2)));
    f.wait();
}

TEST(notifier, reset) {
    Notifier n1;
    Notifier n2;
    bool flag1 = false;
    bool flag2 = false;

    auto f = std::async([&] {
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

    f.wait();
}