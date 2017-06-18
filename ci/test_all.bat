git submodule update --init --recursive
gradlew.bat cpptests_debug luatests_debug
cd build
debug\tests
lua.exe tests.lua
