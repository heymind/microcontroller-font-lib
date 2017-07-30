
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "FF_common.h"
#include "FF_decoder.h"
#define _META_OFFSET (sizeof(FFHeader) + sizeof(FFMetaHeader))

static uint32_t fileLength(FILE *fp)
{
#ifdef FF_FILE_LENGTH
    return FF_FILE_LENGTH(fp);
#else
    fseek(fp, 0, SEEK_END);
    return ftell(fp);
#endif
}

static bool hasGlyph(const struct FFFont *font, uint16_t code)
{
    FFCombinedFontMember *member = (FFCombinedFontMember *)font->data;
    return member->start <= code && code < member->end;
}

static FF_ERROR glyphInfo(const struct FFFont *font, uint16_t code, uint8_t *top, uint8_t *left, uint8_t *width, uint8_t *advance)
{
    FFCombinedFontMember *member = (FFCombinedFontMember *)font->data;
    assert(member->start <= code && code < member->end);

    uint32_t offset = member->offset + _META_OFFSET;
    offset += (code - member->start) * sizeof(FFMeta);

    FFMeta meta;
    FILE *fp = member->fp;
    fseek(fp, offset, SEEK_SET);
    fread(&meta, sizeof(FFMeta), 1, fp);
    *top = meta.top;
    *width = meta.width;
    *advance = meta.advance;
    *left = meta.left;
}

static uint16_t drawGlyph(const struct FFFont *font, uint16_t code, int16_t xOffset, int16_t yOffset, FFDrawCallback cb, void *data)
{
    FFCombinedFontMember *member = (FFCombinedFontMember *)font->data;
    assert(member->start <= code && code < member->end);

    uint32_t offset = member->offset + _META_OFFSET;
    offset += (code - member->start) * sizeof(FFMeta);

    FFMeta meta;
    FILE *fp = member->fp;
    fseek(fp, offset, SEEK_SET);
    fread(&meta, sizeof(FFMeta), 1, fp);

    offset = meta.offset;
    uint32_t length;
    if (code + 1 < member->end)
    {
        FFMeta meta2;
        fread(&meta2, sizeof(FFMeta), 1, fp);
        length = meta2.offset - meta.offset;
    }
    else
    {
        length = member->length - meta.offset;
    }
    offset += member->offset;
    fseek(fp, offset, SEEK_SET);
    uint8_t *buffer = malloc(length);
    fread(buffer, sizeof(uint8_t), length, fp);

    xOffset -= meta.left;
    yOffset -= meta.top;

    bool use1Bit = FFHeaderIs1Bit(&member->header);

    uint16_t x, y;
    if (use1Bit)
    {
        for (int i = 0; i < length; i++)
        {
            for (int n = 0; n < 8; n++)
            {
                if ((buffer[i] >> n) & 1)
                {
                    uint16_t t = i * 8 + n;
                    x = t % meta.width;
                    y = t / meta.width;
                    cb(x + xOffset, y + yOffset, 255, data);
                }
            }
        }
    }
    else
    {
        uint8_t t1, t2;
        for (int i = 0; i < length; i++)
        {
            // printf(/"@%d,%d@",buffer[i],i);
            if ((t1 = buffer[i] & 0b1111))
            {
                x = (i * 2) % meta.width;
                y = (i * 2) / meta.width;
                cb(x + xOffset, y + yOffset, t1 * 16, data);
            }
            if ((t2 = buffer[i] >> 4))
            {
                x = (i * 2 + 1) % meta.width;
                y = (i * 2 + 1) / meta.width;
                cb(x + xOffset, y + yOffset, t2 * 16, data);
            }
        }
    }
    free(buffer);
    return meta.advance;
}

FFCombinedFontHandle *openCombinedFont(char *filePath)
{
    FILE *fp = fopen(filePath, "rb");
    // printf("\n\n%d\n\n",fp);
    CHK_OK(fp, "Load combinedfont file error.");
    uint8_t n;
    fread(&n, sizeof(uint8_t), 1, fp); //how many fonts

    assert(n > 0);

    FFCombinedFontMember *members = calloc(n, sizeof(FFCombinedFontMember));
    FFFont *fonts = calloc(n, sizeof(FFFont));

    FFCombinedFontHandle *handle = calloc(1, sizeof(FFCombinedFontHandle));
    handle->count = n;
    handle->fonts = fonts;
    handle->members = members;

    for (int i = 0; i < n; i++)
    {
        CHK_OK_(fread(fonts[i].name, sizeof(char), 5, fp));
        CHK_OK_(fread(&(members[i].offset), sizeof(uint32_t), 1, fp));
        if (i > 0)
            members[i - 1].length = members[i].offset - members[i - 1].offset;
    }
    members[n - 1].length = fileLength(fp) - members[n - 1].offset;

    FFMetaHeader meta;
    for (int i = 0; i < n; i++)
    {
        fseek(fp, members[i].offset, SEEK_SET);
        fread(&(members[i].header), sizeof(FFHeader), 1, fp);
        fread(&(meta), sizeof(FFMetaHeader), 1, fp);
        members[i].start = meta.start;
        members[i].end = meta.end;
        members[i].fp = fp;

        fonts[i].hasGlyph = &hasGlyph;
        fonts[i].glyphInfo = &glyphInfo;
        fonts[i].drawGlyph = &drawGlyph;
        fonts[i].data = members + i;
    }
    return handle;
}
void closeCombinedFont(FFCombinedFontHandle *handle)
{
    fclose(handle->fp);
    free(handle->members);
    free(handle->fonts);
    free(handle);
}
FF_ERROR drawText(int fontCount, FFFont fonts[], uint16_t  *text, uint16_t pointX, uint16_t pointY, uint16_t width,uint16_t lineHeight ,FFDrawCallback cb, void *data)
{
    uint16_t x = 0;
    uint16_t y = 0;
    while(*text != '\0'){
        for(int i = 0 ; i<fontCount;i++)
        {
           if(fonts[i].hasGlyph(&fonts[i],*text))
           {
               printf("%d\n",*text);
               x += fonts[i].drawGlyph(&fonts[i],*text,pointX + x , pointY + y,cb,data);
               break;
           }
        }
        if(x>width){
            x = 0;
            y+=lineHeight;
        }
        text++;
    }
}


