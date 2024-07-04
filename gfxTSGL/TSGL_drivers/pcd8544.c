#include "../TSGL.h"

static tsgl_driver_list _invert(const tsgl_driver_storage* storage, bool invert) {
    if (invert) {
        tsgl_driver_list list = {
            .list = {
                {0x0C + 1, {0}, 0, -1}
            }
        };
        return list;
    } else {
        tsgl_driver_list list = {
            .list = {
                {0x0C, {0}, 0, -1}
            }
        };
        return list;
    }
}

const tsgl_driver pcd8544 = {
    .colormode = tsgl_monochrome,
    .invert = _invert,
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