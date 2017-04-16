#include <gtest/gtest.h>

#include "test-utils.h"
#include "shared-table.h"
#include "garbage-collector.h"

#include <thread>

using namespace effil;

TEST(sharedTable, primitiveTypes) {
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = SharedTable();

    auto res1 = lua.script(R"(
        st.fst = "first"
        st.snd = 2
        st.thr = true
        st.del = "secret"
        st.del = nil
    )");

    EXPECT_TRUE(res1.valid()) << "Set res1 failed";
    EXPECT_EQ(lua["st"]["fst"], std::string("first"));
    EXPECT_EQ(lua["st"]["snd"], (double)2);
    EXPECT_EQ(lua["st"]["thr"], true);
    EXPECT_EQ(lua["st"]["del"], sol::nil);
    EXPECT_EQ(lua["st"]["nex"], sol::nil);

    auto res2 = lua.script(R"(
        st[1] = 3
        st[2] = "number"
        st[-1] = false
        st[42] = "answer"
        st[42] = nil
        st.deleted = st[42] == nil
    )");

    EXPECT_TRUE(res2.valid()) << "Set res2 failed";
    EXPECT_EQ(lua["st"][1], 3);
    EXPECT_EQ(lua["st"][2], std::string("number"));
    EXPECT_EQ(lua["st"][-1], false);
    EXPECT_EQ(lua["st"]["deleted"], true);

    auto res3 = lua.script(R"(
        st[true] = false
        st[false] = 9
    )");

    EXPECT_TRUE(res3.valid()) << "Set res3 failed";
    EXPECT_EQ(lua["st"][true], false);
    EXPECT_EQ(lua["st"][false], 9);
}

TEST(sharedTable, multipleStates) {
    sol::state lua1, lua2;
    bootstrapState(lua1);
    bootstrapState(lua2);

    auto st = std::make_unique<SharedTable>();

    lua1["cats"] = st.get();
    lua2["dogs"] = st.get();

    auto res1 = lua1.script(R"(
        cats.fluffy = "gav"
        cats.sparky = false
        cats.wow = 3
    )");

    EXPECT_EQ(lua2["dogs"]["fluffy"], std::string("gav"));
    EXPECT_EQ(lua2["dogs"]["sparky"], false);
    EXPECT_EQ(lua2["dogs"]["wow"], 3);
}

TEST(sharedTable, multipleThreads) {
    SharedTable st;

    std::vector<std::thread> threads;

    threads.emplace_back([=]() {
        sol::state lua;
        bootstrapState(lua);
        ;
        lua["st"] = st;
        lua.script(R"(
            while not st.ready do end
            st.fst = true
        )");
    });

    threads.emplace_back([=]() {
        sol::state lua;
        bootstrapState(lua);
        lua["st"] = st;
        lua.script(R"(
            while not st.ready do end
            st.snd = true
        )");
    });

    threads.emplace_back([=]() {
        sol::state lua;
        bootstrapState(lua);
        lua["st"] = st;
        lua.script(R"(
            while not st.ready do end
            st.thr = true
        )");
    });

    sol::state lua;
    bootstrapState(lua);
    lua["st"] = st;
    lua.script("st.ready = true");

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(lua["st"]["fst"], true);
    EXPECT_EQ(lua["st"]["snd"], true);
    EXPECT_EQ(lua["st"]["thr"], true);
}

TEST(sharedTable, playingWithSharedTables) {
    sol::state lua;
    bootstrapState(lua);

    lua["recursive"] = GC::instance().create<SharedTable>();
    lua["st1"] = GC::instance().create<SharedTable>();
    lua["st2"] = GC::instance().create<SharedTable>();

    lua.script(R"(
        st1.proxy = st2
        st1.proxy.value = true
        recursive.next = recursive
        recursive.val = "yes"
    )");
    EXPECT_EQ(lua["st2"]["value"], true);
    EXPECT_EQ(lua["recursive"]["next"]["next"]["next"]["val"], std::string("yes"));
}

