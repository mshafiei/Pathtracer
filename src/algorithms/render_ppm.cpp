#include "../scene.h"
#include "../color.h"
#include "../samplers.h"
#include "../cameras.h"
#include "../hash.h"
#include "../hash_grid.h"
#include "../debug.h"
#include "../intersect.h"
#define BASIC_PATH_TRACER 1
//#define NEXT_EVENT_ESTIMATOR 2
//#define MULTIPLE_IMPORTANCE_SAMPLING 3
#ifdef _OPENMP
#include <omp.h>
#define OMP_THREAD_NUM() omp_get_thread_num()
#else
#define OMP_THREAD_NUM() 0
#endif

struct Photon {
    rgb contrib;    ///< Path contribution
    float3 in_dir;  ///< Incoming direction
    float3 pos;     ///< Surface parameters at the vertex

    Photon() {}
    Photon(const rgb& c, const float3& i, const float3& p)
        : contrib(c), in_dir(i), pos(p)
    {}
};

struct PhotonMap {
    const std::vector<Photon>& photons;
    HashGrid grid;
    float radius;

    /// Builds a photon map on a set of photons with the speicified query radius
    PhotonMap(const std::vector<Photon>& photons, float radius)
        : photons(photons), radius(radius)
    {
        grid.build([&](int i){ return photons[i].pos; }, photons.size(), radius);
    }

    /// Queries the photon map and calls the given function for each photon found
    template <typename PhotonCallbackFn>
    void query(const float3& pos, PhotonCallbackFn callback) const {
        grid.query(pos,
                   [&] (int i) { return photons[i].pos; },
                   [&] (int id, float d) { callback(photons[id], d); });
    }
};

static void trace_photons(std::vector<Photon>& photons, const Scene& scene, Sampler& sampler) {
    static constexpr float offset = 1e-4f;

    // TODO: Choose a light to sample from (uniformly) and get an emission sample for it
    int lighti = std::floor(sampler() * (((float)scene.lights.size()) - 0.01));
    float pLight = 1.0f / scene.lights.size();
    auto lightSample = scene.lights[lighti].get()->sample_emission(sampler);
    auto energy = lightSample.intensity;
    //float d = length(lightSample.pos - surf.point);
    float pLightSample = pLight * lightSample.pdf_area * lightSample.pdf_dir;// *(d * d / lightSample.cos);
    //float pdf = pLightSample;//pdf of the energy throughput
    energy = energy / pLightSample * lightSample.cos;
    // TODO: Create the starting ray from the light sample
    // Hints:
    // _ Add an offset to avoid artifacts. The constructor for Ray is: Ray(origin, direction, offset)
    // _ Initialize the pdf of the path according to the emission sample
    // _ Initialize the contribution of the path according to the emission sample
    Ray ray(lightSample.pos, lightSample.dir,0.001);

    while (true) {
        Hit hit = scene.intersect(ray);
        if (hit.tri < 0) break;

        auto mat = scene.material(hit);
        auto surf = scene.surface_params(ray, hit);
        auto out = -ray.dir;
        if (!mat.bsdf || mat.emitter) break;

        // TODO: Implement photon shooting here
        if (mat.bsdf->type() != Bsdf::Type::Specular)
        {
            photons.emplace_back(energy, out, surf.point);
        }
        
        //Bounce (sample outgoing dir)
        auto sample = mat.bsdf->sample(sampler, surf, out, true);
        energy *= sample.color / sample.pdf;
        ray.org = surf.point;
        ray.dir = sample.in;

        //Terminate?
        float q = 1 - russian_roulette(sample.color / sample.pdf);
        if (sampler() < q)
            break;
        else
            energy *= 1 / (1 - q);
    }
}

void NextEventEstimator2(rgb& irradiance, float &pNE, const Material &mat, const float3 &out, const SurfaceParams &surf, const Scene& scene, Sampler& sampler, const rgb &throughput, float &pBRDF)
{
    int lighti = std::floor(sampler() * (((float)scene.lights.size()) - 0.01));
    float pLight = 1.0f / scene.lights.size();
    auto lightSample = scene.lights[lighti].get()->sample_direct(surf.point, sampler);
    float d = length(lightSample.pos - surf.point);
    auto sampledDir = (lightSample.pos - surf.point) / d;
    pNE = lightSample.pdf_area * (d * d / lightSample.cos) * pLight;
    auto occHit = scene.occluded(Ray(surf.point, sampledDir, 0.0001f, d - 0.0001f));
    if (!occHit)//not occluded
    {
        float cosTheta = std::max(dot(surf.coords.n, sampledDir), 0.0f);
        auto brdf = mat.bsdf->eval(sampledDir, surf, out);//
        irradiance = brdf * cosTheta * lightSample.intensity / pNE * throughput;
    }
    pBRDF = mat.bsdf->pdf(sampledDir, surf, out);
}

