#pragma once

#include "renderer/renderer_utils.h"
#include "types.h"
struct alignas(STD140_ALIGNEMENT) Brdf {
  Color attenuation;
  float roughness;

  constexpr Brdf(Color attenuation_, float roughness_)
      : attenuation(attenuation_), roughness(roughness_) {}

  constexpr Brdf() : attenuation(0.906f, 0.906f, 0.906f, 1.f), roughness(0.5) {}
};

struct alignas(STD140_ALIGNEMENT) Material {
  Color emission;
  Brdf brdf;

  constexpr Material(Color emission_, Brdf brdf_)
      : emission(emission_), brdf(brdf_) {}

  constexpr Material() : emission(0), brdf() {}
};

inline constexpr Material DEFAULT_MATERIAL = Material();
