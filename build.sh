#!/bin/sh

### WARNING: DO NOT CHANGE CODES from HERE !!! ###
#import setup
cd `dirname $0`
_PWD=`pwd`
pushd ./ > /dev/null
while [ ! -f "./xo-setup.conf" ]
do
    cd ../
    SRCROOT=`pwd`
    if [ "$SRCROOT" == "/" ]; then
        echo "Cannot find xo-setup.conf !!"
        exit 1
    fi
done
popd > /dev/null
. ${SRCROOT}/xo-setup.conf
cd ${_PWD}
### WARNING: DO NOT CHANGE CODES until HERE!!! ###

CFLAGS="${CFLAGS}"
export VERSION=1.0

if [ "$MACHINE" == "volans" ]; then
    #export DEVELOP_VER=yes
    CFLAGS="${CFLAGS} -D__VOLANS"
fi


if [ $1 ];
then
    run make $1 || exit 1
else
    mkdir -p /opt/etc/
#    mkdir -p /opt/data/.debug/symbol/
		if [ -z "$USE_AUTOGEN" ]; then
	    run ./autogen.sh || exit 1
	    run ./configure --prefix=$PREFIX || exit 1
		fi
    run make || exit 1
    run make install || exit 1
    mkdir -p ${PREFIX}/bin/
    if [ "$DISTRO" = "vodafone-sdk" ]; then
        make_pkg_option="{make_pkg_option} --postjob=./make_openapi.sh"
    fi
    run make_pkg.sh ${make_pkg_option} || exit 1  	
fi

