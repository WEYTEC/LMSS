# LMSS - Mouse Switching Software
> Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for
> Linux and Unix OS


This software enables mouse switching for Linux/Unix based operating systems.
This software scans the mouse’s position and sends the appropriate commands to
the WEYTEC devices via the USB Interface.
The software needs to be installed on all source PCs to enable the switching
between the PCs by moving the mouse to the edge of the desktop.

The software itself needs no configuration, USB devices are detected
automatically.

The system configuration must be performed either over the Smart Touch Tool or
the WDP Configuration Console or in case of the USB Deskswitch III, with the
Deskswitch setup tool.

It works with the following products of WEY Technology AG:

- WEYTEC IP Remote Transmitter family
    * IP Remote I Transmitter (Part No. 24870T)
    * IP Remote II DVI Transmitter (Part No. 24872T)
    * IP Remote II DP Transmitter (Part No. 24872TDP)
    * IP Remote III 4K Transmitter (Part No. 24873T)
    * IP Remote IV Transmitter (Part No. 24874T)

- WEYTEC USB Deskswitch III (Part No: 22412S)

## Installing

Installing lmss through the rpm or deb package will enable lmss autostart.

### Graphical Package Managers

Download the latest binary [release](https://github.com/WEYTEC/LMSS/releases)
suitable for your distro (deb/rpm based) and install the package with  your
favourite package manager.
On most distributions just navigate to your downloaded package and _double
click_ it, a graphical package manager will guide you through the install
process.

**NOTE**: On some distributions (e.g. Ubuntu) you must choose _`download file`_
in your browser, as _`Open with -> Software Install`_ will fail with a message
like: "Failed to install file: not supported"

### Terminal Package Managers

deb-based (Ubuntu, Debian, ...): 

``` shell
sudo apt install /PATH/TO/PACKAGE.deb
```

or

``` shell
sudo dpkg -i /PATH/TO/PACKAGE.deb
sudo apt install -f
```

rpm-based (CentOS, REL, Fedora, ...): 

``` shell
sudo yum install /PATH/TO/PACKAGE.rpm
```

### Manual Installation

If you decide to do a manual install you will need a terminal with root
permissions, the `lmss` executable and a `lmss.desktop` file. 

**The executable** can be build or downloaded from
[releases](https://github.com/WEYTEC/LMSS/releases).
If you decided to download the binary release `.xz` archive, navigate to the
containing dir - in this example it will be the users download directory - and
decompress it.

```sh
cd ~/Downloads
xz -kd lmss-4.0.4-Linux-bin.xz
```

now you should find a executable named `lmss-4.0.4-Linux-bin`, move it to the
install location and change its name and file permissions to enable running it
in a user session context.

```sh
sudo mv lmss-4.0.4-Linux-bin /usr/bin/lmss
sudo chmod u+s /usr/bin/lmss
```

**The .desktop file** can be downloaded from the
[repository](https://github.com/WEYTEC/LMSS/blob/main/install/lmss.desktop) and
needs to be placed in on of the XDG autostart directories. In this example we
will use the global autostart directory.

```sh
sudo mv lmss.desktop /etc/xdg/autostart/lmss.desktop
```

**The udev.rules file** is needed since v4.0.x and can be downloaded from
[releases](https://github.com/WEYTEC/LMSS/releases). It shall be moved to the 
udev-rules.d dir, e.g. `/usr/lib/udev/rules.d/`

```sh
sudo mv 100-wey-usbhid.rules /usr/lib/udev/rules.d/100-wey-usbhid.rules
```

**_After logoff/login_** lmss should autostart with your user session.


## Autostart

lmss uses the autostart feature described in the Desktop Application Autostart
Specification on
[freedesktop.org](https://specifications.freedesktop.org/autostart-spec/0.5/index.html) 

The following is only necessary if lmss was manually installed. The installation
through the rpm or deb package automatically starts lmss.

### How to enable the autostart feature globally

Place
[lmss.dektop](https://github.com/WEYTEC/LMSS/blob/main/install/lmss.desktop) in
your global XDG autostart dir `/etc/xdg/autostart` and make sure your lmss
binary is installed correctly.

### How to enable the autostart feature per user

If you do not want to enable this feature globaly, you need to move the
`lmss.desktop` file from the `/etc/xdg/autostart` directory and place it in the
user autostart dir `~/.config/autostart/` for all user accounts you want this
feature to be enabled on.


### How to disable autostart feature per user

Just place a file called `lmss.desktop` containing `Hidden=true` in the users
home `~/.config/autostart/`.

## Tested Distributions

* Ubuntu 20.04 LTS, 22.04 LTS
* CentOS 7, 8
* AlmaLinux 8.9
* Rocky Linux 8
* RHEL® 7.9

Other distributions should work too, as long as X.org is used as session window
system. The installation of the software / autostart / rights might differ per
distribution. Please provide feedback and feel free to provide PR with changes
that are needed to make the software run on other distributions and versions.

## Building

### Dependencies
 - C++20 compatible compiler (gcc >= 10)
 - cmake
 - libxi-dev
 - libxrandr-dev

### Compiling

``` shell
git clone https://github.com/WEYTEC/LMSS.git 
cd LMSS
./build.sh
```

### Packaging

``` shell
cd build
cpack -G DEB
cpack -G RPM
```

## Debugging

The verbosity of log messages can be set with the `-v` command line argument. To
enable the most verbose output (debug level) set `-v` to level 7:

``` shell
lmss -v 7
```

See the usage output for more options:

``` shell
lmss --help
```

### Verbosity levels

| Level | Description   |
|-------|---------------|
| 1     | Alert         |
| 2     | Critical      |
| 3     | Error         |
| 4     | Warning       |
| 5     | Notice        |
| 6     | Informational |
| 7     | Debug         |

## Known Limitations

* requires X.org as session window system at the moment
* on some distributions Mouse Switching does not work on the login screen, due to missing user rights