TEST(sharedTable, playingWithFunctions) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = st;

    lua.script(R"(
        st.fn = function ()
            print "Hello C++"
            return true
        end
        st.fn()
    )");

    sol::function sf = lua["st"]["fn"];
    EXPECT_TRUE((bool)sf());

    sol::state lua2;
    bootstrapState(lua2);

    lua2["st2"] = st;
    lua2.script(R"(
        st2.fn2 = function(str)
            return "*" .. str .. "*"
        end
    )");

    sol::function sf2 = lua["st"]["fn2"];

    EXPECT_EQ(sf2(std::string("SUCCESS")).get<std::string>(), std::string("*SUCCESS*"));
}

TEST(sharedTable, playingWithTables) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = st;
    auto res = lua.script(R"(
        st.works = "fine"
        st.person = {name = 'John Doe', age = 25}
        pet = {
            type = "cat",
            name = "Tomas",
            real = "Яша",
            owner = "Mama",
            spec = { colour = "grey", legs = 4, eyes = 2 }
        }
        st.pet = pet
        recursive = {}
        recursive.next = recursive
        recursive.prev = recursive
        recursive.val = "recursive"
        st.recursive = recursive
    )");

    EXPECT_TRUE(res.valid());
    EXPECT_EQ(lua["st"]["person"]["name"], std::string("John Doe"));
    EXPECT_EQ(lua["st"]["person"]["age"], 25);
    EXPECT_EQ(lua["st"]["pet"]["type"], std::string("cat"));
    EXPECT_EQ(lua["st"]["pet"]["name"], std::string("Tomas"));
    EXPECT_EQ(lua["st"]["pet"]["real"], std::string("Яша"));
    EXPECT_EQ(lua["st"]["pet"]["spec"]["colour"], std::string("grey"));
    EXPECT_EQ(lua["st"]["pet"]["spec"]["legs"], 4);
    EXPECT_EQ(lua["st"]["recursive"]["prev"]["next"]["next"]["val"], std::string("recursive"));
}

TEST(sharedTable, stress) {
    sol::state lua;
    bootstrapState(lua);
    SharedTable st;

    lua["st"] = st;

    auto res1 = lua.script(R"(
        for i = 1, 1000000 do
            st[i] = tostring(i)
        end
    )");

    EXPECT_TRUE(res1.valid());
    EXPECT_TRUE(SharedTable::luaSize(st) == 1'000'000);

    auto res2 = lua.script(R"(
        for i = 1000000, 1, -1 do
            st[i] = nil
        end
    )");
    EXPECT_TRUE(res2.valid());
    EXPECT_TRUE(SharedTable::luaSize(st) == 0);
}

TEST(sharedTable, stressWithThreads) {
    SharedTable st;

    const size_t threadCount = 10;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < threadCount; i++) {
        threads.emplace_back([=] {
            sol::state lua;
            bootstrapState(lua);
            lua["st"] = st;
            std::stringstream ss;
            ss << "st[" << i << "] = 1" << std::endl;
            ss << "for i = 1, 100000 do" << std::endl;
            ss << "    st[" << i << "] = "
               << "st[" << i << "] + 1" << std::endl;
            ss << "end" << std::endl;
            lua.script(ss.str());
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    sol::state lua;
    bootstrapState(lua);
    lua["st"] = st;
    for (size_t i = 0; i < threadCount; i++) {
        EXPECT_TRUE(lua["st"][i] == 100'001) << (double)lua["st"][i] << std::endl;
    }
}

TEST(sharedTable, ExternalUserdata) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);
    lua["st"] = st;

    struct TestUserdata
    {
        int field;
    };

    lua["udata"] = TestUserdata{17};
    EXPECT_THROW(lua.script("st.userdata = udata"), sol::error);
}

TEST(sharedTable, LightUserdata) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);
    lua["st"] = st;

    int lightUserdata = 19;
    lua_pushlightuserdata(lua, (void*)&lightUserdata);
    lua["light_udata"] = sol::stack::pop<sol::object>(lua);
    EXPECT_TRUE((bool)lua.script("st.light_userdata = light_udata; return st.light_userdata == light_udata"));
    std::string strRet = lua.script("return type(light_udata)");
    EXPECT_EQ(strRet, "userdata");
    EXPECT_EQ(lua["light_udata"].get<sol::lightuserdata_value>().value, &lightUserdata);
}
