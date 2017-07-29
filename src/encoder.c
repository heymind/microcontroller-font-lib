#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "FF_common.h"

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library;
FT_Face face;

// function from https://gist.github.com/ironpinguin/7627623
unsigned char *unpack_mono_bitmap(FT_Bitmap bitmap)
{
    unsigned char *result;
    int y, x, byte_index, num_bits_done, rowstart, bits, bit_index;
    unsigned char byte_value;

    result = malloc(bitmap.rows * bitmap.width);

    for (y = 0; y < bitmap.rows; y++)
    {
        for (byte_index = 0; byte_index < bitmap.pitch; byte_index++)
        {

            byte_value = bitmap.buffer[y * bitmap.pitch + byte_index];

            num_bits_done = byte_index * 8;

            rowstart = y * bitmap.width + byte_index * 8;

            bits = 8;
            if ((bitmap.width - num_bits_done) < 8)
            {
                bits = bitmap.width - num_bits_done;
            }

            for (bit_index = 0; bit_index < bits; bit_index++)
            {
                int bit;
                bit = byte_value & (1 << (7 - bit_index));

                result[rowstart + bit_index] = bit;
            }
        }
    }

    return result;

}
// Chinese glyph is from 0x4e00 to 0x9fbb
// https://zh.wikipedia.org/wiki/Unicode
int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        printf("encoder ttfFile outputFile size start end faceNumber(use 0)\n");
        exit(0);
    }

    char *inputFilePath = *++argv,
         *outputFilePath = *++argv;

    uint16_t size = atoi(*++argv),
             start = atoi(*++argv),
             end = atoi(*++argv),
             faceNum = atoi(*++argv);

    printf("%d",faceNum);
    CHK_ERR(FT_Init_FreeType(&library), "init error.");
    CHK_ERR(FT_New_Face(library,
                        inputFilePath,
                        faceNum,
                        &face),
            "face load error.");

    bool use1Bit = false;

    assert(size > 7);
    printf("num_fixed_sizes(%d):", face->num_fixed_sizes);
    for (int i = 0; i < (face->num_fixed_sizes); i++)
    {
        int height = face->available_sizes[i].height;
        printf("%d|", height);
        if (size == height)
            use1Bit = true;
    }
    printf("\n");

    if (use1Bit)
        printf("The 1-bit bitmap font is available for the requested size,generate 1-bit output... \n");

    CHK_ERR(FT_Set_Pixel_Sizes(face, 0, size), "set pixel size fault.");

    assert(start > 0);
    assert(end > start);

    uint32_t FFHeaderOffset = 0;
    uint32_t FFMetaHeaderOffset = FFHeaderOffset + sizeof(FFHeader);
    uint32_t FFMetaOffset = FFMetaHeaderOffset + sizeof(FFMetaHeader);
    uint32_t FFDataOffset = FFMetaOffset + (end - start) * sizeof(FFMeta);

    //open output file

    FILE *fp;
    fp = fopen(outputFilePath, "wb");
    CHK_OK(fp, "Fail to open output file...");
    //write MetaHeader

    CHK_ERR_(fseek(fp, FFMetaHeaderOffset, SEEK_SET));

    FFMetaHeader metaHeader = {
        .start = start,
        .end = end};

    //-----------------
    assert(fwrite(&metaHeader, sizeof(FFMetaHeader), 1, fp) == 1);

    for (uint16_t index = start; index < end; index++)
    {
        // FFMeta meta;
        // unsigned char* cache;
        int glyphIndex = FT_Get_Char_Index(face, index);

        CHK_OK(glyphIndex, "char not found.");
        CHK_ERR_(FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT));
        CHK_ERR_(FT_Render_Glyph(face->glyph,
                                 use1Bit ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL));

        FT_Bitmap bitmap = face->glyph->bitmap;
        if( face->glyph->bitmap_left < 0)
        {
            int d = 0 - face->glyph->bitmap_left;
            face->glyph->advance.x += d*64;
            face->glyph->bitmap_left += d;
        } 
        FFMeta meta = {
            .left = face->glyph->bitmap_left,
            .top = face->glyph->bitmap_top,
            .advance = face->glyph->advance.x / 64,
            .width = bitmap.width,
            .offset = FFDataOffset};

        uint8_t *cache, *data;
        uint32_t length;
        if (use1Bit)
        {
            cache = unpack_mono_bitmap(bitmap);
            length = ceil((float)(bitmap.width * bitmap.rows) / 8);
            data = calloc(length, sizeof(uint8_t));

            for (uint32_t i = 0; i < bitmap.width * bitmap.rows; i++)
            {
                if (cache[i])
                {
                    uint32_t t1 = i / 8;
                    uint32_t t2 = i % 8;
                    data[t1] |= 1 << t2;
                }
            }
        }
        else
        {
            cache = bitmap.buffer;
            length = ceil((float)(bitmap.width * bitmap.rows) / 2);
            data = calloc(length, sizeof(uint8_t));
            for (uint32_t i = 0; i < bitmap.width * bitmap.rows; i++)
            {
                uint16_t grey;
                if (grey = cache[i] / 16)
                {
                    printf("(%d,%d)-%d|",i % bitmap.width,i / bitmap.width,grey );
                    uint32_t t1 = i / 2;
                    uint32_t t2 = (i % 2) * 4;
                    data[t1] |= grey << t2;
                }
            }
        }
        printf("\n");
        for(int i=0;i<length;i++){
            printf("%02X ",data[i]);
        }
        printf("\n");
        //write them to file
        printf("writing char %04X,width = %d,point = (%d,%d),advance = %d ,length = %d,@%d \n",
            index,
            meta.width,
            meta.left,
            meta.top,
            meta.advance,
            length,
            FFDataOffset
            );
        
        CHK_ERR_(fseek(fp, FFMetaOffset, SEEK_SET));
        assert(fwrite(&meta, sizeof(FFMeta), 1, fp) == 1);
        FFMetaOffset += sizeof(FFMeta);

        CHK_ERR_(fseek(fp, FFDataOffset, SEEK_SET));
        assert(fwrite(data, sizeof(uint8_t), length, fp) == length);
        FFDataOffset += length;

        if (use1Bit)
            free(cache);
    }

    printf("Total glyph count:%d , metadata size: %d B , data size: %d KB \n",
           end - start,
           (FFMetaOffset - FFMetaHeaderOffset) ,
           (FFDataOffset - FFMetaOffset)/1024
           );

    printf("writing file header...");
    FFHeader header = {
        .version = 1,
        .flags = use1Bit
        };

    CHK_ERR_(fseek(fp, 0, SEEK_SET));
    assert(fwrite(&header, sizeof(FFHeader), 1, fp) == 1);
    // FFMetaOffset += sizeof(FFMeta);

    fclose(fp);
    return 0; //no error
}
