#! /bin/sh
cpp_workspace=$CPP_WORKSPACE
siitframe_path=$SIITFRAME_PATH/crossplatform-1.1
library_version=1.0.1

echo $CPP_WORKSPACE
echo ${cpp_workspace}
echo ${siitframe_path}
echo ${library_version}

#network
mkdir $(pwd)/../build
mkdir $(pwd)/../bin

cd $(pwd)/../build

rm -r Release
mkdir Release
cd Release

mkdir ${cpp_workspace}/common/quartz-${library_version}
mkdir ${cpp_workspace}/common/quartz-${library_version}/lib
mkdir ${cpp_workspace}/common/quartz-${library_version}/include
mkdir ${cpp_workspace}/common/quartz-${library_version}/include/quartz

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ../../quartz
make
cp ${cpp_workspace}/bin/Release/libquartz.so ${cpp_workspace}/common/quartz-${library_version}/lib/libquartz.so

#
cp ${cpp_workspace}/bin/Debug/libquartzd.so ${siitframe_path}/foundation-${library_version}/lib/libquartzd.so
cp ${cpp_workspace}/bin/Release/libquartz.so ${siitframe_path}/foundation-${library_version}/lib/libquartz.so

#
cp -r ${cpp_workspace}/quartz/include/* ${siitframe_path}/quartz-${library_version}/include/quartz
#
cd .. #build
cd .. #

