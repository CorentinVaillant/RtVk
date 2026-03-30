#pragma once

#include "glm/geometric.hpp"
#include "renderer/renderer_utils.h"
#include "types.h"

struct Camera {
  float aspectRatio = 1.0;
  float vFov = M_PI / 2;

  float defocusAngle = 0;
  float focusDist = 10;

  glm::vec4 lookFrom{0, 5, -5, 1};
  glm::vec4 lookAt{0, 0, 0, 1};
  glm::vec4 vUp{0, 1, 0, 1};

  struct alignas(STD140_ALIGNEMENT) CameraRenderInfo {
    glm::vec4 center;
    glm::vec4 view_u, view_v;
    glm::vec4 dt_u, dt_v;                     // differances between two pixel
    glm::vec4 u, v, w;                        // Camera frame basis
    glm::vec4 defocus_disk_u, defocus_disk_v; // Defocus disk
    glm::vec4 viewport_uper_left;
    glm::vec4 px00_loc;

    glm::vec4 defocus_disk_sample(glm::vec4 center) const {
      glm::vec2 p = random_in_unit_disk();
      return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
  };

  // -- Methods --

  CameraRenderInfo get_render_info(size_t width, size_t heigth) const {
    float aspect_ratio = static_cast<float>(width) / static_cast<float>(heigth);

    CameraRenderInfo result;

    result.center = lookFrom;
    float h = std::tan(vFov / 2);
    float view_heigth = 2 * h * focusDist;
    float view_width = view_heigth * aspect_ratio;

    glm::vec3 w = glm::normalize(xyz(lookFrom - lookAt));
    glm::vec3 u = glm::normalize(glm::cross(xyz(vUp), w));
    glm::vec3 v = glm::cross(w, u);

    result.w = {w, 1};
    result.u = {u, 1};
    result.v = {v, 1};

    result.view_u = view_width * result.u;
    result.view_v = view_heigth * result.v;

    result.dt_u = result.view_u / static_cast<float>(width);
    result.dt_v = result.view_v / static_cast<float>(heigth);

    float defocus_rad = focusDist * std::tan(defocusAngle / 2.f);
    result.defocus_disk_u = result.u * defocus_rad;
    result.defocus_disk_v = result.v * defocus_rad;

    result.viewport_uper_left = lookFrom - (focusDist * result.w) -
                                result.view_u / 2.f - result.view_v / 2.f;

    result.px00_loc =
        result.viewport_uper_left + 0.5f * (result.dt_u + result.dt_v);

    return result;
  }

  void move_forward(float dist) {
    lookFrom += glm::normalize(lookAt - lookFrom) * dist;
  }

  void move_backward(float dist) { move_forward(-dist); }

  void move_upward(float dist) { lookFrom += glm::normalize(vUp) * dist; }

  void move_downward(float dist) { move_upward(-dist); }

  void move_left(float dist) {
    lookFrom -= glm::vec4(
        glm::normalize(glm::cross(xyz(lookAt - lookFrom), xyz(vUp))) * dist,
        lookFrom.w);
  }

  void move_right(float dist) {
    lookFrom += glm::vec4(
        glm::normalize(glm::cross(xyz(lookAt - lookFrom), xyz(vUp))) * dist,
        lookFrom.w);
  }
};