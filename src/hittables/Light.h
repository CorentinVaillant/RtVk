#pragma once

#include "renderer/renderer_utils.h"
#include "types.h"

struct LightPoint {
  glm::vec4 emission;
  glm::vec3 position;
  float radius;
};

struct LightMesh {
  glm::vec4 emission;
  float area;
  uint32_t start_index;
  uint32_t end_index;
};

struct LightSun {
  glm::vec4 emission;
  glm::vec3 forward_vec;
  float angle;
};

enum class LightType : uint32_t {
  Point = 0u,
  Mesh = 1u,
  Sun = 2u,
};

struct alignas(STD140_ALIGNEMENT) Light {
  LightType type;
  uint32_t start_index, end_index;
  uint32_t _pad;

  glm::vec4 emission;

  glm::vec3 position_forward_vec;
  float radius_angle_area;

  static constexpr uint32_t BINDING = 8;
  static constexpr uint32_t HIT_GROUP = 2;

  Light(LightPoint p)
      : type(LightType::Point), emission(p.emission),
        position_forward_vec(p.position), radius_angle_area(p.radius) {}

  Light(LightMesh m)
      : type(LightType::Mesh), start_index(m.start_index),
        end_index(m.end_index), emission(m.emission),
        radius_angle_area(m.area) {}

  Light(LightSun s)
      : type(LightType::Sun), emission(s.emission),
        position_forward_vec(s.forward_vec) {}

  std::optional<BBox> get_bbox() const {
    if (type == LightType::Point)
      return BBox(position_forward_vec, radius_angle_area).padded();
    else
      return std::nullopt;
  }
};