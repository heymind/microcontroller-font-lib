#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "FF_common.h"

typedef int8_t (*FFDrawCallback)(uint16_t x,uint16_t y,uint8_t gery,void* data);

struct FFFont {
    char name[5];
    void* data;
    bool (*hasGlyph)(const struct FFFont *font,uint16_t code);
    FF_ERROR (*glyphInfo)(const struct FFFont *font,uint16_t code,uint8_t* top,uint8_t* left,uint8_t* width,uint8_t* advance); 
    uint16_t (*drawGlyph)(const struct FFFont *font,uint16_t code,int16_t xOffset,int16_t yOffset,FFDrawCallback,void*);
};
typedef struct FFFont FFFont;

typedef struct {
    uint16_t start;
    uint16_t end;
    uint32_t offset;
    uint32_t length;
    FFHeader header;
    FILE *fp;
} FFCombinedFontMember;

typedef struct {
    FILE *fp;
    uint8_t count;
    FFCombinedFontMember* members;
    FFFont* fonts;

} FFCombinedFontHandle;

FFCombinedFontHandle* openCombinedFont(char *filePath);
void closeCombinedFont(FFCombinedFontHandle *handle);
FF_ERROR drawText(int fontCount,FFFont fonts[],uint16_t* text,uint16_t pointX,uint16_t pointY,uint16_t width,uint16_t lineHeight,FFDrawCallback cb,void* data);


