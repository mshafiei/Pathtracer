#include <iostream>
#include <chrono>
#include <climits>
#include <fstream>
#include <sstream>

#include <SDL2/SDL.h>

#include "common.h"
#include "scene.h"
#include "options.h"
#include "render.h"
#include "cameras.h"
#include "debug.h"

#ifndef NDEBUG
static bool debug = false;
#endif

typedef std::function<void (const Scene&, Image&, int)> RenderFunction;

static const char* render_fn_names[] = { "DEBUG", "PT", "PPM" };
static RenderFunction render_fns[] = { render_debug, render_pt, render_ppm };

static constexpr int num_render_fns = sizeof(render_fns) / sizeof(render_fns[0]);

bool handle_events(SDL_Window* window, Scene& scene, int& render_fn, int& accum) {
    SDL_Event event;

    static bool arrows[4], speed[2];
    const float rspeed = 0.005f;
    static float tspeed = 0.1f;

    static bool camera_on = false;
    static bool select_on = false;

    while (SDL_PollEvent(&event)) {
        bool key_down = event.type == SDL_KEYDOWN;
        switch (event.type) {
            case SDL_QUIT: return true;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    camera_on = true;
                }
#ifndef NDEBUG
                if (!camera_on && event.button.button == SDL_BUTTON_RIGHT) {
                    select_on = true;
                    debug_xmin = event.button.x;
                    debug_xmax = INT_MIN;
                    debug_ymin = event.button.y;
                    debug_ymax = INT_MIN;
                }
#endif
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    camera_on = false;
                }
#ifndef NDEBUG
                if (event.button.button == SDL_BUTTON_RIGHT) select_on = false;
#endif
                break;
            case SDL_MOUSEMOTION:
                {
                    if (camera_on) {
                        scene.camera->mouse_motion(event.motion.xrel * rspeed, event.motion.yrel * rspeed);
                        accum = 0;
                    }
#ifndef NDEBUG
                    if (select_on) {
                        debug_xmax = std::max(debug_xmax, event.motion.x);
                        debug_ymax = std::max(debug_ymax, event.motion.y);
                    }
#endif
                }
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
#ifndef NDEBUG
                    case SDLK_d:        debug = key_down; break;
#endif
                    case SDLK_UP:       arrows[0] = key_down; break;
                    case SDLK_DOWN:     arrows[1] = key_down; break;
                    case SDLK_LEFT:     arrows[2] = key_down; break;
                    case SDLK_RIGHT:    arrows[3] = key_down; break;
                    case SDLK_KP_PLUS:  speed[0] = key_down; break;
                    case SDLK_KP_MINUS: speed[1] = key_down; break;
                    case SDLK_r:
                        if (key_down) {
                            std::ostringstream title;
                            render_fn = (render_fn + 1) % num_render_fns;
                            title << "arty (" << render_fn_names[render_fn] << ")";
                            SDL_SetWindowTitle(window, title.str().c_str());
                            accum = 0;
                        }
                        break;
                    case SDLK_ESCAPE:
                        return true;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (arrows[0]) { scene.camera->keyboard_motion(0, 0,  tspeed); accum = 0; }
    if (arrows[1]) { scene.camera->keyboard_motion(0, 0, -tspeed); accum = 0; }
    if (arrows[2]) { scene.camera->keyboard_motion(-tspeed, 0, 0); accum = 0; }
    if (arrows[3]) { scene.camera->keyboard_motion( tspeed, 0, 0); accum = 0; }
    if (speed[0]) tspeed *= 1.1f;
    if (speed[1]) tspeed *= 0.9f;

    return false;
}

