# rtl8812AU
This repository contains drivers for the rtl8812AU and rtl8821AU/rtl8811AU chipsets.

It started out because I found a newer driver then I found elsewhere, namely version 4.3.8.
But since then I found even newer versions released by Realtek and decided to use/update those too.  
If you find a newer version then I have, I'd appreciate it if you would share it.

Currently there are the following branches, named after the initial Realtek driver version.
- [version 4.3.8](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.8)
- [version 4.3.14](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.14)
- [version 4.3.22-beta](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.22-beta)

The driver (versions) are primarily targetted at [Debian](https://www.debian.org) and [Raspbian](https://www.raspbian.org), but I have no reason to assume it won't work for other distrubutions. If fixes are needed for other distrubution with no negative effect on Debian or Raspbian, I'll gladly incorporate those into my repository.

I have currently successfully compiled the driver on Debian kernel 3.2, 3.16, 4.1-4.8 and on Raspbian with kernel 3.18, 4.1 and 4.4.

## DKMS
[DKMS](http://linux.dell.com/dkms/) is a system which will automatically recompile and install a kernel module when a new kernel gets installed or updated.
To make use of DKMS, install the `dkms` package, which on Debian (based) systems is done like this:
```
# apt-get install dkms
```
Where '#' denotes that it should be executed as root or with sudo, but don't type that character.

To make use of the DKMS feature with this project, do the following:
```
# DRV_NAME=rtl8812AU
# DRV_VERSION=4.3.14
# mkdir /usr/src/${DRV_NAME}-${DRV_VERSION}
# git archive driver-${DRV_VERSION} | tar -x -C /usr/src/${DRV_NAME}-${DRV_VERSION}
# dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
```
Whereby it is assumed you're in the cloned project directory and the current branch is `driver-4.3.14` (the default). If you want to use another driver version, adjust `DRV_VERSION` accordingly.

If you later on want to remove it again, do the following:
```
# DRV_NAME=rtl8812AU
# DRV_VERSION=4.3.14
# dkms remove ${DRV_NAME}/${DRV_VERSION} --all
```
