# Network setup guide

### Mellanox cards setup guide
ConnectX-4 or newer Mellanox Ethernet NICs are supported.

Following steps should be performed to run DaqDB with this network configuration.
* [Build and install kernel with PMEM and OFED support](#Build-and-install-kernel-with-PMEM-and-OFED-support)
* [Install Mellanox OFED driver](#Install-Mellanox-OFED-driver)
* [Set Ethernet mode](Set-Ethernet-mode)

##### Build and install kernel with PMEM and OFED support
Kernel config file can be taken from [here](config/config_4_15.txt).

Kernel building and installation may be done as below:
```
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.15.tar.xz
tar xvf linux-4.15.tar.xz
cd linux-4.15
cp <config_file> .config
make oldconfig
make -j `nproc` && make modules_install && make install
yum install rpm-build -y
make rpm-pkg
rpm -iUv ~/rpmbuild/RPMS/x86_64/kernel-headers-4.15.0-1.x86_64.rpm
```
Update kernel boot options e.g.
```
vim /etc/default/grub
...
GRUB_CMDLINE_LINUX="crashkernel=auto rd.lvm.lv=centos/root rd.lvm.lv=centos/swap rhgb quiet"
...

grub2-mkconfig -o /etc/grub2-efi.cfg
```
_Note: The highest version that may be used is 4.15
e.g. kernel version 4.17 doesn't support Mellanox OFED._


##### Install Mellanox OFED driver
```
wget http://www.mellanox.com/downloads/ofed/MLNX_EN-4.4-1.0.1.0/mlnx-en-4.4-1.0.1.0-rhel7.5-x86_64.tgz
tar zxf mlnx-en-4.4-1.0.1.0-rhel7.5-x86_64
cd mlnx-en-4.4-1.0.1.0-rhel7.5-x86_64
./install --add-kernel-support --dpdk
```
_Note: It may be necessary to install some extra packages e.g createrepo_

##### Set Ethernet mode
```
wget http://www.mellanox.com/downloads/MFT/mft-4.11.0-103-x86_64-rpm.tgz
tar zxvf mft-4.11.0-103-x86_64-rpm.tgz
cd  mft-4.11.0-103-x86_64-rpm
./install.sh
```
follow [Getting Started with ConnectX-5 100Gb/s Adapters for Linux](https://community.mellanox.com/s/article/getting-started-with-connectx-5-100gb-s-adapters-for-linux)