# Arty

Arty is an educational rendering framework. It contains all the basic infrastructure to render from simple to complex scenes, and will be used throughout the _Realistic Image Synthesis_ course.

## Disclaimers

Please do not distribute or share the source code of this program. If you want to use a version control software, please do so privately. Sharing source code publicly will be considered cheating.

The test scenes are provided with authorization from their respective authors. If you want to redistribute the scenes, make sure to follow their licenses, if any.

## Building

The dependencies are:

- [CMake](https://cmake.org/download/)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

> On Linux, all of these should be available from your package manager. The versions of CMake, SDL2 and libpng that come from your package manager can be installed, but please **do not** use the version of yaml-cpp that comes from your package manager. Instead, use the provided link to download or clone the yaml-cpp repository and build it from source.

> On Windows, we recommend using [mingw-w64](https://mingw-w64.org/doku.php/download), but MSVC is supported (with OpenMP disabled, since OpenMP 3.0 is not supported by MSVC). For VS2015 and VS2017 users, we provide setup scripts: Run them after installing CMake.

Start by downloading the prerequisites and building yaml-cpp from source (build instructions for yaml-cpp are on the GitHub page). Once this is done, create a `build` directory in the directory where Arty was downloaded/cloned. Open a command line prompt in the `build` directory, and type:

    cmake-gui ..

This should open the GUI for CMake. Choose which version of Visual Studio you have installed or "MinGW Makefiles" as a generator and press the "configure" button. Fill in the paths for SDL2/libpng/yaml-cpp include directories and libraries.

Finally, press the "generate" button, which will create, depending on your choice, a Makefile for mingw-w64/gcc or an MSVC solution.

When using gcc, the build command is simply:

    make

With mingw-w64, it is:

    mingw32-make

With MSVC, the build command is available from the "Build" menu, once the solution has been loaded.

## Running

Arty comes with a set of test scenes. They are present in the `test` directory. Each scene is described in a YAML file (`.yml` extension). To run Arty, use the following command:

    arty <path-to-scene>.yml

To get the full list of options, type:

    arty -h

By default, Arty runs the debug renderer (eye-light shading only). The camera position can be controlled using the keyboard arrows. The keypad `+`/`-` keys control the translation speed. The camera direction is controlled with the mouse. To enable camera control, keep the left mouse button pressed while moving the cursor. The `R` key cycles through the different rendering algorithms. The implementation of these algorithms is missing and will be your task.

## Scene format

The scene files are described in [YAML](http://yaml.org/). Here is an example of scene:

```yaml
---
# List of OBJ files
meshes: ["model.obj"]
# Camera definition
camera: !perspective_camera {
    eye: [-0.45,  1.5, -1.0],      # Position of the camera
    center: [-0.45, -10.0, -100],  # Point to look at
    up: [0, 1, 0],                 # Up vector
    fov: 60.0                      # Field of view, in degrees
}
# List of lights (optional, emissive materials in the OBJ file will be converted to area lights)
lights: [
    !point_light {
        position: [0, 10, 0],
        color: [100, 100, 0]
    },
    !triangle_light {
        v0: [0, 1, 2],              # First vertex
        v1: [0, 1, 3],              # Second vertex
        v2: [1, 1, 2],              # Third vertex
        color: [100, 0, 100]
    }
]
```

## Conventions

The conventions in Arty are as follows:

- Functions to sample a direction should return **normalized** vectors
- Rays generated from the camera should have a **normalized** direction
- Materials and lights expect a **normalized** direction for sampling and evaluation
- _No redundant normalization should be done otherwise_

By enforcing those conventions, we ensure that the code is correct _and_ fast. In debug mode, you can check whether this convention is actually followed using the `assert_normalized` macro in `common.h`.

## Documentation

The source code is thoroughly documented, and [Doxygen](http://www.stack.nl/~dimitri/doxygen/) can generate structured HTML pages from all the inline comments in the source files.
