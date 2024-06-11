#pragma once

#define TSGL_GFX_RECT(arg, fill, x, y, width, height, color, strokelen) { \
    fill(arg, x, y, width, strokelen, color); \
    fill(arg, x, (y + height) - strokelen, width, strokelen, color); \
    fill(arg, x, y + 1, strokelen, height - 2, color); \
    fill(arg, (x + width) - strokelen, y + 1, strokelen, height - 2, color); \
}

#define TSGL_GFX_LINE(arg, set, fill, x1, y1, x2, y2, color) { \
    if (y1 == y2) { \
        fill(arg, x1, y1, abs(x2 - x1), 1, color); \
        return; \
    } else if (x1 == x2) { \
        fill(arg, x1, y1, 1, abs(x2 - x1), color); \
        return; \
    } \
    tsgl_pos inLoopValueFrom; \
    tsgl_pos inLoopValueTo; \
    tsgl_pos outLoopValueFrom; \
    tsgl_pos outLoopValueTo; \
    bool isReversed; \
    tsgl_pos inLoopValueDelta = abs(x2 - x1); \
    tsgl_pos outLoopValueDelta = abs(y2 - y1); \
    if (inLoopValueDelta < outLoopValueDelta) { \
        tsgl_pos t = inLoopValueDelta; \
        inLoopValueDelta = outLoopValueDelta; \
        outLoopValueDelta = t; \
 \
        inLoopValueFrom = y1; \
        inLoopValueTo = y2; \
        outLoopValueFrom = x1; \
        outLoopValueTo = x2; \
        isReversed = true; \
    } else { \
        inLoopValueFrom = x1; \
        inLoopValueTo = x2; \
        outLoopValueFrom = y1; \
        outLoopValueTo = y2; \
        isReversed = false; \
    } \
 \
    if (outLoopValueFrom > outLoopValueTo) { \
        tsgl_pos t = inLoopValueFrom; \
        inLoopValueFrom = inLoopValueTo; \
        inLoopValueTo = t; \
 \
        t = outLoopValueFrom; \
        outLoopValueFrom = outLoopValueTo; \
        outLoopValueTo = t; \
    } \
 \
    tsgl_pos outLoopValue = outLoopValueFrom; \
    tsgl_pos outLoopValueCounter = 1; \
    tsgl_pos outLoopValueTriggerIncrement = inLoopValueDelta / outLoopValueDelta; \
    tsgl_pos outLoopValueTrigger = outLoopValueTriggerIncrement; \
 \
    for (tsgl_pos inLoopValue = inLoopValueFrom; inLoopValue <= inLoopValueTo; inLoopValue += (inLoopValueFrom < inLoopValueTo ? 1 : -1)) { \
        if (isReversed) { \
            set(arg, outLoopValue, inLoopValue, color); \
        } else { \
            set(arg, inLoopValue, outLoopValue, color); \
        } \
 \
        outLoopValueCounter++; \
        if (outLoopValueCounter > outLoopValueTrigger) { \
            outLoopValue++; \
            outLoopValueTrigger += outLoopValueTriggerIncrement; \
        } \
    } \
}