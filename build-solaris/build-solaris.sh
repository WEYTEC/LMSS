#!/bin/sh

set -e

cd ..

echo "building project"
./build.sh

echo "preparing folder structure in build/solaris"
mkdir -p build/solaris/etc/xdg/autostart
mkdir -p build/solaris/usr/bin

echo "copy files to build/solaris"
cp install/lmss.desktop build/solaris/etc/xdg/autostart/
cp build/lmss build/solaris/usr/bin

echo "linting pkg config"
if pkglint -c build/solaris-reference -r http://pkg.oracle.com/solaris/release build-solaris/lmss.p5m.3.res
then
    echo "You can now publish the lmss-pkg to your repo"
else
    echo "Something went wrong"
fi
