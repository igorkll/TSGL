# TSGL 0.4.1 - BETA
* extended graphics library for the ESP 32 family controller
* this library is written entirely in "C"
* designed exclusively for use with ESP-IDF
* the esp-idf version i use is v5.4.1
* also, this library has an APIs for working with other peripherals

## supported platforms
* esp32
* esp32c3

## supported displays
* st7735
* st7789
* st7796
* pcd8544 (nokia 84x48 display)

## supported ST77XX colormodes
* st7735 - st7735_rgb444 / st7735_rgb565 / st7735_rgb666
* st7789 - st77XX_rgb444 / st77XX_rgb565 / st77XX_rgb666
* st7796 - st77XX_rgb565 / st77XX_rgb888

## features
* the ability to install custom drivers without having to change the library code
* the ability to install drivers directly inside the project
* the ability to use different color spaces such as high color and true color to choose from
* the ability to change driver settings for specific display features, which allows you to use 1 driver for most displays (can configure: resolution, offsets, inversion, flipX, flipY, swapXY)
* are red and blue mixed up? this is not a problem, just send the color encoded on the display not using the driver's color space, but its BGR/RGB counterpart. or set the swapRGB flag in the driver settings
* support for the 444 color space, where 3 pixels are encoded in 2 bits (accessing such a framebuffer is slower, and working without framebuffer can cause graphical artifacts)

## warnings
