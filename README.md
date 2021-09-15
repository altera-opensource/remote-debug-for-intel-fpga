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
