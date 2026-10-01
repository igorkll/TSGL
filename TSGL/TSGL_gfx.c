#include "TSGL.h"
#include "TSGL_gfx.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_font.h"
#include "TSGL_math.h"
#include <esp_log.h>
#include <string.h>
#include <limits.h>

static const char* TAG = "TSGL_gfx";

void tsgl_gfx_rect(void* arg, TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    fill(arg, x, y, width, stroke, color);
    fill(arg, x, (y + height) - stroke, width, stroke, color);
    fill(arg, x, y + 1, stroke, height - 2, color);
    fill(arg, (x + width) - stroke, y + 1, stroke, height - 2, color);
}

void tsgl_gfx_line(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    if (x1 < minX) x1 = minX; else if (x1 >= maxX) x1 = maxX - 1;
    if (y1 < minY) y1 = minY; else if (y1 >= maxY) y1 = maxY - 1;
    if (x2 < minX) x2 = minX; else if (x2 >= maxX) x2 = maxX - 1;
    if (y2 < minY) y2 = minY; else if (y2 >= maxY) y2 = maxY - 1;

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

void tsgl_gfx_push(void* arg, TSGL_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    sprite->rotation = ((uint8_t)(-sprite->rotation)) % (uint8_t)4;

    if (sprite->sprite->hardwareRotate) {
        ESP_LOGE(TAG, "a sprite cannot have a hardware rotation");
        return;
    }

    tsgl_pos realSpriteWidth;
    tsgl_pos realSpriteHeight;
    switch (sprite->rotation) {
        case 1:
        case 3:
            realSpriteWidth = sprite->sprite->defaultHeight;
            realSpriteHeight = sprite->sprite->defaultWidth;
            break;

        default:
            realSpriteWidth = sprite->sprite->defaultWidth;
            realSpriteHeight = sprite->sprite->defaultHeight;
            break;
    }

    tsgl_pos spriteWidth = realSpriteWidth;
    if (sprite->resizeWidth != 0) spriteWidth = sprite->resizeWidth;
    tsgl_pos spriteHeight = realSpriteHeight;
    if (sprite->resizeHeight != 0) spriteHeight = sprite->resizeHeight;

    tsgl_pos startX = 0;
    tsgl_pos startY = 0;
    if (x < minX) startX = minX - x;
    if (y < minY) startY = minY - y;
    tsgl_pos maxSpriteWidth = maxX - x;
    tsgl_pos maxSpriteHeight = maxY - y;
    tsgl_pos spriteMaxPointX = spriteWidth - 1;
    tsgl_pos spriteMaxPointY = spriteHeight - 1;
    tsgl_pos spriteRealMaxPointX = realSpriteWidth - 1;
    tsgl_pos spriteRealMaxPointY = realSpriteHeight - 1;
    if (spriteWidth > maxSpriteWidth) spriteWidth = maxSpriteWidth;
    if (spriteHeight > maxSpriteHeight) spriteHeight = maxSpriteHeight;
    for (tsgl_pos posX = startX; posX < spriteWidth; posX++) {
        tsgl_pos setPosX = posX + x;
        for (tsgl_pos posY = startY; posY < spriteHeight; posY++) {
            tsgl_pos setPosY = posY + y;
            tsgl_pos getPosX = sprite->flixX ? (spriteMaxPointX - posX) : posX;
            tsgl_pos getPosY = sprite->flixY ? (spriteMaxPointY - posY) : posY;
            tsgl_rawcolor color = tsgl_framebuffer_rotationGet(sprite->sprite, sprite->rotation,
                sprite->resizeWidth == 0 ? getPosX : tsgl_math_imap(getPosX, 0, spriteMaxPointX, 0, spriteRealMaxPointX),
                sprite->resizeHeight == 0 ? getPosY : tsgl_math_imap(getPosY, 0, spriteMaxPointY, 0, spriteRealMaxPointY)
            );

            if (sprite->transparentColor.invalid || !tsgl_color_rawColorCompare(color, sprite->transparentColor, sprite->sprite->colorsize, sprite->sprite->floatColorsize)) {
                set(arg, setPosX, setPosY, color);
            }
        }
    }
}

void tsgl_gfx_push_wtrans(void* arg, TSGL_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    sprite->rotation = ((uint8_t)(-sprite->rotation)) % (uint8_t)4;

    if (sprite->sprite->hardwareRotate) {
        ESP_LOGE(TAG, "a sprite cannot have a hardware rotation");
        return;
    }

    tsgl_pos realSpriteWidth;
    tsgl_pos realSpriteHeight;
    switch (sprite->rotation) {
        case 1:
        case 3:
            realSpriteWidth = sprite->sprite->defaultHeight;
            realSpriteHeight = sprite->sprite->defaultWidth;
            break;

        default:
            realSpriteWidth = sprite->sprite->defaultWidth;
            realSpriteHeight = sprite->sprite->defaultHeight;
            break;
    }

    tsgl_pos spriteWidth = realSpriteWidth;
    if (sprite->resizeWidth != 0) spriteWidth = sprite->resizeWidth;
    tsgl_pos spriteHeight = realSpriteHeight;
    if (sprite->resizeHeight != 0) spriteHeight = sprite->resizeHeight;

    tsgl_pos startX = 0;
    tsgl_pos startY = 0;
    if (x < minX) startX = minX - x;
    if (y < minY) startY = minY - y;
    tsgl_pos maxSpriteWidth = maxX - x;
    tsgl_pos maxSpriteHeight = maxY - y;
    tsgl_pos spriteMaxPointX = spriteWidth - 1;
    tsgl_pos spriteMaxPointY = spriteHeight - 1;
    tsgl_pos spriteRealMaxPointX = realSpriteWidth - 1;
    tsgl_pos spriteRealMaxPointY = realSpriteHeight - 1;
    if (spriteWidth > maxSpriteWidth) spriteWidth = maxSpriteWidth;
    if (spriteHeight > maxSpriteHeight) spriteHeight = maxSpriteHeight;
    for (tsgl_pos posX = startX; posX < spriteWidth; posX++) {
        tsgl_pos setPosX = posX + x;
        for (tsgl_pos posY = startY; posY < spriteHeight; posY++) {
            tsgl_pos setPosY = posY + y;
            tsgl_pos getPosX = sprite->flixX ? (spriteMaxPointX - posX) : posX;
            tsgl_pos getPosY = sprite->flixY ? (spriteMaxPointY - posY) : posY;
            tsgl_rawcolor color = tsgl_framebuffer_rotationGet(sprite->sprite, sprite->rotation,
                sprite->resizeWidth == 0 ? getPosX : tsgl_math_imap(getPosX, 0, spriteMaxPointX, 0, spriteRealMaxPointX),
                sprite->resizeHeight == 0 ? getPosY : tsgl_math_imap(getPosY, 0, spriteMaxPointY, 0, spriteRealMaxPointY)
            );

            set(arg, setPosX, setPosY, color);
        }
    }
}

static size_t _len(const char* str) {
    size_t size = 0;
    while (str[size] != '\n' && str[size] != '\0') size++;
    return size;
}

static tsgl_pos _getY(const tsgl_print_settings* sets, tsgl_pos y, tsgl_pos iy, tsgl_pos scaleCharHeight, tsgl_pos maxScaleCharHeight) {
    switch (sets->locationMode) {
        case tsgl_print_start_bottom:
            iy = (maxScaleCharHeight - 1) - iy;
            break;

        case tsgl_print_start_top:
            break;
    }

    tsgl_pos riy = 0;
    switch (sets->locationMode) {
        case tsgl_print_start_bottom:
            riy = y - iy;
            break;

        case tsgl_print_start_top:
            riy = y + iy;
            break;
    }

    bool shiftIy =
        sets->localLocationMode == tsgl_print_localLocationMode_bottom ||
        (sets->localLocationMode == tsgl_print_localLocationMode_from_localtionMode && sets->locationMode == tsgl_print_start_bottom);

    if (shiftIy) {
        riy += maxScaleCharHeight - scaleCharHeight;
    } else if (sets->localLocationMode == tsgl_print_localLocationMode_center) {
        riy += (maxScaleCharHeight - scaleCharHeight) / 2;
    }

    return riy;
}

// ---------------- text rasterizer
// produces exactly the same pixels as the straightforward per-pixel implementation:
// every float/double expression that affects the picture is evaluated in the same form, it is just moved out of the hot loops

#define _TEXT_FONTCACHE_SIZE 128 // power of two. covers ASCII without collisions
#define _TEXT_CHUNK 32 // glyph columns processed per pass over the glyph rows
#define _TEXT_THRESHOLDS 16 // power of two
#define _TEXT_STROKE_MAX_THICKNESS 63 // the stroke grid keeps neighbor counts in 7 bits
#define _TEXT_STROKE_MAX_GRID 65536

typedef struct {
    const uint8_t* font;
    uint32_t glyph[_TEXT_FONTCACHE_SIZE]; // offset of the header of the first glyph with this code, 0 - unknown
    uint32_t missing[8]; // codes that are not in the font
} _text_fontcache;

typedef struct {
    const uint8_t* bits;
    uint16_t width;
    uint16_t height;
    uint16_t scaleWidth;
    uint16_t scaleHeight;
    uint16_t blockX;
    uint16_t blockY;
    uint32_t blockThreshold;
} _text_glyph;

typedef struct {
    _text_fontcache* cache;
    const tsgl_print_settings* sets;
    void* arg;
    TSGL_SET_REFERENCE(set);
    bool linearRows; // otherwise all rows of a glyph land on the same screen line
    int textMinX, textMaxX, textMinY, textMaxY; // glyph pixels are processed only inside this area (inclusive)
    int strokeMinX, strokeMaxX, strokeMinY, strokeMaxY; // stroke pixels are drawn only inside this area (inclusive)
    float contrast;
    int sourceBlock;
    tsgl_pos sourceX[_TEXT_CHUNK]; // glyph columns for the screen columns of sourceBlock
    uint16_t thresholdKey[_TEXT_THRESHOLDS];
    uint32_t thresholdValue[_TEXT_THRESHOLDS];
    uint8_t* grid;
    size_t gridSize;
} _text_raster;

static inline uint16_t _text_read16(const uint8_t* ptr, size_t index) {
    return (ptr[index] << 8) | ptr[index + 1];
}

static void _text_fontcacheInit(_text_fontcache* cache, const void* fontData) {
    memset(cache, 0, sizeof(_text_fontcache));
    cache->font = fontData;
}

// same result as tsgl_font_find (the bitmap of the first glyph with this code), but without double math and with caching
static size_t _text_find(_text_fontcache* cache, char chr) {
#if CHAR_MIN < 0
    if (chr < 0) return 0; // a negative char is never equal to a font byte
#endif
    const uint8_t* ptr = cache->font;
    uint8_t code = chr;

    uint32_t* slot = &cache->glyph[code & (_TEXT_FONTCACHE_SIZE - 1)];
    if (*slot != 0 && ptr[*slot] == code) return *slot + 5;
    if (cache->missing[code >> 5] & (1u << (code & 31))) return 0;

    size_t index = 1;
    while (ptr[index] != code) {
        if (ptr[index] == 0) {
            cache->missing[code >> 5] |= 1u << (code & 31);
            return 0;
        }
        // the search always starts from the beginning, so a free slot means this is the first glyph with this code
        uint32_t* other = &cache->glyph[ptr[index] & (_TEXT_FONTCACHE_SIZE - 1)];
        if (*other == 0) *other = index;
        index += ((((uint32_t)_text_read16(ptr, index + 1) * _text_read16(ptr, index + 3)) + 7) / 8) + 5;
    }
    *slot = index;
    return index + 5;
}

static uint16_t _text_width(_text_fontcache* cache, char chr) {
    size_t index = _text_find(cache, chr);
    if (index == 0) return 0;
    return _text_read16(cache->font, index - 4);
}

static uint16_t _text_height(_text_fontcache* cache, char chr) {
    size_t index = _text_find(cache, chr);
    if (index == 0) return 0;
    return _text_read16(cache->font, index - 2);
}

// (uint16_t)(value + 0.5) where the addition is done in double, but without software double math
static uint16_t _text_scaleSize(uint16_t size, float scale1, float scale2) {
    float value = (float)size * scale1 * scale2;
    if (value >= 0 && value < 65535.5f) {
        uint32_t integer = value;
        return integer + ((value - integer) >= 0.5f);
    }
    return value + 0.5;
}

// the smallest number of set source pixels in a block of allCount pixels at which the screen pixel belongs to the text
// (the original checked "(float)findedCount / (float)allCount > contrast" for every pixel)
static uint32_t _text_threshold(_text_raster* raster, uint16_t allCount) {
    uint32_t slot = allCount & (_TEXT_THRESHOLDS - 1);
    if (raster->thresholdKey[slot] == allCount) return raster->thresholdValue[slot];

    uint32_t low = 0;
    uint32_t high = (uint32_t)allCount + 1; // count / allCount > 1 >= contrast
    while (low < high) {
        uint32_t mid = (low + high) / 2;
        if ((float)mid / (float)allCount > raster->contrast) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    raster->thresholdKey[slot] = allCount;
    raster->thresholdValue[slot] = low;
    return low;
}

static bool _text_getGlyph(_text_raster* raster, char chr, _text_glyph* glyph) {
    size_t index = _text_find(raster->cache, chr);
    if (index == 0) return false;
    const uint8_t* fontData = raster->cache->font;
    glyph->bits = fontData + index;
    glyph->width = _text_read16(fontData, index - 4);
    glyph->height = _text_read16(fontData, index - 2);
    glyph->scaleWidth = _text_scaleSize(glyph->width, raster->sets->_scaleX, raster->sets->scaleX);
    glyph->scaleHeight = _text_scaleSize(glyph->height, raster->sets->_scaleY, raster->sets->scaleY);
    return true;
}

// called only for glyphs with visible pixels, so scaleWidth and scaleHeight are not zero
static void _text_glyphBlock(_text_raster* raster, _text_glyph* glyph) {
    glyph->blockX = glyph->width / glyph->scaleWidth;
    if (glyph->blockX < 1) glyph->blockX = 1;
    glyph->blockY = glyph->height / glyph->scaleHeight;
    if (glyph->blockY < 1) glyph->blockY = 1;
    glyph->blockThreshold = _text_threshold(raster, glyph->blockX * glyph->blockY);
}

// glyph columns for the screen columns [block * _TEXT_CHUNK, (block + 1) * _TEXT_CHUNK). they do not depend on the glyph
static const tsgl_pos* _text_sourceColumns(_text_raster* raster, int block) {
    if (raster->sourceBlock != block) {
        for (int i = 0; i < _TEXT_CHUNK; i++) {
            tsgl_pos ix = (block * _TEXT_CHUNK) + i;
            raster->sourceX[i] = ((float)ix) / raster->sets->_scaleX / raster->sets->scaleX;
        }
        raster->sourceBlock = block;
    }
    return raster->sourceX;
}

static inline uint32_t _text_popcount(uint32_t value) {
    value = value - ((value >> 1) & 0x55555555);
    value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
    value = (value + (value >> 4)) & 0x0F0F0F0F;
    return (value * 0x01010101) >> 24;
}

// number of set bits in [start, start + count) of a glyph bitmap. only the bytes holding these bits are read
static inline uint32_t _text_countBits(const uint8_t* bits, uint32_t start, uint32_t count) {
    const uint8_t* ptr = bits + (start >> 3);
    uint32_t shift = start & 7;
    uint32_t result = 0;
    while (count > 0) {
        uint32_t part = count > 24 ? 24 : count;
        uint32_t end = shift + part;
        uint32_t word = ptr[0];
        if (end > 8) word |= (uint32_t)ptr[1] << 8;
        if (end > 16) word |= (uint32_t)ptr[2] << 16;
        if (end > 24) word |= (uint32_t)ptr[3] << 24;
        result += _text_popcount((word >> shift) & ((1u << part) - 1));
        ptr += end >> 3;
        shift = end & 7;
        count -= part;
    }
    return result;
}

// cover[i] = 1 if the screen pixel of glyph row iy with glyph column sourceX[i] belongs to the text
static void _text_coverRow(_text_raster* raster, const _text_glyph* glyph, const tsgl_pos* sourceX, int count, tsgl_pos iy, uint8_t* cover) {
    tsgl_pos sourceY = ((float)iy) / raster->sets->_scaleY / raster->sets->scaleY;
    int restY = glyph->height - sourceY;
    uint32_t blockY = restY <= 0 ? 0 : (restY < glyph->blockY ? restY : glyph->blockY);
    if (blockY == 0) { // 0 / 0 in the original
        memset(cover, 0, count);
        return;
    }

    const uint8_t* bits = glyph->bits;
    uint32_t width = glyph->width;
    uint32_t rowStart = sourceY * width;
    if (glyph->blockX == 1 && blockY == 1) { // every screen pixel samples exactly one glyph pixel
        uint8_t bitSet = glyph->blockThreshold <= 1;
        uint8_t bitClear = glyph->blockThreshold == 0;
        for (int i = 0; i < count; i++) {
            uint32_t sx = sourceX[i];
            if (sx >= width) { // 0 / 0 in the original
                cover[i] = 0;
                continue;
            }
            uint32_t index = rowStart + sx;
            cover[i] = ((bits[index >> 3] >> (index & 7)) & 1) ? bitSet : bitClear;
        }
        return;
    }

    for (int i = 0; i < count; i++) {
        int restX = glyph->width - sourceX[i];
        uint32_t blockX = restX <= 0 ? 0 : (restX < glyph->blockX ? restX : glyph->blockX);
        if (blockX == 0) {
            cover[i] = 0;
            continue;
        }

        uint32_t start = rowStart + sourceX[i];
        uint32_t findedCount;
        if (blockX == 1 && blockY == 1) {
            findedCount = (bits[start >> 3] >> (start & 7)) & 1;
        } else {
            findedCount = 0;
            for (uint32_t ly = 0; ly < blockY; ly++) {
                findedCount += _text_countBits(bits, start, blockX);
                start += width;
            }
        }

        uint32_t threshold;
        if (blockX == glyph->blockX && blockY == glyph->blockY) {
            threshold = glyph->blockThreshold;
        } else {
            threshold = _text_threshold(raster, blockX * blockY);
        }
        cover[i] = (uint16_t)findedCount >= threshold;
    }
}

// pyFirst - screen y of the glyph row rowFirst
static void _text_drawGlyph(_text_raster* raster, const _text_glyph* glyph, tsgl_pos base, tsgl_pos pyFirst, int colFirst, int colEnd, int rowFirst, int rowEnd) {
    const tsgl_print_settings* sets = raster->sets;
    bool drawFg = !sets->fg.invalid;
    bool drawBg = !sets->bg.invalid;
    uint8_t cover[_TEXT_CHUNK];

    for (int first = colFirst; first < colEnd;) {
        int block = first / _TEXT_CHUNK;
        int count = TSGL_MATH_MIN(colEnd, (block + 1) * _TEXT_CHUNK) - first;
        const tsgl_pos* sourceX = _text_sourceColumns(raster, block) + (first - (block * _TEXT_CHUNK));
        for (int iy = rowFirst; iy < rowEnd; iy++) {
            tsgl_pos py = raster->linearRows ? pyFirst + (iy - rowFirst) : pyFirst;
            _text_coverRow(raster, glyph, sourceX, count, iy, cover);
            for (int i = 0; i < count; i++) {
                tsgl_pos px = base + first + i;
                if (cover[i]) {
                    if (drawFg) raster->set(raster->arg, px, py, sets->fg);
                } else if (drawBg) {
                    raster->set(raster->arg, px, py, sets->bg);
                }
            }
        }
        first += count;
    }
}

static void _text_strokeSet(_text_raster* raster, tsgl_pos px, tsgl_pos py) {
    if (px < raster->strokeMinX || px > raster->strokeMaxX || py < raster->strokeMinY || py > raster->strokeMaxY) return;
    raster->set(raster->arg, px, py, raster->sets->stroke);
}

static void _text_strokeGlyph(_text_raster* raster, const _text_glyph* glyph, tsgl_pos base, tsgl_pos pyFirst, int colFirst, int colEnd, int rowFirst, int rowEnd) {
    const tsgl_print_settings* sets = raster->sets;
    int thickness = sets->stroke_thickness;
    int width = colEnd - colFirst;
    int height = rowEnd - rowFirst;
    int gridWidth = width + (thickness * 2);
    int gridHeight = height + (thickness * 2);
    bool useGrid = raster->grid != NULL && (raster->linearRows || height == 1) && ((size_t)gridWidth * gridHeight) <= raster->gridSize;
    uint8_t* grid = raster->grid;
    uint8_t cover[_TEXT_CHUNK];

    if (useGrid) memset(grid, 0, (size_t)gridWidth * gridHeight);
    for (int first = colFirst; first < colEnd;) {
        int block = first / _TEXT_CHUNK;
        int count = TSGL_MATH_MIN(colEnd, (block + 1) * _TEXT_CHUNK) - first;
        const tsgl_pos* sourceX = _text_sourceColumns(raster, block) + (first - (block * _TEXT_CHUNK));
        for (int iy = rowFirst; iy < rowEnd; iy++) {
            _text_coverRow(raster, glyph, sourceX, count, iy, cover);
            if (useGrid) {
                uint8_t* cell = grid + ((iy - rowFirst + thickness) * gridWidth) + (first - colFirst + thickness);
                for (int i = 0; i < count; i++) cell[i] = cover[i] << 7;
            } else {
                tsgl_pos py = raster->linearRows ? pyFirst + (iy - rowFirst) : pyFirst;
                for (int i = 0; i < count; i++) {
                    if (!cover[i]) continue;
                    tsgl_pos px = base + first + i;
                    for (int ox = -thickness; ox <= thickness; ox++) {
                        for (int oy = -thickness; oy <= thickness; oy++) {
                            if (ox != 0 || oy != 0) _text_strokeSet(raster, px + ox, py + oy);
                        }
                    }
                }
            }
        }
        first += count;
    }
    if (!useGrid) return;

    // the stroke used to be drawn around every text pixel separately (the pixel itself excluded).
    // here every stroke pixel is drawn once: when there is another text pixel within the stroke radius.
    // pixels that the glyph itself paints over later are skipped
    int window = (thickness * 2) + 1;
    for (int gy = thickness; gy < thickness + height; gy++) { // low 7 bits: text pixels in the same row within the radius
        uint8_t* row = grid + (gy * gridWidth);
        int count = 0;
        for (int gx = 0; gx < gridWidth + thickness; gx++) {
            if (gx < gridWidth) count += row[gx] >> 7;
            if (gx >= window) count -= row[gx - window] >> 7;
            if (gx >= thickness) row[gx - thickness] |= count;
        }
    }

    bool fgOverlap = !sets->fg.invalid;
    bool bgOverlap = !sets->bg.invalid;
    int originX = (base + colFirst) - thickness;
    int originY = pyFirst - thickness;
    for (int gx = 0; gx < gridWidth; gx++) {
        tsgl_pos px = originX + gx;
        if (px < raster->strokeMinX || px > raster->strokeMaxX) continue;
        bool insideX = gx >= thickness && gx < thickness + width;
        int count = 0;
        for (int gy = 0; gy < gridHeight + thickness; gy++) {
            if (gy < gridHeight) count += grid[(gy * gridWidth) + gx] & 0x7F;
            if (gy >= window) count -= grid[((gy - window) * gridWidth) + gx] & 0x7F;
            int cy = gy - thickness;
            if (cy < 0) continue;

            int self = grid[(cy * gridWidth) + gx] >> 7;
            if (count <= self) continue;
            if (insideX && cy >= thickness && cy < thickness + height && (self ? fgOverlap : bgOverlap)) continue;
            tsgl_pos py = originY + cy;
            if (py < raster->strokeMinY || py > raster->strokeMaxY) continue;
            raster->set(raster->arg, px, py, sets->stroke);
        }
    }
}

static void _text_line(_text_raster* raster, bool drawStroke, const char* text, size_t strsize, uint16_t maxScaleCharHeight, tsgl_pos spacing, tsgl_pos spaceSize, tsgl_pos x, tsgl_pos y, tsgl_print_textArea* textArea) {
    const tsgl_print_settings* sets = raster->sets;
    bool draw = raster->set != NULL && (!sets->fg.invalid || !sets->bg.invalid);

    tsgl_pos offset = 0;
    for (size_t i = 0; i < strsize; i++) {
        char chr = text[i];
        if (chr != ' ') {
            _text_glyph glyph;
            if (_text_getGlyph(raster, chr, &glyph)) {
                tsgl_pos base = x + offset;
                int colFirst = TSGL_MATH_MAX(0, raster->textMinX - base);
                int colEnd = TSGL_MATH_MIN((int)glyph.scaleWidth, raster->textMaxX - base + 1);

                tsgl_pos py = _getY(sets, y, 0, glyph.scaleHeight, maxScaleCharHeight);
                int rowFirst = 0;
                int rowEnd = 0;
                if (raster->linearRows) {
                    rowFirst = TSGL_MATH_MAX(0, raster->textMinY - py);
                    rowEnd = TSGL_MATH_MIN((int)glyph.scaleHeight, raster->textMaxY - py + 1);
                    py += rowFirst;
                } else if (py >= raster->textMinY && py <= raster->textMaxY) {
                    rowEnd = glyph.scaleHeight;
                }

                if (rowFirst < rowEnd) {
                    if (drawStroke) {
                        if (colFirst < colEnd) {
                            _text_glyphBlock(raster, &glyph);
                            _text_strokeGlyph(raster, &glyph, base, py, colFirst, colEnd, rowFirst, rowEnd);
                        }
                    } else {
                        tsgl_pos pyLast = raster->linearRows ? py + ((rowEnd - rowFirst) - 1) : py;
                        if (py < textArea->top) textArea->top = py;
                        if (pyLast > textArea->bottom) textArea->bottom = pyLast;
                        if (colFirst < colEnd) {
                            tsgl_pos right = base + (colEnd - 1);
                            if (right > textArea->right) textArea->right = right;
                            if (draw) {
                                _text_glyphBlock(raster, &glyph);
                                _text_drawGlyph(raster, &glyph, base, py, colFirst, colEnd, rowFirst, rowEnd);
                            }
                        }
                    }
                }
                offset += glyph.scaleWidth + spacing;
            }
        } else {
            if (!drawStroke) {
                tsgl_pos staceEndPos = x + spaceSize + offset;
                if (staceEndPos > textArea->right) textArea->right = staceEndPos;
            }
            offset += spaceSize + spacing;
        }
    }
}

static tsgl_print_textArea _text(_text_fontcache* cache, void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY);

// same as tsgl_font_getTextArea, but reuses the glyph cache
static tsgl_print_textArea _text_area(_text_fontcache* cache, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return _text(cache, NULL, NULL, NULL, x, y, sets, text, TSGL_POS_MIN, TSGL_POS_MIN, TSGL_POS_MAX, TSGL_POS_MAX);
}

static tsgl_print_textArea _text(_text_fontcache* cache, void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    if (cache->font != sets.font) _text_fontcacheInit(cache, sets.font);

    size_t realsize = strlen(text);
    tsgl_print_textArea textArea = {
        .strlen = realsize
    };

    if (sets._scaleX == 0) sets._scaleX = 1;
    if (sets._scaleY == 0) sets._scaleY = 1;
    if (sets.scaleX == 0) sets.scaleX = 1;
    if (sets.scaleY == 0) sets.scaleY = 1;

    tsgl_pos standartWidth = _text_width(cache, 'A');
    if (sets.targetWidth != 0) {
        sets._scaleX = ((float)sets.targetWidth) / ((float)standartWidth);
    }
    standartWidth = (((float)standartWidth) * sets._scaleX) + 0.5;

    tsgl_pos standartHeight = _text_height(cache, 'A');
    if (sets.targetHeight == 0) {
        sets.targetHeight = sets.targetWidth;
    }
    if (sets.targetHeight != 0) {
        sets._scaleY = ((float)sets.targetHeight) / ((float)standartHeight);
    }
    standartHeight = (((float)standartHeight) * sets._scaleY) + 0.5;

    tsgl_pos spacing = sets.spacing > 0 ? sets.spacing : (standartWidth / 4);
    if (spacing <= 0) spacing = 1;

    if (!sets.fill.invalid && fill != NULL) {
        tsgl_print_textArea area = _text_area(cache, x, y, sets, text);

        tsgl_pos left   = TSGL_MATH_MAX(area.left, minX);
        tsgl_pos top    = TSGL_MATH_MAX(area.top, minY);
        tsgl_pos right  = TSGL_MATH_MIN(area.left + area.width - 1, maxX - 1);
        tsgl_pos bottom = TSGL_MATH_MIN(area.top + area.height - 1, maxY - 1);

        if (left <= right && top <= bottom) {
            fill(arg, left, top, right - left + 1, bottom - top + 1, sets.fill);
        }
    }

    if (sets.multiline) {
        tsgl_pos oldX = x;
        tsgl_pos oldY = y;

        if (sets.globalCentering || sets.globalAlignmentX != tsgl_print_alignment_left || sets.globalAlignmentY != tsgl_print_alignment_left) {
            tsgl_print_settings lSets;
            memcpy(&lSets, &sets, sizeof(tsgl_print_settings));
            lSets.globalCentering = false;
            lSets.globalAlignmentX = tsgl_print_alignment_left;
            lSets.globalAlignmentY = tsgl_print_alignment_left;
            lSets.alignment = tsgl_print_alignment_left;
            lSets._minWidth = oldX;
            lSets._maxWidth = (oldX + sets.width) - 1;
            lSets._clamp = true;
            lSets.stroke_thickness = 0;
            lSets.stroke = TSGL_INVALID_RAWCOLOR;

            switch (sets.locationMode) {
                case tsgl_print_start_bottom:
                    lSets._minHeight = (y - sets.height) + 1;
                    lSets._maxHeight = y;
                    break;

                case tsgl_print_start_top:
                    lSets._minHeight = y;
                    lSets._maxHeight = (y + sets.height) - 1;
                    break;
            }

            tsgl_print_textArea textArea = _text_area(cache, x, y, lSets, text);
            if (sets.width > 0) {
                if (sets.globalCentering || sets.globalAlignmentX == tsgl_print_alignment_center) {
                    x += (sets.width / 2) - (textArea.width / 2);
                } else if (sets.globalAlignmentX == tsgl_print_alignment_right) {
                    x += sets.width - textArea.width;
                }
            }

            if (sets.height > 0) {
                if (sets.globalCentering || sets.globalAlignmentY == tsgl_print_alignment_center) {
                    y += (sets.height / 2) - (textArea.height / 2);
                } else if (sets.globalAlignmentY == tsgl_print_alignment_right) {
                    y += sets.height - textArea.height;
                }
            }
        }

        tsgl_print_settings newSets = {
            .font = sets.font,
            .fill = TSGL_INVALID_RAWCOLOR,
            .bg = sets.bg,
            .fg = sets.fg,
            ._minWidth = oldX,
            ._maxWidth = (oldX + sets.width) - 1,
            ._clamp = true,
            .scaleX = sets.scaleX,
            .scaleY = sets.scaleY,
            ._scaleX = sets._scaleX,
            ._scaleY = sets._scaleY,
            .spacing = sets.spacing,
            .spaceSize = sets.spaceSize,
            .locationMode = sets.locationMode,
            .localLocationMode = sets.localLocationMode,
            .stroke = sets.stroke,
            .stroke_thickness = sets.stroke_thickness,
            .stroke_no_clamp = sets.stroke_no_clamp
        };

        switch (sets.locationMode) {
            case tsgl_print_start_bottom:
                newSets._minHeight = (oldY - sets.height) + 1;
                newSets._maxHeight = oldY;
                break;

            case tsgl_print_start_top:
                newSets._minHeight = oldY;
                newSets._maxHeight = (oldY + sets.height) - 1;
                break;
        }

        tsgl_print_settings newSetsCheck;
        memcpy(&newSetsCheck, &newSets, sizeof(tsgl_print_settings));
        newSetsCheck.stroke = TSGL_INVALID_RAWCOLOR;
        newSetsCheck.stroke_thickness = 0;

        textArea.top = TSGL_POS_MAX;
        textArea.bottom = TSGL_POS_MIN;
        textArea.left = TSGL_POS_MAX;
        textArea.right = TSGL_POS_MIN;

        tsgl_pos high_size = 0;
        if (sets.alignment != tsgl_print_alignment_left) {
            for (size_t i = 0; i < realsize;) {
                tsgl_print_textArea lTextArea = _text_area(cache, x, y, newSetsCheck, text + i);
                if (lTextArea.width > high_size) high_size = lTextArea.width;
                i += lTextArea.strlen + 1;
                if (*((const char*)(text + i)) == '\0') break;
            }
        }

        tsgl_pos currentY = y;
        for (size_t i = 0; i < realsize;) {
            tsgl_pos offsetX = 0;

            if (sets.alignment != tsgl_print_alignment_left) {
                tsgl_print_textArea llTextArea = _text_area(cache, x, y, newSetsCheck, text + i);
                if (sets.alignment == tsgl_print_alignment_center) offsetX += (high_size / 2) - (llTextArea.width / 2);
                else if (sets.alignment == tsgl_print_alignment_right) offsetX += high_size - llTextArea.width;
            }

            tsgl_print_textArea lTextArea = _text(cache, arg, set, fill, x + offsetX, currentY, newSets, text + i, minX, minY, maxX, maxY);
            if (lTextArea.top < textArea.top) textArea.top = lTextArea.top;
            if (lTextArea.bottom > textArea.bottom) textArea.bottom = lTextArea.bottom;
            if (lTextArea.left < textArea.left) textArea.left = lTextArea.left;
            if (lTextArea.right > textArea.right) textArea.right = lTextArea.right;
            i += lTextArea.strlen + 1;
            if (*((const char*)(text + i)) == '\0') break;
            switch (sets.locationMode) {
                case tsgl_print_start_bottom:
                    currentY -= lTextArea.height + spacing;
                    break;

                case tsgl_print_start_top:
                    currentY += lTextArea.height + spacing;
                    break;
            }
        }
        textArea.width = (textArea.right - textArea.left) + 1;
        textArea.height = (textArea.bottom - textArea.top) + 1;
        return textArea;
    }

    textArea.left = x;
    textArea.right = x;
    switch (sets.locationMode) {
        case tsgl_print_start_bottom:
            textArea.top = y;
            textArea.bottom = y;
            break;
        case tsgl_print_start_top:
            textArea.top = y;
            textArea.bottom = y;
            break;
    }
    size_t strsize = _len(text);
    textArea.strlen = strsize;

    _text_raster raster = {
        .cache = cache,
        .sets = &sets,
        .arg = arg,
        .set = set,
        .linearRows = sets.locationMode == tsgl_print_start_bottom || sets.locationMode == tsgl_print_start_top,
        .textMinX = minX,
        .textMaxX = maxX - 1,
        .textMinY = minY,
        .textMaxY = maxY - 1,
        .sourceBlock = -1
    };
    if (sets._clamp) {
        raster.textMinX = TSGL_MATH_MAX(raster.textMinX, sets._minWidth);
        raster.textMaxX = TSGL_MATH_MIN(raster.textMaxX, sets._maxWidth);
        raster.textMinY = TSGL_MATH_MAX(raster.textMinY, sets._minHeight);
        raster.textMaxY = TSGL_MATH_MIN(raster.textMaxY, sets._maxHeight);
    }
    raster.strokeMinX = minX;
    raster.strokeMaxX = maxX - 1;
    raster.strokeMinY = minY;
    raster.strokeMaxY = maxY - 1;
    if (sets._clamp && !sets.stroke_no_clamp) {
        raster.strokeMinX = TSGL_MATH_MAX(raster.strokeMinX, sets._minWidth);
        raster.strokeMaxX = TSGL_MATH_MIN(raster.strokeMaxX, sets._maxWidth);
        raster.strokeMinY = TSGL_MATH_MAX(raster.strokeMinY, sets._minHeight);
        raster.strokeMaxY = TSGL_MATH_MIN(raster.strokeMaxY, sets._maxHeight);
    }
    float contrast = sets.contrast > 0 ? sets.contrast : DEFAULT_CONTRAST;
    raster.contrast = 1 - contrast;
    for (size_t i = 0; i < _TEXT_THRESHOLDS; i++) {
        raster.thresholdKey[i] = 0;
        raster.thresholdValue[i] = 1; // 0 / 0 is not more than the contrast, count / 0 is
    }

    uint16_t maxScaleCharWidth = 0;
    uint16_t maxScaleCharHeight = 0;
    for (size_t i = 0; i < strsize; i++) {
        _text_glyph glyph;
        if (text[i] != ' ' && _text_getGlyph(&raster, text[i], &glyph)) {
            if (glyph.scaleWidth > maxScaleCharWidth) maxScaleCharWidth = glyph.scaleWidth;
            if (glyph.scaleHeight > maxScaleCharHeight) maxScaleCharHeight = glyph.scaleHeight;
        }
    }

    tsgl_pos spaceSize;
    if (sets.spaceSize == 0) {
        spaceSize = standartWidth * 0.7;
    } else {
        spaceSize = sets.spaceSize;
    }

    // the stroke pass changes only pixels, so it is not needed when only the text area is calculated
    if (set != NULL && !sets.stroke.invalid && sets.stroke_thickness > 0) {
        if (sets.stroke_thickness <= _TEXT_STROKE_MAX_THICKNESS) {
            size_t gridSize = (size_t)(maxScaleCharWidth + (sets.stroke_thickness * 2)) * (maxScaleCharHeight + (sets.stroke_thickness * 2));
            if (gridSize <= _TEXT_STROKE_MAX_GRID) {
                raster.grid = malloc(gridSize);
                if (raster.grid != NULL) raster.gridSize = gridSize;
            }
        }
        _text_line(&raster, true, text, strsize, maxScaleCharHeight, spacing, spaceSize, x, y, &textArea);
        free(raster.grid);
        raster.grid = NULL;
    }
    _text_line(&raster, false, text, strsize, maxScaleCharHeight, spacing, spaceSize, x, y, &textArea);

    textArea.width = (textArea.right - textArea.left) + 1;
    textArea.height = (textArea.bottom - textArea.top) + 1;
    return textArea;
}

tsgl_print_textArea tsgl_gfx_text(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    _text_fontcache cache;
    _text_fontcacheInit(&cache, sets.font);
    return _text(&cache, arg, set, fill, x, y, sets, text, minX, minY, maxX, maxY);
}

/*
tsgl_print_textArea TSGL_FAST_FUNC tsgl_gfx_text(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    return (tsgl_print_textArea) {};
}
*/

tsgl_sprite* tsgl_gfx_renderTextToSprite(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_print_settings sets, const char* text, tsgl_colormode colormode, int64_t caps, tsgl_rawcolor transparentColor, tsgl_rawcolor clearcolor) {
    tsgl_sprite* sprite = calloc(1, sizeof(tsgl_sprite));
    tsgl_framebuffer* sprite_fb = malloc(sizeof(tsgl_framebuffer));
    sprite->sprite = sprite_fb;
    sprite->transparentColor = transparentColor;

    if (tsgl_framebuffer_init(sprite_fb, colormode, width, height, caps) != ESP_OK) {
        free(sprite);
        free(sprite_fb);
        return NULL;
    }

    if (!clearcolor.invalid) tsgl_framebuffer_clear(sprite_fb, clearcolor);
    tsgl_gfx_text(sprite_fb, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fillWithoutCheck, x, y, sets, text, sprite_fb->viewport_minX, sprite_fb->viewport_minY, sprite_fb->viewport_maxX, sprite_fb->viewport_maxY);

    return sprite;
}
