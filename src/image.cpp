#include <fstream>
#include <cassert>
#include <cstring>
#include <png.h>

#include "image.h"

static void read_from_stream(png_structp png_ptr, png_bytep data, png_size_t length) {
    png_voidp a = png_get_io_ptr(png_ptr);
    ((std::istream*)a)->read((char*)data, length);
}

static void write_to_stream(png_structp png_ptr, png_bytep data, png_size_t length) {
    png_voidp a = png_get_io_ptr(png_ptr);
    ((std::ostream*)a)->write((const char*)data, length);
}

static void flush_stream(png_structp) {
    // Nothing to do
}

bool load_png(const std::string& png_file, Image& image) {
    std::ifstream file(png_file, std::ifstream::binary);
    if (!file)
        return false;

    // Read signature
    char sig[8];
    file.read(sig, 8);
    if (!png_check_sig((unsigned char*)sig, 8))
        return false;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
        return false;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    png_set_sig_bytes(png_ptr, 8);
    png_set_read_fn(png_ptr, (png_voidp)&file, read_from_stream);
    png_read_info(png_ptr, info_ptr);

    int width  = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);
    png_uint_32 bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    // Expand paletted and grayscale images to RGB
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    } else if (color_type == PNG_COLOR_TYPE_GRAY ||
               color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }

    // Transform to 8 bit per channel
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    // Get alpha channel when there is one
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    // Otherwise add an opaque alpha channel
    else
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    image.resize(width, height);
    std::vector<png_byte> row_bytes(width * 4);
    for (int y = 0; y < height; y++) {
        png_read_row(png_ptr, row_bytes.data(), nullptr);
        rgba* img_row = image.row(y);
        for (int x = 0; x < width; x++) {
            img_row[x].x = row_bytes[x * 4 + 0] / 255.0f;
            img_row[x].y = row_bytes[x * 4 + 1] / 255.0f;
            img_row[x].z = row_bytes[x * 4 + 2] / 255.0f;
            img_row[x].w = row_bytes[x * 4 + 3] / 255.0f;
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return true;
}

bool save_png(const Image& image, const std::string& png_file) {
    std::ofstream file(png_file, std::ofstream::binary);
    if (!file)
        return false;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
        return false;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    std::vector<uint8_t> row(image.width * 4);
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    png_set_write_fn(png_ptr, &file, write_to_stream, flush_stream);

    png_set_IHDR(png_ptr, info_ptr, image.width, image.height,
                 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < image.height; y++) {
        const rgba* input = image.row(y);
        for (int x = 0; x < image.width; x++) {
            row[x * 4 + 0] = input[x].x * 255.0f;
            row[x * 4 + 1] = input[x].y * 255.0f;
            row[x * 4 + 2] = input[x].z * 255.0f;
            row[x * 4 + 3] = input[x].w * 255.0f;
        }
        png_write_row(png_ptr, row.data());
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
}

struct TgaHeader
{
    unsigned short width;
    unsigned short height;
    unsigned char bpp;
    unsigned char desc;
};

enum TgaType {
    TGA_NONE,
    TGA_RAW,
    TGA_COMP
};

inline TgaType check_signature(const char* sig) {
    const char raw_sig[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};
    const char comp_sig[12] = {0,0,10,0,0,0,0,0,0,0,0,0};

    if (!std::memcmp(sig, raw_sig, sizeof(char) * 12))
        return TGA_RAW;

    if (!std::memcmp(sig, comp_sig, sizeof(char) * 12))
        return TGA_COMP;

    return TGA_NONE;
}

inline void copy_pixels24(rgba* img, const unsigned char* pixels, int n) {
    for (int i = 0; i < n; i++) {
        img[i].z = pixels[i * 3 + 0] / 255.0f;
        img[i].y = pixels[i * 3 + 1] / 255.0f;
        img[i].x = pixels[i * 3 + 2] / 255.0f;
        img[i].w = 1.0f;
    }
}

inline void copy_pixels32(rgba* img, const unsigned char* pixels, int n) {
    for (int i = 0; i < n; i++) {
        img[i].z = pixels[i * 4 + 0] / 255.0f;
        img[i].y = pixels[i * 4 + 1] / 255.0f;
        img[i].x = pixels[i * 4 + 2] / 255.0f;
        img[i].w = pixels[i * 4 + 3] / 255.0f;
    }
}

static void load_raw_tga(const TgaHeader& tga, std::istream& stream, Image& image) {
    assert(tga.bpp == 24 || tga.bpp == 32);

    if (tga.bpp == 24) {
        std::vector<char> tga_row(3 * tga.width);
        for (int y = 0; y < tga.height; y++) {
            rgba* row = image.row(tga.height - y - 1);
            stream.read(tga_row.data(), tga_row.size());
            copy_pixels24(row, (unsigned char*)tga_row.data(), tga.width);
        }
    } else {
        std::vector<char> tga_row(4 * tga.width);
        for (int y = 0; y < tga.height; y++) {
            rgba* row = image.row(tga.height - y - 1);
            stream.read(tga_row.data(), tga_row.size());
            copy_pixels32(row, (unsigned char*)tga_row.data(), tga.width);
        }
    }
}

static void load_compressed_tga(const TgaHeader& tga, std::istream& stream, Image& image) {
    assert(tga.bpp == 24 || tga.bpp == 32);

    const int pix_count = tga.width * tga.height;
    int cur_pix = 0;
    while (cur_pix < pix_count) {
        unsigned char chunk;
        stream.read((char*)&chunk, 1);

        if (chunk < 128) {
            chunk++;

            char pixels[4 * 128];
            stream.read(pixels, chunk * (tga.bpp / 8));
            if (cur_pix + chunk > pix_count) chunk = pix_count - cur_pix;
            
            if (tga.bpp == 24) {
                copy_pixels24(image.pixels.data() + cur_pix, (unsigned char*)pixels, chunk);
            } else {
                copy_pixels32(image.pixels.data() + cur_pix, (unsigned char*)pixels, chunk);
            }

            cur_pix += chunk;
        } else {
            chunk -= 127;

            unsigned char tga_pix[4];
            tga_pix[3] = 255;
            stream.read((char*)tga_pix, (tga.bpp / 8));

            if (cur_pix + chunk > pix_count) chunk = pix_count - cur_pix;

            rgba* pix = image.pixels.data() + cur_pix;
            const rgba c(tga_pix[2] / 255.0f,
                         tga_pix[1] / 255.0f,
                         tga_pix[0] / 255.0f,
                         tga_pix[3] / 255.0f);
            for (int i = 0; i < chunk; i++)
                pix[i] = c;

            cur_pix += chunk;
        }
    }
}

bool load_tga(const std::string& tga_file, Image& image) {
    std::ifstream file(tga_file, std::ifstream::binary);
    if (!file)
        return false;

    // Read signature
    char sig[12];
    file.read(sig, 12);
    TgaType type = check_signature(sig);
    if (type == TGA_NONE)
        return false;

    TgaHeader header;
    file.read((char*)&header, sizeof(TgaHeader));
    if (!file) return false;

    if (header.width <= 0 || header.height <= 0 ||
        (header.bpp != 24 && header.bpp !=32)) {
        return false;
    }

    image.resize(header.width, header.height);

    if (type == TGA_RAW) {
        load_raw_tga(header, file, image);
    } else {
        load_compressed_tga(header, file, image);
    }

    return true;
}
