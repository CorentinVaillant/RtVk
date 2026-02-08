#pragma once

// std
#include <array>
#include <cmath>
#include <cstddef>
#include <ctype.h>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>
// glm
#include <glm/common.hpp>
// other
#include <fmt/core.h>
#include <utility>

#define VERBOSE 4
#define NDEBUG 4

#define NO_COPY(CLASS_NAME)                                                    \
  CLASS_NAME(const CLASS_NAME &) = delete;                                     \
  CLASS_NAME &operator=(const CLASS_NAME &) = delete;

#ifndef VERBOSE
#define LOG(level, ...)                                                        \
  do {                                                                         \
  } while (0)
#else
#define LOG(level, ...)                                                        \
  do {                                                                         \
    if (VERBOSE >= level) {                                                    \
      fmt::print("[log {}::{}] ", __func__, __LINE__);                         \
      fmt::println(__VA_ARGS__);                                               \
    }                                                                          \
  } while (0)
#endif

// common types

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
};

inline constexpr BBox BBOX_UNIVERSE = {INTERVAL_REELS, INTERVAL_REELS,
                                       INTERVAL_REELS};
inline constexpr BBox BBOX_EMPTY = {INTERVAL_EMPTY, INTERVAL_EMPTY,
                                    INTERVAL_EMPTY};

// -- Common functions --

inline constexpr float lenght_sq(const glm::vec3 &v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

// -- Randoms

inline float random_float() {
  thread_local std::uniform_real_distribution<float> distrib(0.f, 1.f);
  thread_local std::mt19937 generator;
  return distrib(generator);
}

inline float random_float(float min, float max) {
  return min + (max - min) * random_float();
}

inline glm::vec2 random_in_unit_disk() {
  float theta = random_float(0, M_2_PI);
  float ro = random_float();

  return ro * glm::vec2(std::cos(theta), std::sin(theta));
}