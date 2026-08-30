#!/bin/bash
#
# This is a means of restarting the USB system on Linux without powering down.
# Obviously, this can be dangerous if, for example, you have USB attached disks!
#
# This MUST be run as sudo.
# The host controller must be identified using: lspci -D | grep -i usb (look for USB xHCI Host Controller).
# In the script below, this is hardwired as: 0000:00:14.0
# Note that after the unbind, USB devices (including keyboard and mouse) will not work! Until the bind ...
#
# It seems that this WILL FIX the dreaded LIBUSB_ERROR_IO if that crops up.
#
echo -n "0000:00:14.0" | sudo tee /sys/bus/pci/drivers/xhci_hcd/unbind
sleep 10
echo -n "0000:00:14.0" | sudo tee /sys/bus/pci/drivers/xhci_hcd/bind
sleep 5
echo "Done."
