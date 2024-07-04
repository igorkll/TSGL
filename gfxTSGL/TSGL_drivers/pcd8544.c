#include "../TSGL.h"

const tsgl_driver pcd8544 = {
    .colormode = tsgl_mono8_ver,
    .init = {
        {0x21, {0}, 0, 0},
        {0x10 + 4, {0}, 0, 0},
        {0x04 + 0, {0}, 0, 0},
        {0xB8, {0}, 0, 0},
        {0x20, {0}, 0, 0},
        {0x0C, {0}, 0, 0},
        {0x80, {0}, 0, 0},
        {0x40, {0}, 0, -1}
    }
};