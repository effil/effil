#include <gtest/gtest.h>

#include "test-utils.h"
#include "garbage-collector.h"

#include <thread>
#include <vector>

using namespace effil;

TEST(gc, GCObject) {
    GCObject o1;
    EXPECT_EQ(o1.instances(), (size_t)1);

    GCObject o2 = GC::instance().create<GCObject>();
    EXPECT_EQ(o2.instances(), (size_t)2);

    GCObject o3 = GC::instance().create<GCObject>();
    GCObject o4(o3);
    GCObject o5(o4);
    EXPECT_EQ(o5.instances(), o3.instances());
    EXPECT_EQ(o5.instances(), (size_t)4);
    EXPECT_EQ(o5.handle(), o3.handle());
    EXPECT_NE(o1.handle(), o5.handle());
}

TEST(gc, collect) {
    GC::instance().collect();
    ASSERT_EQ(GC::instance().size(), (size_t)1);
    {
        GCObject o1 = GC::instance().create<GCObject>();
        GCObject o2 = GC::instance().create<GCObject>();
    }
    EXPECT_EQ(GC::instance().size(), (size_t)3);
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)1);
}

namespace {

struct Dummy : public GCObject {
    void add(GCObjectHandle ref) { refs_->insert(ref); }
};

}

TEST(gc, withRefs) {
    GC::instance().collect();
    {
        Dummy root = GC::instance().create<Dummy>();

        {
            Dummy orphan = GC::instance().create<Dummy>();
            for (size_t i = 0; i < 3; i++) {
                Dummy child = GC::instance().create<Dummy>();
                root.add(child.handle());
            }
        }
        EXPECT_EQ(GC::instance().size(), (size_t)6);
        GC::instance().collect();
        EXPECT_EQ(GC::instance().size(), (size_t)5);
    }
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)1);
}

TEST(gc, autoCleanup) {
    std::vector<std::thread> threads;
    size_t objectsPerThread = 1000;

    for (size_t i = 0; i < 5; i++)
        threads.emplace_back([=] {
            for (size_t i = 0; i < objectsPerThread; i++)
                GC::instance().create<GCObject>();
        });

    for (auto& thread : threads)
        thread.join();

    EXPECT_LT(GC::instance().size(), GC::instance().step());
}

TEST(gc, gcInLuaState) {
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = GC::instance().create<SharedTable>();
    lua.script(R"(
for i=1,1000 do
st[i] = {"Wow"}
end
)");
    EXPECT_EQ(GC::instance().size(), (size_t)1002);

    lua.script(R"(
for i=1,1000 do
st[i] = nil
end
)");
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)2);
}

TEST(gc, cycles) {
    {
        sol::state lua;
        bootstrapState(lua);
        GC::instance().collect();

        lua["st"] = GC::instance().create<SharedTable>();
        lua.script(R"(
st.parent = {}
st.parent.child = { ref = st.parent }
st[4] = { one = 1 }
st[5] = { flag = true }
)");
        EXPECT_EQ(GC::instance().size(), (size_t)6);

        lua.script("st.parent = nil");

        lua.collect_garbage();
        GC::instance().collect();
        EXPECT_EQ(GC::instance().size(), (size_t)4);
    }
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)1);
}

TEST(gc, multipleStates) {
    sol::state lua1;
    sol::state lua2;
    bootstrapState(lua1);
    bootstrapState(lua2);

    {
        SharedTable st = GC::instance().create<SharedTable>();
        lua1["st"] = st;
        lua2["st"] = st;
    }
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)2);

    lua1.script(R"(
st.men = { name = "John", age = 22 }
st.car = { name = "Lada", model = 12 }
st.cat = { name = "Tomas" }
st.fish = { name = "Herbert" }

st.men.car = st.car
st.men.cat = st.cat
st.men.fish = st.fish
)");
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)6);

    lua2.script("copy = { st.men } st = nil");
    lua1.script("st = nil");
    lua1.collect_garbage();
    lua2.collect_garbage();
    GC::instance().collect();
    EXPECT_EQ(GC::instance().size(), (size_t)5);
}
