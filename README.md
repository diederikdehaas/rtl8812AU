# rtl8812AU
This repository is a fork of diederikdehaas' rtl8812AU repository, branch 4.3.22-beta.

I have tried using several different versions (GitHub repositories) of the rtl8812AU driver, but they don't support 802.11ac in AP mode (Hostapd). Only 802.11n is usable and attempting to use 802.11ac will cause Hostapd to exit with the error `Invalid argument`.

After inspecting the code, I found that Realtek probably didn't write the function for specifying the VHT capabilities of the NIC. cfg80211 believed that the NIC was not capable of 802.11ac. Therefore, I added two functions which are `rtw_cfg80211_init_vht_capab()` and `rtw_cfg80211_init_vht_capab_ex()`.

This driver should work well with AC433 (80 MHz, 1T1R). For AC867 or above (160 MHz/80+80 MHz, more than 1T1R), additional tweaks or changes to the code may be needed to achieve the respective speed.

The driver has been successfully compiled on Ubuntu kernel 4.4.
 
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
# DRV_VERSION=4.3.22-beta
# mkdir /usr/src/${DRV_NAME}-${DRV_VERSION}
# git archive driver-${DRV_VERSION} | tar -x -C /usr/src/${DRV_NAME}-${DRV_VERSION}
# dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
# dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
```
Whereby it is assumed you're in the cloned project directory and using the branch `driver-4.3.22-beta`. If you want to use another driver version, adjust `DRV_VERSION` accordingly.

If you later on want to remove it again, do the following:
```
# DRV_NAME=rtl8812AU
# DRV_VERSION=4.3.22-beta
# dkms remove ${DRV_NAME}/${DRV_VERSION} --all
```

### Notes
Realtek built regulatory compliance into the driver (`wifi_regd.c`), which is too restrictive and disables many channels. This means that users may encounter problems when they try to set up an AP in 802.11ac mode. Users may want to comment out the related function in `wifi_regd.c` as follows:
```
int rtw_regd_init(_adapter * padapter)
{
/*
	struct wiphy *wiphy = padapter->rtw_wdev->wiphy;

#if 0
	if (rtw_regd == NULL) {
		rtw_regd = (struct rtw_regulatory *)
		    rtw_malloc(sizeof(struct rtw_regulatory));

		rtw_regd->alpha2[0] = '9';
		rtw_regd->alpha2[1] = '9';

		rtw_regd->country_code = COUNTRY_CODE_USER;
	}

	DBG_8192C("%s: Country alpha2 being used: %c%c\n",
		  __func__, rtw_regd->alpha2[0], rtw_regd->alpha2[1]);
#endif

	_rtw_regd_init_wiphy(NULL, wiphy);
*/

	return 0;
}
```
According to [this](https://wireless.wiki.kernel.org/en/developers/regulatory/processing_rules), I believe that *cfg80211* + *CRDA* + *wireless-regdb* should be able to help users comply with the wireless regulatory and ensure that the device is operated legally. (Please correct me if I'm wrong.)

You may not need to edit that file if you don't use AP mode or you want to play safe. My commit keeps the **original** version of `wifi_regd.c`.

### Disclaimer
- I'm not a programmer and therefore I modified the driver in an **unprofessional** way. Because of my very limited knowledge of C language, I don't know how to enable the respective VHT capabilities of AC867 and above. (To be more specific, I'm not sure how I can detect the channel bandwidth / available antennas and instruct the driver to add the capabilities.)
- **Please forgive me if you find any bugs.**
- Driver maintenance and availability of this repository are **_not_** guaranteed.