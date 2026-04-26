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
                  float threshold = 1)
    {
        this.scene = scene;
        this.origin = origin;
        this.threshold = threshold;
    }

    public float value(Vec3 direction)
    {
        RayDesc ray;
        ray.Direction = direction;
        ray.Origin = origin;
        ray.TMin = 0.0001f;
        ray.TMax = 10000.f;

        PdfPayload payload;
        payload.min_light_val = 1.f;

        TraceRay(scene, ~0, 0, 0, 0, 0, ray, payload);

        if (!payload.hit)
        { // TODO sort the light buffer before uploading, and pass a sun index to speed up the process
            for (uint i = 0; i < punctualLightBuffer.getCount(); i++)
                if (punctualLightBuffer[i].type == LightType::Sun)
                {
                    let sun = punctualLightBuffer[i].get_sun_light();
                    return 1.f / (M_TWO_PI_F * sq(sun.angle));
                }

            return 0.f;
        }

        return payload.pdf_val;
    }

    public Vec3 generate(inout uint seed, in float tMax, out float t)
    {
        // Take a random index from the light list
        let index : uint = clamp(random(seed, 0, punctualLightBuffer.getCount()), 0, punctualLightBuffer.getCount());
        let light : Light = punctualLightBuffer[index];

        switch (light.type)
        {
        case LightType::Point:
            return light.get_point_light().generate_random_dir(seed, origin, t);
        case LightType::Mesh:
            return light.get_mesh_light().generate_random_dir(seed, origin, t, indexBuffer, vertexBuffer);
        case LightType::Sun:
            t = tMax;
            return light.get_sun_light().generate_random_dir(seed, origin);
        default:
            t = -1;
            return Vec3(0); // should not append
        }
    }
}