static rgb eye_trace(Ray ray, const Scene& scene, const PhotonMap& photon_map, Sampler& sampler, int light_path_count) {
    static constexpr float offset = 1e-4f;

    // TODO: Initialize path variables (see Path Tracing assignment)
    rgb color(0.0f), throughput(1.0f);
    float pBRDF = 1;
    ray.tmin = offset;
    auto lastMat = Bsdf::Type::Specular;
    while (true) {
        Hit hit = scene.intersect(ray);
        if (hit.tri < 0) break;

        auto surf = scene.surface_params(ray, hit);
        auto mat = scene.material(hit);
        auto out = -ray.dir;

        // TODO: Handle direct light hits (see Path Tracing assignment)
        if (auto light = mat.emitter) {
            // Direct hits on a light source
            if (surf.entering) {
                if(lastMat != Bsdf::Type::Glossy)
                    color += throughput * light->emission(out, surf.uv.x, surf.uv.y).intensity;
            }
            break;
        }
        if (!mat.bsdf) break;
        if (mat.bsdf->type() == Bsdf::Type::Specular)
        {
            // TODO: Do a photon query if the material is not specular, otherwise bounce (as in Path Tracing)
            ray.org = surf.point;
            ray.dir = mat.bsdf->sample(sampler, surf, out).in;//pdf is one
            lastMat = Bsdf::Type::Specular;
        }
        else if (mat.bsdf->type() == Bsdf::Type::Glossy)
        {
            rgb directIrradiance(0);
            float pNE = 0;
            NextEventEstimator2(directIrradiance, pNE, mat, out, surf, scene, sampler, throughput, pBRDF);
            color += directIrradiance;

            //Trace another path
            auto sample = mat.bsdf->sample(sampler, surf, out);
            throughput *= (sample.color / sample.pdf);
            ray.org = surf.point;
            ray.dir = sample.in;
            lastMat = Bsdf::Type::Glossy;

        }
        else
        {
            
        // Hints:
        // _ Photons can be queried from the "photon_map" object. Use the followin function call:
            //rgb density(0); 
            
            photon_map.query(surf.point, [&] (const Photon& p, float d2) {
            // p is the photon, d2 is the squared distance from the point to the photon
            // TODO: Do something
            float r2 = photon_map.radius * photon_map.radius;
            float pir2 = r2 * pi;
            rgb k;
            k = 2 / pir2 * (1 - (d2 / r2)) * p.contrib / photon_map.photons.size(); //Epanechnikov Filter
            
            //k = 1.0f / (pi * d2) * p.contrib;
            
            color += throughput * mat.bsdf->eval(p.in_dir, surf, out) * k;
        }); 
            lastMat = Bsdf::Type::Diffuse;
        break; 
        
        }
    }

    return color;
}

static float estimate_pixel_size(const Scene& scene, int w, int h) {
    float total_dist = 0.0f;
    int total_count = 0;

    auto kx = 2.0f / (w - 1);
    auto ky = 2.0f / (h - 1);

    // Compute distance between neighboring pixels in world space,
    // in order to get a good estimate for the initial photon size.
    #pragma omp parallel for
    for (int y = 0; y < h; y += 8) {
        float d = 0; int c = 0;
        for (int x = 0; x < w; x += 8) {
            Ray rays[4]; Hit hits[4];
            for (int i = 0; i < 4; i++) {
                rays[i] = scene.camera->gen_ray(
                    (x + (i % 2 ? 4 : 0)) * kx - 1.0f,
                    1.0f - (y + (i / 2 ? 4 : 0)) * ky);
                hits[i] = scene.intersect(rays[i]);
            }
            auto eval_distance = [&] (int i, int j) {
                if (hits[i].tri >= 0 && hits[i].tri == hits[j].tri) {
                    d += length((rays[i].org + hits[i].t * rays[i].dir) -
                                (rays[j].org + hits[j].t * rays[j].dir));
                    c++;
                }
            };
            eval_distance(0, 1);
            eval_distance(2, 3);
            eval_distance(0, 2);
            eval_distance(1, 3);
        }

        #pragma omp atomic
        total_dist += d;
        #pragma omp atomic
        total_count += c;
    }

    return total_count > 0 ? total_dist / (4 * total_count) : 1.0f;
}

void render_ppm(const Scene& scene, Image& img, int iter) {
    constexpr float alpha = 0.75f;

    static float base_radius = 1.0f;
    if (iter == 1)
        base_radius = 2.0f * estimate_pixel_size(scene, img.width, img.height);

    auto kx = 2.0f / (img.width - 1);
    auto ky = 2.0f / (img.height - 1);

    std::vector<Photon> photons;

    // Trace a light path for every pixel
    #pragma omp parallel
    {
        std::vector<Photon> photon_buffer;
        UniformSampler sampler(sampler_seed(OMP_THREAD_NUM(), iter));

        #pragma omp for schedule(dynamic) nowait
        for (int i = 0; i < img.height * img.width; i++)
            trace_photons(photon_buffer, scene, sampler);

        #pragma omp critical
        { photons.insert(photons.end(), photon_buffer.begin(), photon_buffer.end()); }
    }

    // Build the photon map
    float radius = base_radius / std::pow(float(iter), 0.5f * (1.0f - alpha));
    PhotonMap photon_map(photons, radius);

    // Trace the eye paths
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < img.height; y++) {
        UniformSampler sampler(sampler_seed(y, iter));

        for (int x = 0; x < img.width; x++) {
            auto ray = scene.camera->gen_ray((x + sampler()) * kx - 1.0f, 1.0f - (y + sampler()) * ky);
            debug_raster(x, y);
            img(x, y) += atomically(rgba(eye_trace(ray, scene, photon_map, sampler, img.width * img.height), 1.0f));
        }
    }
}
