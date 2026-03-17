#pragma once

#include "types.h"

#include "hittables/Hittable.h"

class Sphere : public IHittable {
public:
  struct SphereGpu {
    glm::vec4 _center;
    float _radius;
  };

  Sphere(glm::vec3 center, float radius) : data{glm::vec4(center, 1), radius} {}

  ~Sphere() {}

  bool hit(Ray r, Interval ray_t, HitRecord *rec) const override {
    // Fancy quadratic formula

    glm::vec3 oc = xyz(data._center) - r.origin;
    float a = lenght_sq(r.direction);
    float h = dot(r.direction, oc);
    float c = lenght_sq(oc) - data._radius * data._radius;

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

    glm::vec3 out_normal = (rec->p - xyz(data._center)) / data._radius;
    rec->set_face_normal(r.direction, out_normal);

    return true;
  }
  BBox get_bbox() const override {
    return BBox(data._center - data._radius, data._center + data._radius);
  }

  HittableInfo gpu_info() const override {
    return HittableInfo{
        .gpu_instance = reinterpret_cast<const uint8_t *>(&data),
        .binding = 3,
        .obj_size = 32,
        .obj_offset = 0,
    }; // 32 -> std140 alignement
  }

private:
  SphereGpu data;
};
