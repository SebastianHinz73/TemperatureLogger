# Install Temperature Logger

**Attention: At the moment only one board is supported.** (See [https://www.diy-temperature-logger.com/](https://www.diy-temperature-logger.com/))

There are various ways to install the Temperature Logger. In the case of a completely new installation, OpenDTU can be installed and then updated to the Temperature Logger with the firmware.bin file. In the other case, the script flash.cmd can be used. To update, simply flash the firmware.bin file via the Firmware - Settings menu.

## Installation with flash.cmd

1. Download the files flash.cmd, esptool.exe and firmware.factory.bin from the install folder.
2. Install driver for the board [https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads)

![DeviceManager](../docs/pics/device-manager.png)

3. Run flash.cmd
4. Change the COM port and flash the firmware. When ...... appears on the screen, the boot button on the board must be pressed.

![Install](../docs/pics/install.png)

## Configuration

1. The temperature logger has opened an access point. Connect to that access point.
2. The page http:\\\192.168.4.1 may need to be opened manually in browser.
3. Use admin and openDTU42 as username and password.
4. The router can be entered in the Settings - Network Settings menu.
