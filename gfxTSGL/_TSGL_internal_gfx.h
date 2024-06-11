#pragma once

#define TSGL_GFX_RECT(arg, fill, x, y, width, height, color, strokelen) \
fill(arg, x, y, width, strokelen, color); \
fill(arg, x, (y + height) - strokelen, width, strokelen, color); \
fill(arg, x, y + 1, strokelen, height - 2, color); \
fill(arg, (x + width) - strokelen, y + 1, strokelen, height - 2, color)

#define TSGL_GFX_LINE(arg, set, x1, y1, x2, y2, color) \
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
    outLoopValueFrom, outLoopValueTo = outLoopValueTo, outLoopValueFrom
    inLoopValueFrom, inLoopValueTo = inLoopValueTo, inLoopValueFrom
}

local outLoopValue, outLoopValueCounter, outLoopValueTriggerIncrement = outLoopValueFrom, 1, inLoopValueDelta / outLoopValueDelta
local outLoopValueTrigger = outLoopValueTriggerIncrement

for inLoopValue = inLoopValueFrom, inLoopValueTo, inLoopValueFrom < inLoopValueTo and 1 or -1 do
    if (isReversed) {
        set(arg, outLoopValue, inLoopValue, color);
    } else {
        set(arg, inLoopValue, outLoopValue, color);
    }

    outLoopValueCounter = outLoopValueCounter + 1
    if outLoopValueCounter > outLoopValueTrigger then
        outLoopValue, outLoopValueTrigger = outLoopValue + 1, outLoopValueTrigger + outLoopValueTriggerIncrement
    end
end