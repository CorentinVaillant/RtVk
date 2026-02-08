#pragma once

#include "types.h"

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

class IHittable {
public:
  virtual bool hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual BBox get_bbox() const = 0;

  virtual ~IHittable() = default;
};

template <typename T>
concept Hittable = std::is_base_of_v<IHittable, T>;

class IAccStruct {
public:
  virtual ~IAccStruct() = default;

  static constexpr uint32_t MISS_INDEX = -1;
  virtual uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual std::optional<const IHittable *> get_hitted(uint32_t index) const = 0;
};

class HittableVector : public IAccStruct {
public:
  // -- Constructors
  HittableVector() = default;

  template <Hittable T> HittableVector(std::vector<T> &&objects) {
    for (auto &&object : std::move(objects)) {
      _objects.push_back(std::make_unique<T>(std::move(object)));
    }
  }

  NO_COPY(HittableVector);

  ~HittableVector() {}

  // move constructors
  HittableVector(HittableVector &&other)
      : _objects(std::move(other._objects)) {}

  HittableVector &operator=(HittableVector &&other) {
    if (this != &other) {
      _objects = std::move(other._objects);
    }
    return *this;
  }

  // -- Methods
  template <Hittable Hit> void push(Hit &&object) {
    _objects.push_back(std::make_unique(std::move(object)));
  }

  // -- IAccStruct impl
  uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const override {
    float closest_so_far = ray_t.max;
    uint32_t closest_index = MISS_INDEX;
    HitRecord temp_rec;

    for (uint32_t i = 0; i < _objects.size(); i++) {
      const IHittable &object = *_objects[i].get();
      if (object.hit(r, Interval(ray_t.min, closest_so_far), &temp_rec)) {
        closest_index = i;
        closest_so_far = temp_rec.t;
        *records = temp_rec;
      }
    }
    return closest_index;
  }
  std::optional<const IHittable *> get_hitted(uint32_t index) const override {
    return (index < _objects.size())
               ? std::optional<const IHittable *>{_objects[index].get()}
               : std::nullopt;
  }

private:
  // -- Members
  std::vector<std::unique_ptr<IHittable>> _objects;
};