# LMSS - Mouse Switching Software
Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for Linux and Unix OS


This software enables mouse switching for Linux/Unix based operating systems.
This software scans the mouse’s position and sends the appropriate commands to the WEYTEC devices via the USB Interface.
The software needs to be installed on all source PCs to enable the switching between the PCs by moving the mouse to the
edge of the desktop.

The software itself needs no configuration, USB devices are detected automatically.

The system configuration must be performed either over the Smart Touch Tool or the WDP Configuration Console or in case
of the USB Deskswitch III, with the Deskswitch setup tool.

It works with the following products of WEY Technology AG:

- WEYTEC IP Remote Transmitter family
    * IP Remote I Transmitter (Part No. 24872T)
    * IP Remote II DVI Transmitter (Part No. 24872T)
    * IP Remote II DP Transmitter (Part No. 24872TDP)
    * IP Remote III 4K Transmitter (Part No. 24873T)
    * IP Remote IV Transmitter (Part No. 24874T)

- WEYTEC USB Deskswitch III (Part No: 22412S)


## Tested Distributions

* Ubuntu 16.04 LTS, 18.04 LTS, 20.04 LTS
* CentOS 7, 8
* IgelOS (work in progress)
* Solaris 11.4

Other distributions should work too, as long as X.org is used as session window system. The installation of the
software / autostart / rights might differ per distribution. Please provide feedback and feel free to provide PR with
changes that are needed to make the software run on other distributions and versions.

## Known Limitations
* requires X.org as session window system at the moment
* on some distributions Mouse Switching does not work on the login screen, due to missing user rights


see [the LMSS wiki](https://github.com/WEYTEC/LMSS/wiki) for more details.
