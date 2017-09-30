#ifndef RENDER_H
#define RENDER_H

class Camera;
struct Scene;
struct Image;

/// Renders a debug image to test rendering functionality and performance.
void render_debug(const Scene& scene, Image& img, int iter);
/// Renders an image using Path Tracing.
void render_pt(const Scene& scene, Image& img, int iter);
/// Renders an image using Bidirectional Path Tracing.
void render_bpt(const Scene& scene, Image& img, int iter);
/// Renders an image using Progressive Photon Mapping.
void render_ppm(const Scene& scene, Image& img, int iter);

#endif // RENDER_H
