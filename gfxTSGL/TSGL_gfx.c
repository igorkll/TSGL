#include "TSGL.h"
#include "TSGL_gfx.h"
#include "TSGL_color.h"

void tsgl_gfx_rect(void* arg, TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    fill(arg, x, y, width, stroke, color);
    fill(arg, x, (y + height) - stroke, width, stroke, color);
    fill(arg, x, y + 1, stroke, height - 2, color);
    fill(arg, (x + width) - stroke, y + 1, stroke, height - 2, color);
}

void tsgl_gfx_line(void* arg, TSGL_GFX_SET_REFERENCE(set), TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_pos strokeD = stroke / 2;
    tsgl_pos fx = (x1 > x2 ? x2 : x1) - (stroke / 2);
    tsgl_pos fy = (y1 > y2 ? y2 : y1) - (stroke / 2);
    if (y1 == y2) {
        fill(arg, fx, fy, abs(x2 - x1), stroke, color);
        return;
    } else if (x1 == x2) {
        fill(arg, fx, fy, stroke, abs(y2 - y1), color);
        return;
    }
    tsgl_pos inLoopValueFrom;
    tsgl_pos inLoopValueTo;
    tsgl_pos outLoopValueFrom;
    tsgl_pos outLoopValueTo;
    bool isReversed;
    tsgl_pos inLoopValueDelta = abs(x2 - x1);
    tsgl_pos outLoopValueDelta = abs(y2 - y1);
    if (inLoopValueDelta < outLoopValueDelta) {
        tsgl_pos t = inLoopValueDelta;
        inLoopValueDelta = outLoopValueDelta;
        outLoopValueDelta = t;

        inLoopValueFrom = y1;
        inLoopValueTo = y2;
        outLoopValueFrom = x1;
        outLoopValueTo = x2;
        isReversed = true;
    } else {
        inLoopValueFrom = x1;
        inLoopValueTo = x2;
        outLoopValueFrom = y1;
        outLoopValueTo = y2;
        isReversed = false;
    }

    if (outLoopValueFrom > outLoopValueTo) {
        tsgl_pos t = inLoopValueFrom;
        inLoopValueFrom = inLoopValueTo;
        inLoopValueTo = t;

        t = outLoopValueFrom;
        outLoopValueFrom = outLoopValueTo;
        outLoopValueTo = t;
    }

    tsgl_pos outLoopValue = outLoopValueFrom;
    tsgl_pos outLoopValueCounter = 1;
    float outLoopValueTriggerIncrement = (float)inLoopValueDelta / (float)outLoopValueDelta;
    float outLoopValueTrigger = outLoopValueTriggerIncrement;

    for (tsgl_pos inLoopValue = inLoopValueFrom; inLoopValue <= inLoopValueTo; inLoopValue += (inLoopValueFrom < inLoopValueTo ? 1 : -1)) {
        if (stroke > 1) {
            if (isReversed) {
                fill(arg, outLoopValue - strokeD, inLoopValue - strokeD, stroke, stroke, color);
            } else {
                fill(arg, inLoopValue - strokeD, outLoopValue - strokeD, stroke, stroke, color);
            }
        } else {
            if (isReversed) {
                set(arg, outLoopValue, inLoopValue, color);
            } else {
                set(arg, inLoopValue, outLoopValue, color);
            }
        }

        outLoopValueCounter++;
        if (outLoopValueCounter > outLoopValueTrigger) {
            outLoopValue++;
            outLoopValueTrigger += outLoopValueTriggerIncrement;
        }
    }
}