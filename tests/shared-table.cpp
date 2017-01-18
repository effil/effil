#include <gtest/gtest.h>

#include "shared-table.h"

using namespace core;

namespace {

void bootstrapState(sol::state& lua) {
    lua.open_libraries(
            sol::lib::base,
            sol::lib::string
    );
    SharedTable::bind(lua);
}

}

TEST(sharedTable, singleThreadSet) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = &st;

    auto res1 = lua.script(R"(
st.fst = "first"
st.snd = 2
st.thr = true
st.del = "secret"
st.del = nil
)");
    if (!res1.valid()) {
        FAIL() << "Set res1 failed";
    }

    ASSERT_EQ(lua["st"]["fst"], std::string("first"));
    ASSERT_EQ(lua["st"]["snd"], (double)2);
    ASSERT_EQ(lua["st"]["thr"], true);
    ASSERT_EQ(lua["st"]["del"], sol::nil);
    ASSERT_EQ(lua["st"]["nex"], sol::nil);

    auto res2 = lua.script(R"(
st[1] = 3
st[2] = "number"
st[-1] = false
st[42] = "answer"
st[42] = nil
st.deleted = st[42] == nil
)");
    if (!res2.valid()) {
        FAIL() << "Set res2 failed";
    }

    ASSERT_EQ(lua["st"][1], 3);
    ASSERT_EQ(lua["st"][2], std::string("number"));
    ASSERT_EQ(lua["st"][-1], false);
    ASSERT_EQ(lua["st"]["deleted"], true);

    auto res3 = lua.script(R"(
st[true] = false
st[false] = 9
)");
    if (!res3.valid()) {
        FAIL() << "Set res3 failed";
    }

    ASSERT_EQ(lua["st"][true], false);
    ASSERT_EQ(lua["st"][false], 9);
}

TEST(sharedTable, asGlobalTable) {
    sol::state lua;
    SharedTable st;
    SharedTable::bind(lua);

    lua["st"] = &st;

    lua.script(R"(
_G = st
n = 1
b = false
s = "Oo"
)");

    ASSERT_EQ(lua["n"], 1);
    ASSERT_EQ(lua["b"], false);
    ASSERT_EQ(lua["s"], std::string("Oo"));
}

TEST(sharedTable, severalStates) {
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

    ASSERT_EQ(lua2["dogs"]["fluffy"], std::string("gav"));
    ASSERT_EQ(lua2["dogs"]["sparky"], false);
    ASSERT_EQ(lua2["dogs"]["wow"], 3);
}

TEST(sharedTable, multipleThreads) {
    SharedTable st;

    std::vector<std::thread> threads;

    threads.emplace_back([&](){
        sol::state lua;
        bootstrapState(lua);;
        lua["st"] = &st;
        lua.script(R"(
while not st.ready do end
st.fst = true)");
    });

    threads.emplace_back([&](){
        sol::state lua;
        bootstrapState(lua);
        lua["st"] = &st;
        lua.script(R"(
while not st.ready do end
st.snd = true)");
    });

    threads.emplace_back([&](){
        sol::state lua;
        bootstrapState(lua);
        lua["st"] = &st;
        lua.script(R"(
while not st.ready do end
st.thr = true)");
    });

    sol::state lua;
    bootstrapState(lua);
    lua["st"] = &st;
    lua.script("st.ready = true");

    for(auto& thread : threads) { thread.join(); }

    ASSERT_EQ(lua["st"]["fst"], true);
    ASSERT_EQ(lua["st"]["snd"], true);
    ASSERT_EQ(lua["st"]["thr"], true);
}

TEST(sharedTable, nestedTables) {
    SharedTable recursive, st1, st2;
    sol::state lua;
    bootstrapState(lua);

    lua["recursive"] = &recursive;
    lua["st1"] = &st1;
    lua["st2"] = &st2;

    lua.script(R"(
st1.proxy = st2
st1.proxy.value = true
recursive.next = recursive
recursive.val = "yes"
)");
    ASSERT_EQ(lua["st2"]["value"], true);
    ASSERT_EQ(lua["recursive"]["next"]["next"]["next"]["val"], std::string("yes"));
}

TEST(sharedTable, playingWithFunctions) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = &st;

    lua.script(R"(
st.fn = function ()
    print "Hello C++"
    return true
end
st.fn()
)");

    sol::function sf = lua["st"]["fn"];
    ASSERT_TRUE((bool)sf());

    sol::state lua2;
    bootstrapState(lua2);

    lua2["st2"] = &st;
    lua2.script(R"(
st2.fn2 = function(str)
    return "*" .. str .. "*"
end
)");

    sol::function sf2 = lua["st"]["fn2"];

    ASSERT_EQ(sf2(std::string("SUCCESS")).get<std::string>(), std::string("*SUCCESS*"));
}
