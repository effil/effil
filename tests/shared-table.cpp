#include <gtest/gtest.h>

#include "shared-table.h"

#include <thread>

using namespace effil;

namespace {

void bootstrapState(sol::state& lua) {
    lua.open_libraries(
            sol::lib::base,
            sol::lib::string,
            sol::lib::table
    );
    SharedTable::getUserType(lua);
}

} // namespace

TEST(sharedTable, primitiveTypes) {
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

    ASSERT_TRUE(res1.valid()) << "Set res1 failed";
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

    ASSERT_TRUE(res2.valid()) << "Set res2 failed";
    ASSERT_EQ(lua["st"][1], 3);
    ASSERT_EQ(lua["st"][2], std::string("number"));
    ASSERT_EQ(lua["st"][-1], false);
    ASSERT_EQ(lua["st"]["deleted"], true);

    auto res3 = lua.script(R"(
st[true] = false
st[false] = 9
)");

    ASSERT_TRUE(res3.valid()) << "Set res3 failed";
    ASSERT_EQ(lua["st"][true], false);
    ASSERT_EQ(lua["st"][false], 9);
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

TEST(sharedTable, playingWithSharedTables) {
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

TEST(sharedTable, playingWithTables) {
    SharedTable st;
    sol::state lua;
    bootstrapState(lua);

    lua["st"] = &st;
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

    ASSERT_TRUE(res.valid());
    ASSERT_EQ(lua["st"]["person"]["name"], std::string("John Doe"));
    ASSERT_EQ(lua["st"]["person"]["age"], 25);
    ASSERT_EQ(lua["st"]["pet"]["type"], std::string("cat"));
    ASSERT_EQ(lua["st"]["pet"]["name"], std::string("Tomas"));
    ASSERT_EQ(lua["st"]["pet"]["real"], std::string("Яша"));
    ASSERT_EQ(lua["st"]["pet"]["spec"]["colour"], std::string("grey"));
    ASSERT_EQ(lua["st"]["pet"]["spec"]["legs"], 4);
    ASSERT_EQ(lua["st"]["recursive"]["prev"]["next"]["next"]["val"], std::string("recursive"));

    defaultPool().clear();
}

TEST(sharedTable, stress) {
    sol::state lua;
    bootstrapState(lua);
    SharedTable st;

    lua["st"] = &st;

    auto res1 = lua.script(R"(
for i = 1, 1000000 do
    st[i] = tostring(i)
end
)");

    ASSERT_TRUE(res1.valid());
    ASSERT_TRUE(st.size() == 1'000'000);

    auto res2 = lua.script(R"(
for i = 1000000, 1, -1 do
    st[i] = nil
end
)");
    ASSERT_TRUE(res2.valid());
    ASSERT_TRUE(st.size() == 0);
}

TEST(sharedTable, stressWithThreads) {
    SharedTable st;

    const size_t threadCount = 10;
    std::vector<std::thread> threads;
    for(size_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&st, thrId(i)] {
            sol::state lua;
            bootstrapState(lua);
            lua["st"] = &st;
            std::stringstream ss;
            ss << "st[" << thrId << "] = 1" << std::endl;
            ss << "for i = 1, 100000 do" << std::endl;
            ss << "    st[" << thrId << "] = " << "st[" << thrId << "] + 1" << std::endl;
            ss << "end" << std::endl;
            lua.script(ss.str());
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    sol::state lua;
    bootstrapState(lua);
    lua["st"] = &st;
    for(size_t i = 0; i < threadCount; i++) {
        ASSERT_TRUE(lua["st"][i] == 100'001) << (double)lua["st"][i] << std::endl;
    }
}
