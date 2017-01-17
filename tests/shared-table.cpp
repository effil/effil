#include <boost/test/unit_test.hpp>

#include "shared-table.h"
#include "lua-utils.h"

#include <logging.h>

using namespace core;

BOOST_AUTO_TEST_CASE(singleThreadSet) {
    sol::state lua;
    bootstrapState(lua);

    SharedTable st;

    lua["st"] = &st;

    auto res1 = lua.script(R"(
st.fst = "first"
st.snd = 2
st.thr = true
st.del = "secret"
st.del = nil
)");
    if (!res1.valid()) {
        BOOST_FAIL("Set res1 failed");
    }

    BOOST_CHECK(lua["st"]["fst"] == std::string("first"));
    BOOST_CHECK(lua["st"]["snd"] == (double)2);
    BOOST_CHECK(lua["st"]["thr"] == true);
    BOOST_CHECK(lua["st"]["del"] == sol::nil);
    BOOST_CHECK(lua["st"]["nex"] == sol::nil);

    auto res2 = lua.script(R"(
st[1] = 3
st[2] = "number"
st[-1] = false
st[42] = "answer"
st[42] = nil
st.deleted = st[42] == nil
)");
    if (!res2.valid()) {
        BOOST_FAIL("Set res2 failed");
    }

    BOOST_CHECK(lua["st"][1] == 3);
    BOOST_CHECK(lua["st"][2] == std::string("number"));
    BOOST_CHECK(lua["st"][-1] == false);
    BOOST_CHECK(lua["st"]["deleted"] == true);

    auto res3 = lua.script(R"(
st[true] = false
st[false] = 9
)");
    if (!res3.valid()) {
        BOOST_FAIL("Set res3 failed");
    }

    BOOST_CHECK(lua["st"][true]  == false);
    BOOST_CHECK(lua["st"][false] == 9);
}

BOOST_AUTO_TEST_CASE(asGlobalTable) {
    sol::state lua;
    SharedTable st;
    bootstrapState(lua);

    lua["st"] = &st;

    lua.script(R"(
_G = st
n = 1
b = false
s = "Oo"
)");

    BOOST_CHECK(lua["n"] == 1);
    BOOST_CHECK(lua["b"] == false);
    BOOST_CHECK(lua["s"] == std::string("Oo"));
}

BOOST_AUTO_TEST_CASE(severalStates) {
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

    BOOST_CHECK(lua2["dogs"]["fluffy"] == std::string("gav"));
    BOOST_CHECK(lua2["dogs"]["sparky"] == false);
    BOOST_CHECK(lua2["dogs"]["wow"] == 3);
}

BOOST_AUTO_TEST_CASE(multipleThreads) {
    SharedTable st;

    std::vector<std::thread> threads;

    threads.emplace_back([&](){
        sol::state lua;
        bootstrapState(lua);
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

    BOOST_CHECK(lua["st"]["fst"] == true);
    BOOST_CHECK(lua["st"]["snd"] == true);
    BOOST_CHECK(lua["st"]["thr"] == true);
}

BOOST_AUTO_TEST_CASE(nestedTables) {
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
    BOOST_CHECK(lua["st2"]["value"] == true);
    BOOST_CHECK(lua["recursive"]["next"]["next"]["next"]["val"] == std::string("yes"));
}

BOOST_AUTO_TEST_CASE(playingWithFunctions) {
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
    BOOST_CHECK((bool)sf());

    sol::state lua2;
    bootstrapState(lua2);

    lua2["st2"] = &st;
    lua2.script(R"(
st2.fn2 = function(str)
    return "*" .. str .. "*"
end
)");

    sol::function sf2 = lua["st"]["fn2"];

    BOOST_CHECK(sf2(std::string("SUCCESS")).get<std::string>() == std::string("*SUCCESS*"));
}
