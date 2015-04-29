#!/bin/bash

# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  SELF="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

DEPS_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
LOG=${DEPS_DIR}/../install_deps.log
AUX_LIBS=${DEPS_DIR}/isc-dhcp/bind

BIND_DIR=${DEPS_DIR}/isc-dhcp/bind

BIND_SRC_DIR=${DEPS_DIR}/isc-dhcp/bind/bind-expanded-tar
ISC_DIR=${DEPS_DIR}/isc-dhcp

if [ "$1" == "rebuild" ]; then
	REBUILD=1
fi 

touch $LOG
pushd $BIND_DIR

if [ -e ${BIND_SRC_DIR}/Makefile ] && [ -z $REBUILD ]; then                  # ;
	echo "Bind export libraries already configured"   
else                                                       
	echo "Configuring BIND Export libraries for DHCP."
	mkdir -p build
	rm -rf ./lib ./include ./configure.log ./build.log ./install.log
	pushd ${BIND_SRC_DIR}
	./configure --prefix=${BIND_DIR}/build CFLAGS="-fPIC" --disable-kqueue --disable-epoll --disable-devpoll --without-openssl --without-libxml2 --enable-exportlib --enable-threads=no --with-export-includedir=${BIND_DIR}/include --with-export-libdir=${BIND_DIR}/lib --with-gssapi=no > ${BIND_DIR}/configure.log
	popd
fi

if [ -e "${DEPS_DIR}/isc-dhcp/bind/lib/libdns.a" ] && [ -z $REBUILD ]; then
	echo "Bind libraries already built."
else
	echo "Building libraries"
	pushd ${BIND_SRC_DIR}
	make
	make install
	rm -rf ${BIND_DIR}/build
	popd
fi

popd

pushd $ISC_DIR
	
./configure --prefix=${BIND_DIR}/build CFLAGS="-fPIC"  > ${BIND_DIR}/configure.log

popd
