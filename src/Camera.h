#pragma once

#include "types.h"

struct Camera {
  float aspectRatio = 1.0;
  float vFov = M_PI / 2;

  float defocusAngle = 0;
  float focusDist = 10;

  glm::vec3 lookFrom{0, 0, -10};
  glm::vec3 lookAt{0, 0, 0};
  glm::vec3 vUp{0, 1, 0};

  struct CameraRenderInfo {
    glm::vec3 center;
    glm::vec3 view_u, view_v;
    glm::vec3 dt_u, dt_v;                     // differances between two pixel
    glm::mat3 uvw;                            // Camera frame basis
    glm::vec3 defocus_disk_u, defocus_disk_v; // Defocus disk
    glm::vec3 viewport_uper_left;
    glm::vec3 px00_loc;

    glm::vec3 defocus_disk_sample() const {
      glm::vec2 p = random_in_unit_disk();
      return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
  };

  CameraRenderInfo get_render_info(size_t width, size_t heigth) const {
    float aspect_ratio = static_cast<float>(width) / static_cast<float>(heigth);

    CameraRenderInfo result;

    result.center = lookFrom;
    float h = std::tan(vFov / 2);
    float view_heigth = 2 * h * focusDist;
    float view_width = view_heigth * aspect_ratio;

    glm::vec3 w = glm::normalize(lookFrom - lookAt);
    glm::vec3 u = glm::normalize(glm::cross(vUp, w));
    glm::vec3 v = glm::cross(w, u);
    result.uvw = glm::mat3(u, v, w);

    result.view_u = view_width * u;
    result.view_v = view_heigth * v;

    result.dt_u = result.view_u / static_cast<float>(width);
    result.dt_v = result.view_v / static_cast<float>(heigth);

    float defocus_rad = focusDist * std::tan(defocusAngle / 2.f);
    result.defocus_disk_u = u * defocus_rad;
    result.defocus_disk_v = v * defocus_rad;

    result.viewport_uper_left =
        lookFrom - (focusDist * w) - result.view_u / 2.f - result.view_v / 2.f;

    result.px00_loc =
        result.viewport_uper_left + 0.5f * (result.dt_u + result.dt_u);

    return result;
  }
};