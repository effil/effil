#!/usr/bin/env bash

set -e

ROOT_DIR=$(cd $(dirname $0); pwd;)
LUA=$1
TARGET_DIR=$2

echo "Installing '$LUA' to '$TARGET_DIR'"

if [ -z "${PLATFORM:-}" ]; then
  PLATFORM=$TRAVIS_OS_NAME;
fi

if [ "$PLATFORM" == "osx" ]; then
  PLATFORM="macosx";
fi

if [ -z "$PLATFORM" ]; then
  if [ "$(uname)" == "Linux" ]; then
    PLATFORM="linux";
  else
    PLATFORM="macosx";
  fi;
fi

if ! [ -d $TARGET_DIR ]; then
  mkdir $TARGET_DIR
fi

cd $TARGET_DIR

modify_file() {
  file_name=$1; shift
  while [[ $# -gt 1 ]]; do
    src=$1; dst=$2
    cat $file_name | sed -e "s/$src/$dst/" > .tmpFile
    mv .tmpFile $file_name
    shift 2
  done
}

if [ "$LUA" == "luajit" ]; then
    LUAJIT_VERSION="2.0.4"
    LUAJIT_BASE="LuaJIT-$LUAJIT_VERSION"

    curl --location https://github.com/LuaJIT/LuaJIT/archive/v$LUAJIT_VERSION.tar.gz | tar xz;
    cd $LUAJIT_BASE
    make && make install PREFIX="$LUA_HOME_DIR"
    ln -s $LUA_HOME_DIR/bin/luajit $HOME/.lua/luajit
    ln -s $LUA_HOME_DIR/bin/luajit $HOME/.lua/lua;
else

  if [ "$LUA" == "lua5.1" ]; then
    curl http://www.lua.org/ftp/lua-5.1.5.tar.gz | tar xz
    cd lua-5.1.5;
  elif [ "$LUA" == "lua5.2" ]; then
    curl http://www.lua.org/ftp/lua-5.2.4.tar.gz | tar xz
    cd lua-5.2.4;
  elif [ "$LUA" == "lua5.3" ]; then
    curl http://www.lua.org/ftp/lua-5.3.3.tar.gz | tar xz
    cd lua-5.3.3;
  fi

  if [ "$PLATFORM" == "linux" ]; then
    SONAME_FLAG="-soname"
    LIB_POSTFIX="so"
  else
    SONAME_FLAG="-install_name"
    LIB_POSTFIX="dylib"
  fi

  # Modify root Makefile
  modify_file Makefile "^\(TO_LIB=.*\)$" "\1 liblua.$LIB_POSTFIX"
  #Modify child Makefile
  modify_file src/Makefile \
      "^\(LUAC_O=.*\)" '\1\'$'\nLUA_SO= liblua.'"$LIB_POSTFIX" \
      "^\(ALL_T=.*\)"  '\1 $(LUA_SO)' \
      "^\(CFLAGS=.*\)" '\1 -fPIC'

  echo '$(LUA_SO): $(CORE_O) $(LIB_O)' >> src/Makefile
  echo -e "\t\$(CC) -shared -ldl -Wl,$SONAME_FLAG,\$(LUA_SO) -o \$@ \$? -lm \$(MYLDFLAGS)" >> src/Makefile

  make $PLATFORM
  make INSTALL_TOP="$TARGET_DIR" install
fi
