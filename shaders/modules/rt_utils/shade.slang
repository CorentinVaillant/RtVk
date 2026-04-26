implementing rt_utils;

import "../bindings";

import "../utils";
import "../scene_data";
import "../payloads";
import "../attributs";
import "../scene_data";
import "../materials";

public void shade_miss(inout RayPayload payload)
{
    Color sky_color = Vec4(Vec3(0.3), 1);

    uint num_of_light = punctualLightBuffer.getCount();
    for (uint i = 0; i < num_of_light; i++)
    {
        if (punctualLightBuffer[i].type == LightType::Sun)
        {
            LightSun sun = punctualLightBuffer[i].get_sun_light();

            let alignment = dot(normalize(sun.forward_vec), normalize(WorldRayDirection()));
            if (alignment > cos(sun.angle))
            {
                sky_color = sun.emission;
                break;
            }
        }
    }

    payload.color = sky_color;
}

public void shadeDebug(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{
    payload.color = Color((attr.normal + 1) / 2.f, 1.f);
    return;
}

public void shade(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{

    // shadeDebug(ray_in, payload, attr);
    // return;

    let tMax : float = 1e5;
    let tMin : float = 1e-2;

    if (payload.depth <= 0)
        return shade_miss(payload);
    payload.depth--;

    let hit = attr.position;
    let normal = attr.normal;
    let material = materialBuffer[attr.material_index];

    let color_from_emission : Color = material.emited(ray_in, attr, attr.uv);

    ScatterAttribut scattr;
    if (!material.scatter(ray_in, attr, payload.seed, scattr))
    {
        payload.color = color_from_emission;
        return;
    }

    if (scattr.skip_pdf)
    {
        TraceRay(scene, ~0, 0, 0, 0, 0, scattr.skip_pdf_ray, payload);
        payload.color *= scattr.attenuation;
        return;
    }

    let ray_origin = hit + normal * tMin * 2.0;

    let light_pdf = LightScenePdf(lightsScene, ray_origin);

    // generate scattered ray
    RayDesc scattr_ray;
    if (random(payload.seed, 0.f, 1.f) < 0.5)
    {
        scattr_ray = scattr.create_scatter_ray(ray_origin);
    }
    else
    {
        float t;
        scattr_ray.Direction = light_pdf.generate(payload.seed, tMax, t);
        scattr_ray.Origin = ray_origin;
        scattr_ray.TMin = tMin;
        scattr_ray.TMax = tMax;
    }

    let pdf_value : float = 0.5 * light_pdf.value(scattr_ray.Direction) + 0.5 * (scattr.pdf_val);

    TraceRay(scene, ~0u, 0, 0, 0, 0, scattr_ray, payload);

    let color_from_scatter = (scattr.attenuation * payload.color) / pdf_value;

    payload.color = color_from_emission + color_from_scatter;
}

