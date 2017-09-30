#ifndef RANDOM_H
#define RANDOM_H

#include "float3.h"
#include "color.h"
#include "core\matrix.h"
#include "core\rtfloat4.h"
#include "algorithms\Matrix4x4.h"
#include <algorithm>
#include <iostream>
/// Local coordinates for shading.
struct LocalCoords {
    float3 n;           ///< Normal
    float3 t;           ///< Tangent
    float3 bt;          ///< Bitangent
    LocalCoords() {}
    LocalCoords(const float3& n, const float3& t, const float3& bt) : n(n), t(t), bt(bt) {}
    operator rt::Matrix() const 
    {
        return rt::Matrix(rt::Float4(n.x, t.x, bt.x, 0),rt::Float4(n.y, t.y, bt.y, 0),rt::Float4(n.z, t.z, bt.z, 0),rt::Float4( 0, 0, 0, 1));
    };
};

inline float3 operator* (const LocalCoords &l, const float3& v) {
    return float3(dot(l.n, v), dot(l.t, v), dot(l.bt, v));
}

inline float3 operator* (const rt::Matrix &m, const float3& v) {
    rt::Float4 res4 = m * rt::Float4(v.x, v.y, v.z, 1);
    return float3(res4[0], res4[1], res4[2]);
}


/// Generates local coordinates given a normal vector.
inline LocalCoords gen_local_coords(const float3& n) {
    float3 t;
    if (n.x != 0 || n.y != 0) {
        t = normalize(cross(n, float3(0, 0, 1)));
    } else {
        t = float3(1, 0, 0);
    }
    float3 bt = cross(n, t);
    return LocalCoords(n, t, bt);
}

/// Direction sample, from sampling a set of directions.
struct DirSample {
    float3 dir;
    float pdf;
    DirSample() {}
    DirSample(const float3& d, float p) : dir(d), pdf(p) {}
};

/// Evaluates the probability to sample a direction on a uniform sphere.
inline float uniform_sphere_pdf() {
    return 1.0f / (4.0f * pi);
}

/// Samples a sphere uniformly.
inline DirSample sample_uniform_sphere(float u, float v) {
    const float c = 2.0f * v - 1.0f;
    const float s = std::sqrt(1.0f - c * c);
    const float phi = 2.0f * pi * u;
    const float x = s * std::cos(phi);
    const float y = s * std::sin(phi);
    const float z = c;
    return DirSample(float3(x, y, z), uniform_sphere_pdf());
}

/// Evaluates the probability to sample a direction on a cosine-weighted hemisphere.
inline float cosine_hemisphere_pdf(float c) {
    // TODO: "c" is the cosine of the direction.
    // You should return the corresponding pdf.
    
    return c / pi;
    //return  1 / (2*pi); // Fix: Not sure why this pdf is producing right result
}

/// Samples a hemisphere proportionally to the cosine with the normal.
inline DirSample sample_cosine_hemisphere(const LocalCoords& coords, float u, float v) {
    // TODO: Sample a direction on the hemisphere using a pdf proportional to cos(theta).
    // The hemisphere is defined by the coordinate system "coords".
    // "u" and "v" are random numbers between [0, 1].

	float sint = sqrtf(1 - v);
	float phi = 2.f * (float)pi * u;
    float3 dir(cosf(phi) * sint, sinf(phi) * sint, sqrtf(v));

    float3 res = dir.x * coords.bt + dir.y * coords.t + dir.z * coords.n;

    return DirSample(res, sqrtf(v) / pi);
    //return DirSample(res, 1 /  (2*pi));
}

/// Evaluates the probability to sample a direction on a power-cosine-weighted hemisphere.
inline float cosine_power_hemisphere_pdf(float c, float k) {
    // TODO: "c" is the cosine of the direction, and k is the power.
    // You should return the corresponding pdf.
    
	return powf(c, k) * (k + 1) / (2.f * pi);
}

/// Samples a hemisphere proportionally to the cosine lobe spanned by the normal.
inline DirSample sample_cosine_power_hemisphere(const LocalCoords& coords, float k, float u, float v) {
    // TODO: Sample a direction on the hemisphere using a pdf proportional to cos(theta)^k.
    // The hemisphere is defined by the coordinate system "coords".
    // "u" and "v" are random numbers between [0, 1].

	float v1 = powf(v, 1.f / (k + 1.f));
	float v2 = v1 * v1;
	float phi = 2.f * pi * u;
	float sinv2 = sqrtf(1 - v2);
	float3 dir(cosf(phi) * sinv2, sinf(phi) * sinv2, v1);
	float3 res = dir.x * coords.bt + dir.y * coords.t + dir.z * coords.n;

	return DirSample(res, v1 * (k + 1) / (2.f * pi));
}

/// Returns the survival probability of a path, given its contribution.
/// \param c   the contribution of the path
/// \param max the maximum survival probability allowed
inline float russian_roulette(const rgb& c, float max = 0.75f) {
    return std::min(max, dot(c, luminance) * 2.0f);
}

#endif // RANDOM_H
