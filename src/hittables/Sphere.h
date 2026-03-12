#pragma once

#include "types.h"

#include "hittables/Hittable.h"

class Sphere : public IHittable {
public:
  struct SphereGpu {
    glm::vec4 _center;
    float _radius;
  };

  Sphere(glm::vec3 center, float radius)
      : _center(center, 1), _radius(radius) {}

  ~Sphere() {}

  bool hit(Ray r, Interval ray_t, HitRecord *rec) const override {
    // Fancy quadratic formula

    glm::vec3 oc = xyz(_center) - r.origin;
    float a = lenght_sq(r.direction);
    float h = dot(r.direction, oc);
    float c = lenght_sq(oc) - _radius * _radius;

    float discriminant = h * h - a * c;
    if (discriminant < 0)
      return false;

    float delta = std::sqrt(discriminant);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!ray_t.contains_open(root)) {
      root = (h + delta) / a;
      if (!ray_t.contains_open(root))
        return false;
    }

    rec->t = root;
    rec->p = r.at(root);

    glm::vec3 out_normal = (rec->p - xyz(_center)) / _radius;
    rec->set_face_normal(r.direction, out_normal);

    return true;
  }
  BBox get_bbox() const override {
    return BBox(_center - _radius, _center + _radius);
  }

  /// Should return the size taken by the object inside a Vulkan Buffer.
  /// Should be identical for each objects of the same types
  uint32_t get_in_buffer_size() const override { return sizeof(Sphere); }
  // Should return the next write address
  uint32_t write_in_buffer(Buffer<> &buffer, uint32_t index) const override {
    SphereGpu gpu_instance = {_center, _radius};
    buffer.write(index, sizeof(Sphere),
                 reinterpret_cast<const uint8_t *>(&gpu_instance));
    return index + sizeof(Sphere);
  };

private:
  glm::vec4 _center;
  float _radius;
};
