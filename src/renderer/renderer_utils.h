#pragma once

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

  bool contains(float t) const { return min <= t && t <= max; }
  bool contains_open(float t) const { return min < t && t < max; }
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
#ifdef NDEBUG
      LOG(2, "[BBox error] axis is not 0, 1, or 2, return INTERVAL_EMPTY");
#endif
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

      if (t_min <= t_max)
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


// -- Vectors utils 

using glm::xyz;