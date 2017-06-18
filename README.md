#### Install Linux
1. change to kernel source directory

    cd linux-3.2.30
2. generate .config file

    make menuconfig

    (**Notice**:execute this command under native OS instead of domain0)

3. compile the kernel and modules

    make -j16 bzImage && make -j16 modules

4. install the kernel and modules

    make modules_install && make install

#### Install Xen
1. change to Xen source directory

    cd xen-4.2.1

2. check all the dependencies

    ./configure --libdir=/lib64

    (the necessary packages are dev86 and yajl-devel)

3. compile the Xen VMM and management tools

    make xen tools

4. install the Xen VMM and management tools

    make install-xen install-tools
5. modify the grub file

**Notice**: The Linux and Xen source in this project are already patched with Perfctr-xen. For more information, please refer to [Perctr-xen guide](http://people.cs.vt.edu/~rnikola/?page_id=23).
