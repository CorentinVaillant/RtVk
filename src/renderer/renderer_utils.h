#pragma once

#include <cstdlib>
#include <volk.h>

#include "types.h"

// -- Common types --
struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;

  glm::vec3 at(float t) const { return direction * t + origin; }
};

using Color = glm::vec4;

inline constexpr Color BLACK = {0, 0, 0, 1};
inline constexpr Color WHITE = glm::vec4{1};
inline constexpr Color RED = {1, 0, 0, 1};
inline constexpr Color GREEN = {0, 1, 0, 1};
inline constexpr Color BLUE = {0, 0, 1, 1};

struct Interval {
  float min;
  float max;

  constexpr Interval(float min_, float max_) : min(min_), max(max_) {
    assert(max >= min);
  }

  constexpr Interval connexe_union(Interval other) {
    return Interval{
        this->min < other.min ? this->min : other.min,
        this->max > other.max ? this->max : other.max,
    };
  }

  constexpr Interval padded(float epsilon = 1e-8) const {
    return Interval{min - std::abs(epsilon), max + std::abs(epsilon)};
  }

  constexpr bool contains(float t) const { return min <= t && t <= max; }
  constexpr bool contains_open(float t) const { return min < t && t < max; }
};

inline constexpr Interval INTERVAL_REELS = {-INFINITY, INFINITY};
inline constexpr Interval INTERVAL_EMPTY = {INFINITY, -INFINITY};

struct BBox {
  Interval x, y, z;

  constexpr BBox(Interval x_int, Interval y_int, Interval z_int)
      : x(x_int), y(y_int), z(z_int) {}

  constexpr BBox(glm::vec3 a, glm::vec3 b)
      : x(a.x <= b.x ? Interval(a.x, b.x) : Interval(b.x, a.x)),
        y(a.y <= b.y ? Interval(a.y, b.y) : Interval(b.y, a.y)),
        z(a.z <= b.z ? Interval(a.z, b.z) : Interval(b.z, a.z)) {}

  constexpr BBox(glm::vec3 pos, float radius)
      : BBox(pos - radius /* * glm::vec3(1) */,
             pos + radius /* * glm::vec3(1) */) {}

  constexpr BBox(BBox b1, BBox b2)
      : x(b1.x.connexe_union(b2.x)), y(b1.y.connexe_union(b2.y)),
        z(b1.z.connexe_union(b2.z)) {}

  constexpr BBox padded(float epsilon = 1e-8) const {
    return BBox{x.padded(epsilon), y.padded(epsilon), z.padded(epsilon)};
  }

  const Interval &axis_interval(size_t axis) const {
    switch (axis) {
    case 0:
      return x;
      break;
    case 1:
      return y;
      break;
    case 2:
      return z;
      break;
    default:
      LOGWARN("(BBox error) axis is not 0, 1, or 2, return INTERVAL_EMPTY");
      return INTERVAL_EMPTY;
    }
  }

  bool hit(Ray ray, Interval ray_t = INTERVAL_REELS, float *t = nullptr) const {

    float t_min = ray_t.min;
    float t_max = ray_t.max;

    for (size_t axis = 0; axis < 3; axis++) {
      const Interval &ax = axis_interval(axis);
      if (fabsf(ray.direction[axis]) < 1e-8) {
        if (!ax.contains(ray.origin[axis]))
          return false;
        else
          continue;
      }

      const float div = 1.f / ray.direction[axis];

      float t0 = (ax.min - ray.origin[axis]) * div;
      float t1 = (ax.max - ray.origin[axis]) * div;

      if (t0 > t1)
        std::swap(t0, t1);

      t_min = std::max(t_min, t0);
      t_max = std::min(t_max, t1);

      if (t_min > t_max)
        return false;
    }

    if (t)
      *t = t_min;
    return true;
  }

  VkAabbPositionsKHR to_vk() const {
    return VkAabbPositionsKHR{.minX = x.min,
                              .minY = y.min,
                              .minZ = z.min,
                              .maxX = x.max,
                              .maxY = y.max,
                              .maxZ = z.max};
  }
};

inline constexpr BBox BBOX_UNIVERSE = {INTERVAL_REELS, INTERVAL_REELS,
                                       INTERVAL_REELS};
inline constexpr BBox BBOX_EMPTY = {INTERVAL_EMPTY, INTERVAL_EMPTY,
                                    INTERVAL_EMPTY};

struct HitRecord {
  glm::vec3 p;
  float t;
  glm::vec3 normal;
  glm::vec2 uv;
  bool front_face;

  void set_face_normal(glm::vec3 ray_dir, glm::vec3 out_normal) {
    front_face = glm::dot(ray_dir, out_normal) < 0;
    normal = front_face ? out_normal : -out_normal;
  }
};

struct HittableInfo {
  const uint8_t *gpu_instance;
  uint32_t binding;
  uint32_t obj_size, obj_offset;
};

struct alignas(STD140_ALIGNEMENT) Instance {
  uint32_t _materialId;
  uint32_t _indexOffset;

  static constexpr uint32_t BINDING = 4;
};

struct alignas(16) Vertex {
  glm::vec3 position;
  float u;
  glm::vec3 normal;
  float v;

  glm::vec2 uv() const { return glm::vec2(u, v); }

  static constexpr auto VK_FORMAT = VK_FORMAT_R32G32B32_SFLOAT;
  static constexpr uint32_t BINDING = 5;
};

struct alignas(16) Material {
  Color _albedo;
  Color _emission = Color(0);
  float _roughness;

  constexpr Material(Color albedo, Color emission, float roughness)
      : _albedo(albedo), _emission(emission), _roughness(roughness) {}

  constexpr Material()
      : _albedo(0.906f, 0.906f, 0.906f, 1.f), _emission(0.f), _roughness(0.5f) {
  }
};

inline constexpr Material DEFAULT_MATERIAL = Material();

// -- Vectors utils

using glm::xyz;