# rtl8812AU
This repository contains drivers for the rtl8812AU and rtl8821AU/rtl8811AU chipsets.

It started out because I found a newer driver then I found elsewhere, namely version 4.3.8.
But since then I found even newer versions released by Realtek and decided to use/update those too.  

Currently there are the following branches, named after the initial Realtek driver version.
- [version 4.3.8](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.8)
- [version 4.3.14](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.14)
- [version 4.3.20](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.20)
- [version 4.3.22-beta](https://github.com/diederikdehaas/rtl8812AU/tree/driver-4.3.22-beta)
- [version 5.2.20](https://github.com/diederikdehaas/rtl8812AU/tree/driver-5.2.20)
- [version 5.6.4](https://github.com/diederikdehaas/rtl8812AU/tree/driver-5.6.4)

The driver (versions) are primarily targetted at [Debian](https://www.debian.org) and [Raspbian](https://www.raspbian.org), but I have no reason to assume it won't work for other distrubutions. If fixes are needed for other distrubution with no negative effect on Debian or Raspbian, I'll gladly incorporate those into my repository.

I have currently successfully compiled the driver on Debian kernel 3.2, 3.16, 4.1-5.2 and on Raspbian with kernel 3.18, 4.1, 4.4, 4.9 and Raspberry Pi kernel 4.19.

**Note**:
I can reasonably read C code and make light/simple adjustments, but I'm not a C coder. Let alone an experienced one.
If you want/need to have features added to the driver, this repo is probably not for you. I certainly can't code them, but I'm willing to accept PRs if they're not too intrusive. This repo mostly tries to make adjustments so they continue to work on newer kernels.
There is another repo which seem to be maintained by people who (really) know what they're doing and they have added various features: https://github.com/aircrack-ng/rtl8812au.

## Raspberry Pi
There is a section added for compilation on the Raspberry Pi, but it is not enabled by default.  
To enable it, make the following change in the `Makefile`:
```
CONFIG_PLATFORM_I386_PC = y
CONFIG_PLATFORM_ARM_RPI = n
```
to
```
CONFIG_PLATFORM_I386_PC = n
CONFIG_PLATFORM_ARM_RPI = y
```
And then it should compile succesfully **on** the Raspberry Pi.  
At the top of the `Makefile` there are some disabled `CFLAGS` settings which you probably want to enable, by removing the '#' in front of them, when compiling for the Raspberry Pi. You can identify them by the comments above them.
Keep in mind that when using dkms on the Raspberry Pi you need to make the above change before running the dkms installation commands.

## DKMS
[DKMS](http://linux.dell.com/dkms/) is a system which will automatically recompile and install a kernel module when a new kernel gets installed or updated.
To make use of DKMS, install the `dkms` package, which on Debian (based) systems is done like this:
```
# apt-get install dkms
```
Where '#' denotes that it should be executed as root or with sudo, but don't type that character.

To make use of the DKMS feature with this project, do the following as root:
```
# DRV_NAME=rtl8812AU
# DRV_VERSION=4.3.20
# mkdir /usr/src/${DRV_NAME}-${DRV_VERSION}
# git archive driver-${DRV_VERSION} | tar -x -C /usr/src/${DRV_NAME}-${DRV_VERSION}
# dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
```
and the instructions with `sudo` are:

```
$ DRV_NAME=rtl8812AU
$ DRV_VERSION=4.3.20
$ sudo mkdir /usr/src/${DRV_NAME}-${DRV_VERSION}
$ git archive driver-${DRV_VERSION} | sudo tar -x -C /usr/src/${DRV_NAME}-${DRV_VERSION}
$ sudo dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
$ sudo dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
$ sudo dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
```
Whereby it is assumed you're in the cloned project directory and the current branch is `driver-4.3.20`. If you want to use another driver version, adjust `DRV_VERSION` accordingly.

If you later on want to remove it again, do the following as root:
```
# DRV_NAME=rtl8812AU
# DRV_VERSION=4.3.20
# dkms remove ${DRV_NAME}/${DRV_VERSION} --all
```
or with `sudo`:
```
$ DRV_NAME=rtl8812AU
$ DRV_VERSION=4.3.20
$ sudo dkms remove ${DRV_NAME}/${DRV_VERSION} --all
```

