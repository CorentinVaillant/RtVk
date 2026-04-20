#include "Scene.h"
#include "Camera.h"
#include "hittables/Model.h"
#include "hittables/SceneCollection.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/math.hpp>

void load_scene_lights(std::vector<Light> &out, fastgltf::Asset &asset,
                       fastgltf::Node &node,
                       glm::mat4 transform = glm::mat4(1)) {

  if (node.lightIndex.has_value()) {
    auto &gltf_light = asset.lights[node.lightIndex.value()];

    transform *= gltf_transform_to_glm(node.transform);

    if (gltf_light.type == fastgltf::LightType::Point) {

      auto e = gltf_light.color * gltf_light.intensity;
      glm::vec3 pos = xyz(glm::vec4(0, 0, 0, 1) * transform);
      float radius = gltf_light.range.value_or(0);

      out.emplace_back(LightPoint{
          .emission = glm::vec4{e[0], e[1], e[2], 0.f},
          .position = pos,
          .radius = radius,
      });
      LOG(5, "Loaded light : {}", node.name);
    } else if (gltf_light.type == fastgltf::LightType::Directional) {
      auto e = gltf_light.color * gltf_light.intensity;
      glm::vec3 pos = xyz(glm::vec4(0, 0, 0, 1) * transform);
      glm::vec3 direction = -glm::normalize(pos); // pos -> world_ origin

      out.emplace_back(LightSun{
          .emission = glm::vec4{e[0], e[1], e[2], 0.f},
          .forward_vec = direction,
          .angle = 0.5f, // hardcoded, gltf does not provide smt like that

      });
    }

    for (size_t child_idx : node.children) {
      load_scene_lights(out, asset, asset.nodes[child_idx], transform);
    }
  }
}

std::vector<Scene> Scene::load_from_gltf(std::filesystem::path gltf_path) {
  auto gltf_expected = fastgltf::GltfDataBuffer::FromPath(gltf_path);
  if (gltf_expected.error() != fastgltf::Error::None) {
    LOGWARN("Could not read gltf file: {}",
            static_cast<uint64_t>(gltf_expected.error()));
    return {};
  }

  constexpr fastgltf::Options LOAD_OPTIONS =
      fastgltf::Options::LoadExternalBuffers;
  constexpr fastgltf::Extensions EXTENSIONS =
      fastgltf::Extensions::KHR_lights_punctual;

  fastgltf::Parser parser(EXTENSIONS);
  auto asset_expected = parser.loadGltf(gltf_expected.get(),
                                        gltf_path.parent_path(), LOAD_OPTIONS);
  if (asset_expected.error() != fastgltf::Error::None) {
    LOGWARN("Could not parse gltf file: {}",
            static_cast<uint64_t>(asset_expected.error()));
    return {};
  }

  fastgltf::Asset &asset = asset_expected.get();

  // -- Materials
  // Built once and shared across all scenes from this asset
  std::vector<Material> materials = {DEFAULT_MATERIAL}; // index 0 = default
  materials.reserve(asset.materials.size() + 1);

  for (const fastgltf::Material &mat : asset.materials) {
    Material m{};
    m._albedo = {
        mat.pbrData.baseColorFactor[0],
        mat.pbrData.baseColorFactor[1],
        mat.pbrData.baseColorFactor[2],
        mat.pbrData.baseColorFactor[3],
    };
    m._roughness = mat.pbrData.roughnessFactor;
    m._emission = glm::vec4(mat.emissiveFactor[0], mat.emissiveFactor[1],
                            mat.emissiveFactor[2], 0.f) *
                  mat.emissiveStrength;
    materials.emplace_back(m);
  }

  // -- Camera
  // Built once each Scene gets a copy of camera list

  // Transforms
  std::vector<glm::mat4> camera_transforms(asset.cameras.size(),
                                           glm::mat4(1.0f));
  for (const fastgltf::Node &node : asset.nodes) {
    if (node.cameraIndex.has_value())
      camera_transforms[node.cameraIndex.value()] =
          gltf_transform_to_glm(node.transform);
  }

  // Camera
  std::vector<Camera> cameras;
  cameras.reserve(asset.cameras.size());
  for (size_t i = 0; i < asset.cameras.size(); ++i) {
    const fastgltf::Camera &gltf_cam = asset.cameras[i];
    const glm::mat4 &world = camera_transforms[i];

    Camera cam{};

    cam.lookFrom = world * glm::vec4(0, 0, 0, 1); // origin in world space
    cam.lookAt = world * glm::vec4(0, 0, -1, 1);  // one unit ahead along -Z
    cam.vUp = -world * glm::vec4(0, 1, 0, 0); // world-space up (w=0, direction)

    std::visit(fastgltf::visitor{
                   [&](const fastgltf::Camera::Perspective &p) {
                     cam.vFov = p.yfov;
                     cam.aspectRatio = p.aspectRatio.value_or(16.0f / 9.0f);
                   },
                   [&](const fastgltf::Camera::Orthographic &) {

                   },
               },
               gltf_cam.camera);

    cameras.emplace_back(cam);
  }

  if (cameras.empty())
    cameras.push_back(Camera());

  // -- One scene per gltf scene
  std::vector<Scene> scenes;
  scenes.reserve(asset.scenes.size());

  for (fastgltf::Scene &gltf_scene : asset.scenes) {

    // Getting lights to the scene
    std::vector<Light> lights;
    for (size_t node_idx : gltf_scene.nodeIndices) {
      auto &node = asset.nodes[node_idx];
      load_scene_lights(lights, asset, node);
    }
    // add a dummy light
    lights.push_back(LightPoint{glm::vec4{0}, glm::vec3(9999), 0.1});

    // Build geometry into the collection structure
    scenes.emplace_back(Scene{
        ._camera = cameras,
        ._active_camera = 0,
        ._collection = SceneCollection(asset, gltf_scene),
        ._materials = materials,
        ._time = 0.0f,
    });
  }

  // glTF default scene
  if (asset.defaultScene.has_value() && scenes.size() > 1) {
    size_t def = asset.defaultScene.value();
    if (def != 0)
      std::swap(scenes[0], scenes[def]);
  }

  return scenes;
}