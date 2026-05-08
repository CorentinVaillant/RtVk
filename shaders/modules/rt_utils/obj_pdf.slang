implementing rt_utils;

import "../payloads";
import "../pdf";
import "../lights";

public struct LightScenePdf
{
    public RaytracingAccelerationStructure scene;
    public Vec3 origin;
    public float threshold;

    public __init(RaytracingAccelerationStructure scene,
                  Vec3 origin,
                  float threshold = 0.1)
    {
        this.scene = scene;
        this.origin = origin;
        this.threshold = threshold;
    }

    public float value(Vec3 direction, float ray_tmin = 1e-5, float ray_tmax = 1e5)
    {
        uint num_lights = punctualLightBuffer.getCount();
        if (num_lights == 0u)
            return 0.f;

        float inv_num_lights = 1.f / float(num_lights);

        RayDesc ray;
        ray.Direction = direction;
        ray.Origin = origin;
        ray.TMin = ray_tmin;
        ray.TMax = ray_tmax;

        PdfPayload payload;
        payload.hit = false;
        payload.min_light_val = threshold;
        payload.origin = origin;

        TraceRay(scene, RAY_FLAG_NONE, ~0, 1, 1, 1, ray, payload);

        if (!payload.hit)
        {
            float sun_pdf = 0.f;
            for (uint i = 0; i < num_lights; i++)
                if (punctualLightBuffer[i].type == LightType::Sun)
                {
                    let sun = punctualLightBuffer[i].get_sun_light();
                    sun_pdf += sun.pdf(ray);
                }

            return sun_pdf * inv_num_lights;
        }

        return payload.pdf_val * inv_num_lights;
    }

    public NVec3 generate(inout uint seed, in float tMax, out float t)
    {
        let random_index = random(seed, 0, punctualLightBuffer.getCount()) % punctualLightBuffer.getCount();

        // Take a random index from the light list
        let light : Light = punctualLightBuffer[random_index];

        switch (light.type)
        {
        case LightType::Point:
            return light.get_point_light().generate_random_dir(seed, origin, t);
        case LightType::Mesh:
            return light.get_mesh_light().generate_random_dir(seed, origin, t, indexBuffer, vertexBuffer);
        case LightType::Sun:
            t = tMax;
            return light.get_sun_light().generate_random_dir(seed);
        default:
            printf("[Warn ]default case handled in LightScenePdf::generate\n");
            t = -1;
            return NVec3(0); // Should not append
        }
    }
}
