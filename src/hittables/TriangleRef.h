#pragma once

#include "glm/ext/vector_float3.hpp"
#include "renderer/renderer_utils.h"
#include "types.h"

class Mesh;

// from
// https://github.com/CorentinVaillant/travaux_TER/blob/master/rt_series/rt_the_rest_of_my_life/include/Triangle.hpp
class TriangleIndices {
public:
  friend Mesh;

  static constexpr uint32_t BINDING = 7;
  static constexpr uint32_t HIT_GROUP = 0;

private:
  // -- Attributs --
  uint32_t _v1, _v2, _v3;

  // -- Contructors --
public:
  TriangleIndices(uint32_t v1_index, uint32_t v2_index, uint32_t v3_index)
      : _v1(v1_index), _v2(v2_index), _v3(v3_index) {}

  // -- Methods --
public:
  const Vertex &v1(const Vertex *vertices) const { return vertices[_v1]; }
  const Vertex &v2(const Vertex *vertices) const { return vertices[_v2]; }
  const Vertex &v3(const Vertex *vertices) const { return vertices[_v3]; }

  const glm::vec3 &x(const Vertex *vertices) const {
    return vertices[_v1].position;
  }
  const glm::vec3 &y(const Vertex *vertices) const {
    return vertices[_v2].position;
  }
  const glm::vec3 &z(const Vertex *vertices) const {
    return vertices[_v3].position;
  }

  float area(const Vertex *vertices) const {
    glm::vec3 e1 = y(vertices) - x(vertices);
    glm::vec3 e2 = z(vertices) - x(vertices);

    return 0.5 * glm::length(glm::cross(e1, e2));
  }

  glm::vec3 get_normal(const Vertex *v) const {
    // if no valid normals, compute normal from the position
    if (vec_near_zero(v1(v).normal) && vec_near_zero(v2(v).normal) &&
        vec_near_zero(v3(v).normal))
      return (glm::normalize(glm::cross(y(v) - x(v), z(v) - x(v))));
    // else mean of the 3
    else
      return (v1(v).normal + v2(v).normal + v3(v).normal) / 3.0f;
  }

  BBox get_bbox(const Vertex *v) const {
    BBox bbox1 = BBox(x(v), y(v));
    BBox bbox2 = BBox(x(v), z(v));
    return BBox(bbox1, bbox2).padded();
  }

  // IHittable impl

  // Using Möller–Trumbore intersection algorithm
  bool hit(const Vertex *vertices, Ray r, Interval ray_t,
           HitRecord *records) const {
    static constexpr float EPSILON = 1e-8;

    glm::vec3 edge1 = y(vertices) - x(vertices);
    glm::vec3 edge2 = z(vertices) - x(vertices);

    glm::vec3 h = glm::cross(r.direction, edge2);
    float a = glm::dot(edge1, h);

    // parallel case
    if (fabs(a) < EPSILON)
      return false;

    float f = 1.f / a;
    glm::vec3 s = r.origin - x(vertices);
    float u = f * dot(s, h);

    if (!Interval(0.f, 1.f).contains(u))
      return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(r.direction, q);

    if (!Interval(0.f, 1.f).contains(v))
      return false;

    float t = f * dot(edge2, q);
    if (!ray_t.contains(t))
      return false;

    records->t = t;
    records->p = r.at(t);
    records->uv = v1(vertices).uv(); // *~ fixme
    records->set_face_normal(r.direction, get_normal(vertices));

    return true;
  }
};