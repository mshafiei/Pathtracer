#include "../scene.h"
#include "../color.h"
#include "../samplers.h"
#include "../cameras.h"
#include "../hash.h"
#include "../debug.h"

void render_debug(const Scene& scene, Image& img, int iter) {
    auto kx = 2.0f / (img.width - 1);
    auto ky = 2.0f / (img.height - 1);

    #pragma omp parallel for
    for (int y = 0; y < img.height; y++) {
        UniformSampler sampler(sampler_seed(y, iter));

        for (int x = 0; x < img.width; x++) {
            auto ray = scene.camera->gen_ray(
                (x + sampler()) * kx - 1.0f,
                1.0f - (y + sampler()) * ky);
            Hit hit = scene.intersect(ray);

            rgba color(0.0f);
            if (hit.tri >= 0) {
                auto n0 = scene.normals[scene.indices[hit.tri * 4 + 0]];
                auto n1 = scene.normals[scene.indices[hit.tri * 4 + 1]];
                auto n2 = scene.normals[scene.indices[hit.tri * 4 + 2]];
                auto n = normalize(lerp(n0, n1, n2, hit.u, hit.v));
                auto k = fabsf(dot(n, ray.dir));
                color = rgba(k, k, k, 1.0f);
            }

            img(x, y) += color;
        }
    }
}
