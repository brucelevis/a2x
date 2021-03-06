/*
    Copyright 2010, 2016-2018 Alex Margarit <alex@alxm.org>
    This file is part of a2x, a C video game framework.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "a2x_pack_png.v.h"

#include <zlib.h>
#include <png.h>

#include "a2x_pack_file.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_out.v.h"

typedef struct {
    const uint8_t* data;
    size_t offset;
} AByteStream;

static void readFunction(png_structp Png, png_bytep Data, png_size_t Length)
{
    AByteStream* stream = png_get_io_ptr(Png);

    memcpy(Data, stream->data + stream->offset, Length);
    stream->offset += Length;
}

static void pngToPixels(png_structp Png, png_infop Info, APixel** Pixels, int* Width, int* Height)
{
    png_uint_32 w = png_get_image_width(Png, Info);
    png_uint_32 h = png_get_image_height(Png, Info);
    unsigned numChannels = png_get_channels(Png, Info);

    APixel* pixels = a_mem_malloc(w * h * sizeof(APixel));
    png_bytepp rows = png_get_rows(Png, Info);

    *Width = (int)w;
    *Height = (int)h;
    *Pixels = pixels;

    for(png_uint_32 y = h; y--; rows++) {
        for(png_uint_32 x = w, chOffset = 0; x--; chOffset += numChannels) {
            *pixels++ = a_pixel_fromRgb(rows[0][chOffset + 0],
                                        rows[0][chOffset + 1],
                                        rows[0][chOffset + 2]);
        }
    }
}

void a_png_readFile(const char* Path, APixel** Pixels, int* Width, int* Height)
{
    png_structp png = NULL;
    png_infop info = NULL;

    AFile* f = a_file_new(Path, A_FILE_READ | A_FILE_BINARY);

    if(f == NULL) {
        goto cleanUp;
    }

    if(a_path_test(a_file_pathGet(f), A_PATH_EMBEDDED)) {
        a_png_readMemory(a_file__dataGet(f)->buffer, Pixels, Width, Height);
        goto cleanUp;
    }

    #define PNG_SIG 8
    png_byte sig[PNG_SIG];

    a_file_read(f, sig, PNG_SIG);

    if(png_sig_cmp(sig, 0, PNG_SIG) != 0) {
        a_out__error("a_png_readFile(%s): Not a PNG", Path);
        goto cleanUp;
    }

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(png == NULL) {
        goto cleanUp;
    }

    info = png_create_info_struct(png);

    if(info == NULL) {
        goto cleanUp;
    }

    if(setjmp(png_jmpbuf(png))) {
        goto cleanUp;
    }

    png_init_io(png, a_file_handleGet(f));
    png_set_sig_bytes(png, PNG_SIG);
    png_read_png(png, info, PNG_TRANSFORM_EXPAND, NULL);

    int type = png_get_color_type(png, info);

    if(type != PNG_COLOR_TYPE_RGB && type != PNG_COLOR_TYPE_RGBA) {
        a_out__error("a_png_readFile(%s): Not an RGB or RGBA PNG", Path);
        goto cleanUp;
    }

    pngToPixels(png, info, Pixels, Width, Height);

cleanUp:
    if(png) {
        png_destroy_read_struct(&png, info ? &info : NULL, NULL);
    }

    a_file_free(f);
}

void a_png_readMemory(const uint8_t* Data, APixel** Pixels, int* Width, int* Height)
{
    AByteStream* stream = a_mem_malloc(sizeof(AByteStream));

    stream->data = Data;
    stream->offset = 0;

    png_structp png = NULL;
    png_infop info = NULL;

    #define PNG_SIG 8
    png_byte sig[PNG_SIG];

    memcpy(sig, Data, PNG_SIG);

    if(png_sig_cmp(sig, 0, PNG_SIG) != 0) {
        a_out__error("a_png_readMemory: Data not a PNG");
        goto cleanUp;
    }

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(png == NULL) {
        goto cleanUp;
    }

    info = png_create_info_struct(png);

    if(info == NULL) {
        goto cleanUp;
    }

    if(setjmp(png_jmpbuf(png))) {
        goto cleanUp;
    }

    png_set_read_fn(png, stream, readFunction);
    png_read_png(png, info, PNG_TRANSFORM_EXPAND, NULL);

    const int type = png_get_color_type(png, info);

    if(type != PNG_COLOR_TYPE_RGB && type != PNG_COLOR_TYPE_RGBA) {
        a_out__error("a_png_readMemory: Data not an RGB or RGBA PNG");
        goto cleanUp;
    }

    pngToPixels(png, info, Pixels, Width, Height);

cleanUp:
    if(png) {
        png_destroy_read_struct(&png, info ? &info : NULL, NULL);
    }

    free(stream);
}

void a_png_write(const char* Path, const APixel* Data, int Width, int Height, char* Title, char* Description)
{
    AFile* f = a_file_new(Path, A_FILE_WRITE | A_FILE_BINARY);

    png_structp png = NULL;
    png_infop info = NULL;
    volatile png_bytepp rows = NULL;
    volatile png_bytep rowsData = NULL;
    png_text text[2];
    volatile int numText = 0;

    if(f == NULL) {
        goto cleanUp;
    }

    #define COLOR_CHANNELS 3
    rows = a_mem_malloc((unsigned)Height * sizeof(png_bytep));
    rowsData = a_mem_malloc((unsigned)Height * (unsigned)Width * COLOR_CHANNELS
                                * sizeof(png_byte));

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(png == NULL) {
        goto cleanUp;
    }

    info = png_create_info_struct(png);

    if(info == NULL) {
        goto cleanUp;
    }

    if(setjmp(png_jmpbuf(png))) {
        goto cleanUp;
    }

    png_set_compression_level(png, Z_BEST_COMPRESSION);

    png_set_IHDR(png,
                 info,
                 (unsigned)Width,
                 (unsigned)Height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    for(int i = 0; i < Height; i++) {
        rows[i] = rowsData + i * Width * COLOR_CHANNELS;

        for(int j = 0; j < Width; j++) {
            int r, g, b;
            a_pixel_toRgb(*(Data + i * Width + j), &r, &g, &b);

            rows[i][j * COLOR_CHANNELS + 0] = (png_byte)r;
            rows[i][j * COLOR_CHANNELS + 1] = (png_byte)g;
            rows[i][j * COLOR_CHANNELS + 2] = (png_byte)b;
        }
    }

    if(Title) {
        text[numText].compression = PNG_TEXT_COMPRESSION_NONE;
        text[numText].key = "Title";
        text[numText].text = Title;
        numText++;
    }

    if(Description) {
        text[numText].compression = PNG_TEXT_COMPRESSION_NONE;
        text[numText].key = "Description";
        text[numText].text = Description;
        numText++;
    }

    png_init_io(png, a_file_handleGet(f));
    png_set_rows(png, info, rows);
    png_set_text(png, info, text, numText);
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
    png_write_end(png, NULL);

cleanUp:
    if(png) {
        png_destroy_write_struct(&png, info ? &info : NULL);
    }

    free(rows);
    free(rowsData);

    a_file_free(f);
}
