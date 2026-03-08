#pragma once

// std
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <ctype.h>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
// glm
#include <glm/common.hpp>
// vulkan
#define VK_NO_PROTOTYPES
#include <vulkan/vk_enum_string_helper.h>
// other
#include <fmt/core.h>
#include <utils/ansi_code.h>

#define NVERBOSE 4

// -- Log Levels --
enum LogLvl {
  SEVERE = 0,
  WARNING = 1,
  INFO = 2,
  CONFIG = 3,
  FINE = 4,
};

// -- Macros --

constexpr bool THROW_ON_MACRO_ERR = true;

#define NO_COPY(CLASS_NAME)                                                    \
  CLASS_NAME(const CLASS_NAME &) = delete;                                     \
  CLASS_NAME &operator=(const CLASS_NAME &) = delete;

#ifndef NVERBOSE
#define LOG(level, ...)                                                        \
  do {                                                                         \
  } while (0)
#else
#define LOG(level, ...)                                                        \
  do {                                                                         \
    if (NVERBOSE >= level) {                                                   \
      fmt::print("[{}LOG {}] {}::{} ", ansi_code::BBLU, ansi_code::reset,      \
                 __func__, __LINE__);                                          \
      fmt::println(__VA_ARGS__);                                               \
    }                                                                          \
  } while (0)
#endif

#define LOGOK(...)                                                             \
  do {                                                                         \
    fmt::print("[{}OK  {}] {}::{} ", ansi_code::BGRN, ansi_code::reset,        \
               __func__, __LINE__);                                            \
    fmt::println(__VA_ARGS__);                                                 \
  } while (0)

#define LOGWARN(...)                                                           \
  do {                                                                         \
    if (NVERBOSE >= LogLvl::WARNING)                                           \
      fmt::print("[{}WARN {}] {}::{} ", ansi_code::BYEL, ansi_code::reset,     \
                 __func__, __LINE__);                                          \
    fmt::println(__VA_ARGS__);                                                 \
  } while (0)

#define LOGERR(...)                                                            \
  {                                                                            \
    fmt::print("[{}ERR {}] {}::{} ", ansi_code::BRED, ansi_code::reset,        \
               __func__, __LINE__);                                            \
    fmt::println(__VA_ARGS__);                                                 \
    if (THROW_ON_MACRO_ERR)                                                    \
      throw std::runtime_error("Log err");                                     \
  }

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      LOGERR("Detected Vulkan error: \"{}\" -> {} ", #x,                       \
             string_VkResult(err));                                            \
      throw std::runtime_error("VkError");                                     \
    }                                                                          \
  } while (0)


// -- Common templates --

template <typename D, typename T>
concept DestructorOf = requires(D d, T t) {
  { d.release(t) };
  { d.discard(t) } -> std::same_as<T>;
};

template <typename T, typename D>
  requires DestructorOf<D, T>
class ScopeGuard {
public:
  ScopeGuard(T data, D destructor)
      : _data(data), _destructor(std::move(destructor)) {}

  ScopeGuard(ScopeGuard &&rval)
      : _data(std::move(rval._data)), _destructor(std::move(rval._destructor)),
        _is_active(rval._is_active) {
    rval._is_active = false;
  }

  ScopeGuard(D &&destructor)
    requires std::default_initializable<T>
      : ScopeGuard(T(), std::move(destructor)) {}

  NO_COPY(ScopeGuard);

  T &operator*() { return _data; }

  const T &operator*() const { return _data; }

  T &operator=(ScopeGuard &&rval) {
    if (this != &rval) {
      _data = rval._data;
      _destructor = rval._destructor;
      _is_active = rval._is_active;

      rval._is_active = false;
    }
    return *this;
  }

  // -- methods --
  T discard() & = delete;
  T discard() && {
    _is_active = false;
    return _destructor.discard(std::move(_data));
  }
  std::pair<T, D> discard_all() && {
    _is_active = false;
    return std::make_pair(std::move(_data), std::move(_destructor));
  }

  ~ScopeGuard() { _destructor.release(std::move(_data)); }

  // -- members --
private:
  T _data;
  D _destructor;
  bool _is_active = true;
};

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
