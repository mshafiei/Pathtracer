#include "../scene.h"
#include "../color.h"
#include "../samplers.h"
#include "../cameras.h"
#include "../hash.h"
#include "../debug.h"
#include <iostream>
#include <math.h>
//#define BASIC_PATH_TRACER 1
#define NEXT_EVENT_ESTIMATOR 2
//#define MULTIPLE_IMPORTANCE_SAMPLING 3

void NextEventEstimator(rgb& irradiance, float &pNE, const Material &mat, const float3 &out, const SurfaceParams &surf, const Scene& scene, Sampler& sampler, const rgb &throughput,float &pBRDF)
{
	int lighti = std::floor(sampler() * (((float)scene.lights.size()) - 0.01));
	float pLight = 1.0f / scene.lights.size();
	auto lightSample = scene.lights[lighti].get()->sample_direct(surf.point, sampler);
	float d = length(lightSample.pos - surf.point);
	auto sampledDir = (lightSample.pos - surf.point) / d;
	//pNE = (scene.lights.size() * (lightSample.pdf_area)) * (d * d / lightSample.cos);
	pNE = lightSample.pdf_area * (d * d / lightSample.cos) * pLight;
	auto occHit = scene.occluded(Ray(surf.point, sampledDir, 0.0001f, d-0.0001f));
	//pNE *= pBRDF;
	if (!occHit)//not occluded
	{

		float cosTheta = std::max(dot(surf.coords.n, sampledDir), 0.0f);
        auto brdf = mat.bsdf->eval(sampledDir, surf, out);//
		irradiance = brdf * cosTheta * lightSample.intensity / pNE * throughput;
	}
    pBRDF = mat.bsdf->pdf(sampledDir, surf, out);
}

void BrdfEstimator()
{

}

/// Path Tracing with MIS and Russian Roulette.
static rgb path_trace(Ray ray, const Scene& scene, Sampler& sampler) {
    static constexpr float offset = 1e-4f;
    rgb color(0.0f),throughput(1.0f);

    ray.tmin = offset;
    int i = 0;
    Bsdf::Type prevMat = Bsdf::Type::Specular;
    float sp = 0;
	float wNE, wBRDF, pBRDF = 1;
    while (true) {
        Hit hit = scene.intersect(ray);
        if (hit.tri < 0) break;
        

        auto surf = scene.surface_params(ray, hit);
        auto mat = scene.material(hit);
        auto out = -ray.dir;

        if (auto light = mat.emitter ) {
            // Direct hits on a light source
            if (surf.entering ) {
#ifdef NEXT_EVENT_ESTIMATOR
                if(prevMat == Bsdf::Type::Specular)
                    color += throughput * light->emission(out, surf.uv.x, surf.uv.y).intensity;
#elif BASIC_PATH_TRACER
                color += throughput * light->emission(out, surf.uv.x, surf.uv.y).intensity;
#else
                auto e = light->emission(out, surf.uv.x, surf.uv.y);
                float pNE = e.pdf_area * (hit.t * hit.t / dot(surf.coords.n, out)) * 1 / scene.lights.size();
                float wBRDF = prevMat == Bsdf::Type::Specular ? 1.0f : pBRDF / (pBRDF + pNE);
                color += wBRDF * throughput * e.intensity;
#endif
            }
            break;
        }
        // Materials without BSDFs act like black bodies
        if (!mat.bsdf) break;

        bool specular = mat.bsdf->type() == Bsdf::Type::Specular;
		
		rgb directIrradiance(0);
		float pNE = 0;
		NextEventEstimator(directIrradiance, pNE, mat, out, surf, scene, sampler, throughput, pBRDF);

        auto sample = mat.bsdf->sample(sampler, surf, out);
        ray.dir = sample.in;
        ray.org = surf.point;

        if (sample.pdf == 0) break;
#ifdef BASIC_PATH_TRACER
		throughput *= (sample.color / sample.pdf);
#elif NEXT_EVENT_ESTIMATOR
        
		throughput *= (sample.color / sample.pdf);
        if (mat.bsdf->type() != Bsdf::Type::Specular)
		    color += directIrradiance;
#elif MULTIPLE_IMPORTANCE_SAMPLING
        wNE = pNE / (pBRDF + pNE);
		color += directIrradiance * wNE;
		throughput *= (sample.color / pBRDF);
#endif
        float q = 1 - russian_roulette(sample.color / sample.pdf);

        if (sampler() < q)
            break;
        else
            throughput *= 1 / (1 - q);
        
        ++i;
        prevMat = mat.bsdf->type();
        pBRDF = sample.pdf;

    }
    return color;
}


void render_pt(const Scene& scene, Image& img, int iter) {
    auto kx = 2.0f / (img.width - 1);
    auto ky = 2.0f / (img.height - 1);

    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < img.height; y++) {
        UniformSampler sampler(sampler_seed(y, iter));

        for (int x = 0; x < img.width; x++) {
            auto ray = scene.camera->gen_ray(
                (x + sampler()) * kx - 1.0f,
                1.0f - (y + sampler()) * ky);

            debug_raster(x, y);
            img(x, y) += rgba(path_trace(ray, scene, sampler), 1.0f);
        }
    }
}
