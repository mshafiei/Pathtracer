#ifndef MATERIALS_H
#define MATERIALS_H

#include <memory>
#include <cassert>
#include <cmath>
#include <type_traits>

#include "color.h"
#include "float3.h"
#include "common.h"
#include "textures.h"
#include "samplers.h"

/// Sample returned by a BSDF, including direction, pdf, and color.
struct BsdfSample {
    float3 in;                  ///< Sampled direction
    float pdf;                  ///< Probability density function, evaluated for the direction
    rgb color;                  ///< Color of the sample (BSDF value)

    BsdfSample() {}
    BsdfSample(const float3& i, float p, const rgb& c) : in(i), pdf(p), color(c) {}
};

/// Surface parameters for a given point.
struct SurfaceParams {
    bool entering;              ///< True if entering the surface
    float3 point;               ///< Hit point in world coordinates
    float2 uv;                  ///< Texture coordinates
    float3 face_normal;         ///< Geometric normal
    LocalCoords coords;         ///< Local coordinates at the hit point, w.r.t shading normal
};

class Light;
class Bsdf;

/// A material is a combination of a BSDF and an optional light emitter.
struct Material {
    const Bsdf* bsdf;       /// BSDF associated with the material (if any)
    const Light* emitter;   /// Light associated with the material (if any)

    Material() {}
    Material(const Bsdf* f = nullptr,
             const Light* e = nullptr)
        : emitter(e)
        , bsdf(f)
    {}
};

/// Base class for BSDFs.
class Bsdf {
public:
    /// Classification of BSDF shapes
    enum class Type {
        Diffuse  = 0,       ///< Mostly diffuse, i.e no major features, mostly uniform
        Glossy   = 1,       ///< Mostly glossy, i.e hard for Photon Mapping
        Specular = 2        ///< Purely specular, i.e merging/connections are not possible
    };

    Bsdf(Type ty)
        : ty(ty)
    {}

    virtual ~Bsdf() {}

    /// Returns the type of the BSDF, useful to make sampling decisions.
    Type type() const { return ty; }

    /// Evaluates the material for the given pair of directions and surface point. Does NOT include the cosine term.
    virtual rgb eval(const float3& /*in*/, const SurfaceParams& /*surf*/, const float3& /*out*/) const { return rgb(0.0f); }
    /// Samples the material given a surface point and an outgoing direction. The contribution DOES include the cosine term.
    virtual BsdfSample sample(Sampler& sampler, const SurfaceParams& surf, const float3& out, bool adjoint = false) const {
        return BsdfSample(surf.face_normal, 1.0f, rgb(0.0f));
    }
    /// Returns the probability to sample the given input direction (sampled using the sample function).
    virtual float pdf(const float3& /*in*/, const SurfaceParams& /*surf*/, const float3& /*out*/) const { return 0.0f; }

protected:
    /// Utility function to create a MaterialSample.
    /// It prevents corner cases that will cause issues (zero pdf, direction parallel/under the surface).
    /// When inverted is true, it expects the direction to be under the surface, otherwise above.
    template <bool inverted = false>
    BsdfSample make_sample(const float3& dir, float pdf, const rgb& color, const SurfaceParams& surf) const {
        return pdf > 0 && (inverted ^ (dot(dir, surf.face_normal) > 0)) ? BsdfSample(dir, pdf, color) : BsdfSample(dir, 1.0f, rgb(0.0f));
    }

    Type ty;
};

/// Purely Lambertian material.
class DiffuseBsdf : public Bsdf {
public:
    DiffuseBsdf(const Texture& tex)
        : Bsdf(Type::Diffuse)
        , tex(tex)
    {}

    rgb eval(const float3&, const SurfaceParams& surf, const float3&) const override final {
        return tex(surf.uv.x, surf.uv.y) * kd;
    }

    BsdfSample sample(Sampler& sampler, const SurfaceParams& surf, const float3&, bool) const override final {
        auto sample = sample_cosine_hemisphere(surf.coords, sampler(), sampler());
        return make_sample(sample.dir, sample.pdf, tex(surf.uv.x, surf.uv.y) * (std::max(dot(sample.dir, surf.coords.n), 0.0f) * kd), surf);
    }

    float pdf(const float3& in, const SurfaceParams& surf, const float3&) const override final {
        return cosine_hemisphere_pdf(dot(in, surf.coords.n));
    }

private:
    static constexpr float kd = 1.0f / pi;

    const Texture& tex;
};

