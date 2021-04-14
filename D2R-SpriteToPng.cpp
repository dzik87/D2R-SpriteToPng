// D2R Sprite to PNG exporter by dzik

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <png.h>

#pragma warning(disable:4996)

// Packing just in case compiler decides to align it differently
#pragma pack(push)
#pragma pack(1)
struct D2R_SpA1 {
    unsigned char   szHeader[4];                        // 0x00 - Always SpA1
    uint16_t        nVersion;                           // 0x04 - Version? 31 or 61, maybe more
    uint16_t        nFrameWidth;                        // 0x06 - Real frame width
    uint32_t        nWidth;                             // 0x08 - Overall width
    uint32_t        nHeight;                            // 0x0C - Overall height
    uint32_t        nUnk1;                              // 0x10 - 0
    uint32_t        nFrames;                            // 0x14 - Number of frames
    uint32_t        nUnk2;                              // 0x18 - 0
    uint32_t        nUnk3;                              // 0x1C - 0
    uint32_t        nStreamSize;                        // 0x20 - Pixel stream size
    uint32_t        nUnk4;                              // 0x24 - 4
    uint32_t        aPixelStream[1];                    // 0x28 - Pixel stream RGBA
    png_bytep       operator [](unsigned int n) const;  // helper method for convinient pixel export
};
#pragma pack(pop)

png_bytep D2R_SpA1::operator [](unsigned int n) const
{
    return (png_bytep)((char*)this+0x28+n*4);
}

static char* ReadAllBytes(const char* filename)
{
    char* buffer;
    long size;
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer = new char[size];
    file.read(buffer, size);
    file.close();
    return buffer;
}

png_byte** row_pointers = NULL;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " filename.sprite" << std::endl;
        return 0;
    }

    D2R_SpA1* sprite = (D2R_SpA1*)ReadAllBytes(argv[1]);

    std::cout << "Fileversion: " << sprite->nVersion << std::endl;

    // As for now it works only with version 31
    if (sprite->nVersion != 31) {
        std::cout << "Unsuported sprite version" << std::endl;
        return 0;
    }

    // Check if pixel stream size has enough data
    if (sprite->nWidth * sprite->nHeight * 4 > sprite->nStreamSize) {
        std::cout << "Pixel stream size mismatch." << std::endl;
        return 0;
    }

    int y;
    int width = sprite->nWidth, height = sprite->nHeight;

    char newName[256] = {};
    sprintf(newName, "%s.png", argv[1]);

    FILE* fp = fopen(newName, "wb");
    if (!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    row_pointers = (png_byte**)png_malloc(png, height * sizeof(png_byte*));
    for (int y = 0; y < height; y++) {
        png_byte* row = (png_byte*)png_malloc(png, sizeof(uint8_t) * width * 4);
        row_pointers[y] = row;
        for (int x = 0; x < width; x++) {
            png_bytep pixel = (*sprite)[y * width + x];
            *row++ = pixel[0];
            *row++ = pixel[1];
            *row++ = pixel[2];
            *row++ = pixel[3];
        }
    }

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    fclose(fp);

    png_destroy_write_struct(&png, &info);

    delete[] sprite;

    return 0;

}
