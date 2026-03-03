#pragma once

#include "Camera.h"
#include "ImageBuffer.h"
#include "Renderer.h"
#include "hittables/Hittable.h"
#include "types.h"

#include "Scene.h"

class CPURenderer : public Renderer {
public:
  CPURenderer(ImageBuffer &&img_buffer) : Renderer(std::move(img_buffer)) {}
  CPURenderer(size_t img_width, size_t img_heigth,
              ImgFormat format = ImgFormat::RGBA)
      : Renderer(ImageBuffer(img_width, img_heigth, format)) {}

  virtual void render(const Scene &scene) override {
    size_t img_width = _imgBuffer.get_width();
    size_t img_height = _imgBuffer.get_height();
    _camRenderInfo = scene.camera.get_render_info(img_width, img_height);
    for (size_t i = 0; i < img_width; i++)
      for (size_t j = 0; j < img_height; j++) {
        _imgBuffer.write_pixel(i, j, post_process(gen_ray(scene, i, j)));
      }
  }

  virtual ~CPURenderer()  = default;

protected:
  // Pipeline Simulation
  virtual Color gen_ray(const Scene &scene, size_t i, size_t j) const = 0;

  virtual Color closest_hit(const Scene &scene, HitRecord record,
                            uint32_t hit_index) const = 0;

  virtual Color miss(const Scene &scene, Ray r) const = 0;

  virtual Color post_process(Color color) const { return color; }

  // Helpers
  virtual Ray get_ray(size_t i, size_t j, const Camera &cam) const {
    float i_f = static_cast<float>(i);
    float j_f = static_cast<float>(j);

    glm::vec3 pixel_sample = _camRenderInfo.px00_loc +
                             i_f * _camRenderInfo.dt_u +
                             j_f * _camRenderInfo.dt_v;

    glm::vec3 ray_origin = cam.defocusAngle <= 0
                               ? _camRenderInfo.center
                               : _camRenderInfo.defocus_disk_sample();
    glm::vec3 ray_dir = pixel_sample - ray_origin;

    return {ray_origin, ray_dir};
  }

  // -- Members

protected:
  // Uniforms simulation
  Camera::CameraRenderInfo _camRenderInfo;
};

class SimpleCPURenderer : public CPURenderer {
public:
  SimpleCPURenderer(ImageBuffer &&img_buffer)
      : CPURenderer(std::move(img_buffer)) {}
  SimpleCPURenderer(size_t img_width, size_t img_heigth,
                    ImgFormat format = ImgFormat::RGBA)
      : CPURenderer(ImageBuffer(img_width, img_heigth, format)) {}

  ~SimpleCPURenderer() = default;

protected:
  Color gen_ray(const Scene &scene, size_t i, size_t j) const override {
    Ray ray = get_ray(i, j, scene.camera);
    HitRecord record;
    uint32_t hit_index = scene._accStruct->hit(ray, INTERVAL_REELS, &record);
    Color hit_color;
    if (hit_index != IAccStruct::MISS_INDEX)
      hit_color = closest_hit(scene, record, hit_index);
    else
      hit_color = miss(scene, ray);
    return post_process(hit_color);
  }

  Color closest_hit(const Scene &scene, HitRecord record,
                    uint32_t hit_index) const override {
    return Color(record.normal / 2.f + 0.5f, 1);
  }

  Color miss(const Scene &scene, Ray r) const override {
    return Color(glm::normalize(r.direction) / 2.f + 0.5f, 1);
  }
};
