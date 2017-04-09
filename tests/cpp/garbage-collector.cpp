#include <gtest/gtest.h>

#include "test-utils.h"
#include "garbage-collector.h"

#include <thread>
#include <vector>

using namespace effil;

TEST(gc, GCObject) {
    GCObject o1;
    EXPECT_EQ(o1.instances(), (size_t)1);

    GCObject o2 = getGC().create<GCObject>();
    EXPECT_EQ(o2.instances(), (size_t)2);

    GCObject o3 = getGC().create<GCObject>();
    GCObject o4(o3);
    GCObject o5(o4);
    EXPECT_EQ(o5.instances(), o3.instances());
    EXPECT_EQ(o5.instances(), (size_t)4);
    EXPECT_EQ(o5.handle(), o3.handle());
    EXPECT_NE(o1.handle(), o5.handle());
}

TEST(gc, collect) {
    getGC().collect();
    ASSERT_EQ(getGC().size(), (size_t)0);

    {
        GCObject o1 = getGC().create<GCObject>();
        ;
        GCObject o2 = getGC().create<GCObject>();
        ;
    }
    EXPECT_EQ(getGC().size(), (size_t)2);
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)0);
}

namespace {

struct Dummy : public GCObject {
    void add(GCObjectHandle ref) { refs_->insert(ref); }
};
}

TEST(gc, withRefs) {
    getGC().collect();
    {
        Dummy root = getGC().create<Dummy>();

        {
            Dummy orphan = getGC().create<Dummy>();
            for (size_t i = 0; i < 3; i++) {
                Dummy child = getGC().create<Dummy>();
                root.add(child.handle());
            }
        }
        EXPECT_EQ(getGC().size(), (size_t)5);
        getGC().collect();
        EXPECT_EQ(getGC().size(), (size_t)4);
    }
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)0);
}

TEST(gc, autoCleanup) {
    std::vector<std::thread> threads;
    size_t objectsPerThread = 1000;

    for (size_t i = 0; i < 5; i++)
        threads.emplace_back([=] {
            for (size_t i = 0; i < objectsPerThread; i++)
                getGC().create<GCObject>();
        });

    for (auto& thread : threads)
        thread.join();

    EXPECT_LT(getGC().size(), getGC().step());
}

TEST(gc, gcInLuaState) {
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = getGC().create<SharedTable>();
    lua.script(R"(
for i=1,1000 do
st[i] = {"Wow"}
end
)");
    EXPECT_EQ(getGC().size(), (size_t)1001);

    lua.script(R"(
for i=1,1000 do
st[i] = nil
end
)");
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)1);
}

TEST(gc, cycles) {
    {
        sol::state lua;
        bootstrapState(lua);
        getGC().collect();

        lua["st"] = getGC().create<SharedTable>();
        lua.script(R"(
st.parent = {}
st.parent.child = { ref = st.parent }
st[4] = { one = 1 }
st[5] = { flag = true }
)");
        EXPECT_EQ(getGC().size(), (size_t)5);

        lua.script("st.parent = nil");

        lua.collect_garbage();
        getGC().collect();
        EXPECT_EQ(getGC().size(), (size_t)3);
    }
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)0);
}

TEST(gc, multipleStates) {
    sol::state lua1;
    sol::state lua2;
    bootstrapState(lua1);
    bootstrapState(lua2);

    {
        SharedTable st = getGC().create<SharedTable>();
        lua1["st"] = st;
        lua2["st"] = st;
    }
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)1);

    lua1.script(R"(
st.men = { name = "John", age = 22 }
st.car = { name = "Lada", model = 12 }
st.cat = { name = "Tomas" }
st.fish = { name = "Herbert" }

st.men.car = st.car
st.men.cat = st.cat
st.men.fish = st.fish
)");
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)5);

    lua2.script("copy = { st.men } st = nil");
    lua1.script("st = nil");
    lua1.collect_garbage();
    lua2.collect_garbage();
    getGC().collect();
    EXPECT_EQ(getGC().size(), (size_t)4);
}