/// Specular part of the modified (physically correct) Phong.
class GlossyPhongBsdf : public Bsdf {
public:
    GlossyPhongBsdf(const Texture& tex, float ns)
        : Bsdf(Type::Glossy)
        , tex(tex)
        , ns(ns)
        , ks((ns + 2) / (2.0f * pi))
    {}

    rgb eval(const float3& in, const SurfaceParams& surf, const float3& out) const override final {
        auto p = std::max(dot(in, reflect(out, surf.coords.n)), 0.0f);
        return tex(surf.uv.x, surf.uv.y) * std::pow(p, ns) * ks;
    }

    BsdfSample sample(Sampler& sampler, const SurfaceParams& surf, const float3& out, bool) const override final {
        auto coords = gen_local_coords(reflect(out, surf.coords.n));
        auto sample = sample_cosine_power_hemisphere(coords, ns, sampler(), sampler());
        auto p = std::max(dot(sample.dir, reflect(out, surf.coords.n)), 0.0f);
        return make_sample(sample.dir, sample.pdf, tex(surf.uv.x, surf.uv.y) * (std::max(dot(sample.dir, surf.coords.n), 0.0f) * std::pow(p, ns) * ks), surf);
    }

    float pdf(const float3& in, const SurfaceParams& surf, const float3& out) const override final {
        auto p = std::max(dot(in, reflect(out, surf.coords.n)), 0.0f);
        return cosine_power_hemisphere_pdf(p, ns);
    }

private:
    const Texture& tex;
    float ns, ks;
};

/// Purely specular mirror.
class MirrorBsdf : public Bsdf {
public:
    MirrorBsdf() : Bsdf(Type::Specular) {}

    BsdfSample sample(Sampler&, const SurfaceParams& surf, const float3& out, bool) const override final {
        return make_sample(reflect(out, surf.coords.n), 1.0f, rgb(1.0f, 1.0f, 1.0f), surf);
    }
};

/// BSDF that can represent glass or any separation between two mediums.
class GlassBsdf : public Bsdf {
public:
    GlassBsdf(float n1 = 1.0f, float n2 = 1.4f, const rgb& c = rgb(1.0f))
        : Bsdf(Type::Specular)
        , n1(n1)
        , n2(n2)
        , color(c)
    {}

    BsdfSample sample(Sampler& sampler, const SurfaceParams& surf, const float3& out, bool adjoint) const override final {
        const float k1 = surf.entering ? n1 : n2;
        const float k2 = surf.entering ? n2 : n1;
        const float cos_i = dot(out, surf.coords.n);

        const float k = k1 / k2;
        const float cos2_t = 1.0f - k * k * (1.0f - cos_i * cos_i);
        if (cos2_t > 0) {
            // Refraction
            const float cos_t = std::sqrt(cos2_t);
            const float F = fresnel_factor(k1, k2, cos_i, cos_t);
            if (sampler() > F) {
                const float3 t = (k * cos_i - cos_t) * surf.coords.n - k * out;
                const float adjoint_term = adjoint ? k * k : 1.0f;
                return make_sample<true>(t, 1.0f, color * adjoint_term, surf);
            }
        }

        // Reflection
        return make_sample(reflect(out, surf.coords.n), 1.0f, color, surf);
    }

private:
    /// Evaluates the fresnel factor for two different medium and the given cosines of the incoming/transmitted rays.
    static float fresnel_factor(float n1, float n2, float cos_i, float cos_t) {
        const float R_s = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t);
        const float R_p = (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t);
        return (R_s * R_s + R_p * R_p) * 0.5f;
    }

    float n1, n2;
    rgb color;
};

/// A BSDF that combines two materials.
class CombineBsdf : public Bsdf {
public:
    CombineBsdf(Type ty, const Bsdf* a, const Bsdf* b, float k)
        : Bsdf(ty), a(a), b(b), k(k)
    {}

    rgb eval(const float3& in, const SurfaceParams& surf, const float3& out) const override final {
        return lerp(a->eval(in, surf, out), b->eval(in, surf, out), k);
    }

    BsdfSample sample(Sampler& sampler, const SurfaceParams& surf, const float3& out, bool adjoint) const override final {
        return sampler() < k ? b->sample(sampler, surf, out, adjoint) : a->sample(sampler, surf, out, adjoint);
    }

    float pdf(const float3& in, const SurfaceParams& surf, const float3& out) const override final {
        return lerp(a->pdf(in, surf, out), b->pdf(in, surf, out), k);
    }

private:
    std::unique_ptr<const Bsdf> a, b;
    float k;
};

#endif // MATERIALS_H
