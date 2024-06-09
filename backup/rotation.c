tsgl_pos ox = x;
    tsgl_pos oy = y;
    tsgl_pos x2 = 0;
    tsgl_pos y2 = 0;
    switch (display->rotation) {
        case 0:
            x2 = (x + width) - 1;
            y2 = (y + height) - 1;
            break;
        case 1:
            x = display->defaultWidth - y - height;
            x2 = display->defaultWidth - y - 1;
            y = ox;
            y2 = (ox + width) - 1;
            break;
        case 2:
            x2 = display->defaultWidth - x - 1;
            y2 = display->defaultHeight - y - 1;
            x = (x2 - width) + 1;
            y = (y2 - height) + 1;
            break;
        case 3:
            y = display->defaultHeight - x - width;
            x = oy;
            x2 = (x + height) - 1;
            y2 = (y + width) - 1;
            break;
    }
    //printf("_select(%i %i %i %i %i)\n", display->rotation, ox, oy, width, height);
    //printf("output:%i %i %i %i\n", x, y, x2, y2);