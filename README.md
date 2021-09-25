################################
#          README              #
################################

To build simple remote-debug-for-intel-fpga:

1. cmake . -Bbuild && cd build
2. make
3. make install (Need sudo permission and this will install executable into /usr/local/bin)

The executable output will be etherlink. 

Available Options:
1. -DCROSS_COMPILE.
    This is to enable cross compile the executable, otherwise it will use default gcc and g++ on your system.
    Passing the CROSS_COMPILE path during execute cmake command, an example shown below.
    
    cmake . -Bbuild -DCROSS_COMPILE=/rocketboard/s10_gsrd/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu- && cd build

2. -DTEST=yes/no.
    This is to generate test cases under test directory.
    Gtest's include and libray path are needed to generate protoDriver test, path can be specified by 
    during cmake configuration like shown below.
    If gtest include path = /rocketboard/googletest/build-arm/install/include
    If gtest lib path     = /rocketboard/googletest/build-arm/install/lib
    cmake . -Bbuild -DTEST=yes -DCMAKE_CXX_FLAGS="$(CMAKE_CXX_FLAGS) -Wall -I /rocketboard/googletest/build-arm/install/include -L /rocketboard/googletest/build-arm/install/lib"
