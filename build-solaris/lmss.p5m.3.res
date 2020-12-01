set name=pkg.fmri value="pkg://WEYTEC/lmss@3.0.0"
set name=pkg.summary value="LMSS Mouse Switching Software"
set name=pkg.description \
    value="Mouse Switching Software for WEYTEC IP Remote and Deskswitch products"
set name=license value="BSD 3-Clause "
set name=variant.arch value=i386
file build/solaris/etc/xdg/autostart/lmss.desktop \
    path=etc/xdg/autostart/lmss.desktop owner=root group=bin mode=0644
file build/solaris/usr/bin/lmss path=usr/bin/lmss owner=root group=bin \
    mode=04755
depend fmri=pkg:/library/libusb-1@1.0.21-11.4.0.0.1.14.0 type=require
depend fmri=pkg:/system/library@11.4-11.4.0.0.1.15.0 type=require
depend fmri=pkg:/x11/library/libx11@1.6.5-11.4.0.0.1.14.0 type=require
depend fmri=pkg:/x11/library/libxrandr@1.5.1-11.4.0.0.1.14.0 type=require
