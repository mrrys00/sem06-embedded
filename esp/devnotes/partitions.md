# Partitions problems
Following error might happen:
```
Error: app partition is too small for binary esp32_radio.bin size 0x104820:
  - Part 'factory' 0/0 @ 0x10000 size 0x100000 (overflow 0x4820)
```
That means your app has grown too large. Try this:

## Solution 1 - easy way
Use *menuconfig* tool to set **Partition Table** to **Single factory app (large), no OTA**. This will enlarge size for your app by 50%

## Solution 2 - hard way
Still not enough? Use *menuconfig* to set custom partition table. Create a `.csv` file in your project and paste the following:
```csv
# Name,   Type, SubType, Offset,  Size, Flags
# Note: if you change the phy_init or app partition offset, make sure to change the offset in Kconfig.projbuild
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 3M,
```
*You can change 3M up/down depending on further needs; don't change the Offset column*

Then, in *menuconfig* set **Partition Table** to **Custom partition table CSV** and select the file you've created

## Solution 3 - improvise
Refer to https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html and improvise

