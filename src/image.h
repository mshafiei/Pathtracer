#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>

#include "color.h"

struct Image {
    Image() {}
    Image(int w, int h)
        : pixels(w * h), width(w), height(h)
    {}

    const rgba& operator () (int x, int y) const { return pixels[y * width + x]; }
    rgba& operator () (int x, int y) { return pixels[y * width + x]; }

    const rgba* row(int y) const { return &pixels[y * width]; }
    rgba* row(int y) { return &pixels[y * width]; }

    void resize(int w, int h) {
        width = w;
        height = h;
        pixels.resize(w * h);
    }

    void clear() {
        std::fill(pixels.begin(), pixels.end(), rgba(0.0f, 0.0f, 0.0f, 1.0f));
    }

    std::vector<rgba> pixels;
    int width, height;
};

/// Loads an image from a PNG file.
bool load_png(const std::string& png_file, Image& image);
/// Stores an image as a PNG file.
bool save_png(const Image& image, const std::string& png_file);

/// Loads an image from a TGA file.
bool load_tga(const std::string& tga_file, Image& image);

#endif // IMAGE_H