int main(int argc, char** argv) {
    ArgParser parser(argc, argv);

    bool help;
    int width, height;
    std::string output_image;
    double max_time;
    int max_samples;
    int render_fn;

    parser.add_option("help",      "h",    "Prints this message",               help,   false);
    parser.add_option("width",     "sx",   "Sets the window width, in pixels",  width,  1080, "px");
    parser.add_option("height",    "sy",   "Sets the window height, in pixels", height, 720, "px");

    parser.add_option("output",    "o",    "Sets the output file name", output_image, std::string("render.png"), "file.png");

    parser.add_option("samples",   "s",    "Sets the desired number of samples", max_samples, 0);
    parser.add_option("time",      "t",    "Sets the desired render time in seconds", max_time, 0.0);

    parser.add_option("algo",      "a",    "Sets the algorithm used for rendering: debug vis. (0), PT (1), BPT (2), PPM (3)", render_fn, 0);

    parser.parse();
    if (help) {
        parser.usage();
        return 0;
    }

    auto args = parser.arguments();
    if (!args.size()) {
        error("No configuration file specified. Exiting.");
        return 1;
    } else if (args.size() > 1) {
        warn("Too many configuration files specified, all but the first will be ignored.");
    }

    Scene scene;
    scene.width = width;
    scene.height = height;
    if (!load_scene(args[0], scene))
        return 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        error("Cannot initialize SDL.");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("arty", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    SDL_Surface* screen = SDL_GetWindowSurface(window);
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    Image img(width, height);
    img.clear();

#ifndef NDEBUG
    debug_xmin = INT_MAX;
    debug_xmax = INT_MIN;
    debug_ymin = INT_MAX;
    debug_ymax = INT_MIN;
#endif

    bool done = false;
    int frames = 0, accum = 0;
    uint64_t frame_time = 0;
    double total_time = 0;
    int total_frames = 0;

    while (!done) {
        using namespace std::chrono;

#ifndef NDEBUG
        if (debug || (debug_xmin >= debug_xmax && debug_ymin >= debug_ymax)) {
#endif
            if (accum++ == 0) {
                total_time = 0;
                total_frames = 0;
                img.clear();
            }

            auto start_render = high_resolution_clock::now();
            render_fns[render_fn](scene, img, accum);
            auto end_render = high_resolution_clock::now();
            auto render_time = duration_cast<milliseconds>(end_render - start_render).count();
            frame_time += render_time;
            total_time += render_time * 0.001;
            frames++;
            total_frames++;

#ifndef NDEBUG
            if (debug) info("Debug information dumped.");
            debug = false;
        }
#endif

        if (frames > 20 || (frames > 0 && frame_time > 5000)) {
            info("Average frame time: ", frame_time / frames, " ms.");
            frames = 0;
            frame_time = 0;
        }

        if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
        #pragma omp parallel for
        for (int y = 0; y < img.height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)screen->pixels + screen->pitch * y);
            for (int x = 0; x < img.width; x++) {
                auto pix = gamma(img(x, y) / accum);
                const uint8_t r = clamp(pix.x, 0.0f, 1.0f) * 255.0f;
                const uint8_t g = clamp(pix.y, 0.0f, 1.0f) * 255.0f;
                const uint8_t b = clamp(pix.z, 0.0f, 1.0f) * 255.0f;
                const uint8_t a = clamp(pix.w, 0.0f, 1.0f) * 255.0f;
                row[x] = ((r << screen->format->Rshift) & screen->format->Rmask) |
                         ((g << screen->format->Gshift) & screen->format->Gmask) |
                         ((b << screen->format->Bshift) & screen->format->Bmask) |
                         ((a << screen->format->Ashift) & screen->format->Amask);
            }
        }

#ifndef NDEBUG
        if (debug_xmin < debug_xmax && debug_ymin < debug_ymax) {
            for (int y = std::max(0, debug_ymin), h = std::min(img.height, debug_ymax); y < h; y++) {
                uint32_t* row = (uint32_t*)((uint8_t*)screen->pixels + screen->pitch * y);
                for (int x = std::max(0, debug_xmin), w = std::min(img.width, debug_xmax); x < w; x++) {
                const uint8_t r = row[x] & screen->format->Rmask;
                const uint8_t g = row[x] & screen->format->Gmask;
                const uint8_t b = row[x] & screen->format->Bmask;
                const uint8_t a = row[x] & screen->format->Amask;
                row[x] = (((r + 64) << screen->format->Rshift) & screen->format->Rmask) |
                         (((g + 64) << screen->format->Gshift) & screen->format->Gmask) |
                         (((b + 64) << screen->format->Bshift) & screen->format->Bmask) |
                         (((a) << screen->format->Ashift) & screen->format->Amask);
                }
            }
        }
#endif
        if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

        SDL_UpdateWindowSurface(window);
        done = handle_events(window, scene, render_fn, accum);
        done |= max_samples != 0 && total_frames >= max_samples;
        done |= max_time != 0.0  && total_time   >= max_time;
    }

    for (auto& pix : img.pixels)
        pix = clamp(gamma(pix / accum), rgba(0), rgba(1));

    if (!save_png(img, output_image)) {
        error("Failed to save image to '", output_image, "'.");
        return 1;
    }
    info("Image saved to '", output_image, "' (", accum, " samples, ", total_time, " s).");

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
