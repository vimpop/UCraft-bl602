# UCraft for BL602

This repository contains an implementation of the UCraft Minecraft server adapted to run on the BL602 microcontroller.
## Configuration
The defualt SSID and PSK that the server connects to is defined under `main.c`
```
#define SSID "Einstein"
#define PSK "germany101"
```
This should be changed according to your WIFI access point.

For UCraft specifc configuration, they can be found under the `app_ucraft/UCraft/` folder
## Building
The build system only supports linux due to the BL602 SDK (WSL works). So all steps below must be conducted in a linux enviorment with make installed (this can be installed by the `build-essential` package on ubuntu).
Start by cloning the BL602 SDK and setting up the build enviorment:
```
git clone https://github.com/pine64/bl_iot_sdk.git
cd bl_iot_sdk
export BL60X_SDK_PATH="$(pwd)"
export CONFIG_CHIP_NAME=BL602
```
Next clone this repository **outside** of the cloned sdk folder
```
git clone --recurse-submodules https://github.com/vimpop/UCraft-bl602.git
cd UCraft-bl602
chmod +x ./genromap
./genromap
```
This will produce an error like message but its usually fine if you see the `Generating BIN file` message below (in this case pip is not installed but even if it was, it still fails )
```
...
Generating BIN File to /home/vimpo/pine64/UCraft-bl602/build_out/app_ucraft.bin 
/usr/bin/python3: No module named pip
make: *** [/home/vimpo/pine64/bl_iot_sdk/make_scripts_riscv/project.mk:194: all] Error 1
```
Proceed to the next steps if `app_ucraft.bin` is present under the `build_out` folder

## Flashing
This step requires that the BL602 is connected to a UART interface

A tool like [BLDevCube GUI](https://dev.bouffalolab.com/download) can be used to flash the microcontroller or [bl60x-flash](https://github.com/stschake/bl60x-flash) as shown below

```
pip install bl60x-flash
```
Then invoke with serial port and firmware binary, e.g.:
```
bl60x-flash COM4 app_ucraft.bin
```