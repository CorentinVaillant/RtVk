#pragma once

#include "Camera.h"
#include "ImageBuffer.h"
#include "hittables/Hittable.h"
#include "types.h"

#include "Scene.h"
#include <cstddef>

class SimpleRenderer {
public:
  SimpleRenderer(ImageBuffer &&image_buffer)
      : _imgBuffer(std::move(image_buffer)) {}

  SimpleRenderer(size_t img_width, size_t img_heigth)
      : SimpleRenderer(ImageBuffer(img_width, img_heigth)) {}

  void render(const Scene &scene) {
    size_t img_width = _imgBuffer.get_width();
    size_t img_height = _imgBuffer.get_height();
    _camRenderInfo = scene.camera.get_render_info(img_width, img_height);
    for (size_t i = 0; i < img_width; i++)
      for (size_t j = 0; j < img_height; j++) {
        _imgBuffer.write_pixel(i, j, post_process(gen_ray(scene, i, j)));
      }
  }

  const ImageBuffer &get_img_buff() const { return _imgBuffer; }

protected:
  // Pipeline Simulation
  Color gen_ray(const Scene &scene, size_t i, size_t j) const {
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
                    uint32_t hit_index) const {
    return Color(record.normal / 2.f +  1.f, 1);
  }

  Color miss(const Scene &scene, Ray r) const { return Color(r.direction, 1); }

  Color post_process(Color color) const { return color; }

  // Helpers
  Ray get_ray(size_t i, size_t j, const Camera &cam) const {
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

  ImageBuffer _imgBuffer;
  // Uniforms simulation
  Camera::CameraRenderInfo _camRenderInfo;
};