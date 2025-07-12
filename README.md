# TSGL - ALPHA
* extended graphics library for the ESP 32 family controller
* this library is written entirely in "C"
* designed exclusively for use with ESP-IDF
* the esp-idf version i use is 5.3
* also, this library has an APIs for working with other peripherals

# features
* the ability to install custom drivers without having to change the library code
* the ability to install drivers directly inside the project
* the ability to use different color spaces such as high color and true color to choose from
* the ability to change driver settings for specific display features, which allows you to use 1 driver for most displays (can configure: resolution, offsets, inversion, flipX, flipY, swapXY)
* are red and blue mixed up? this is not a problem, just send the color encoded on the display not using the driver's color space, but its BGR/RGB counterpart. or set the swapRGB flag in the driver settings