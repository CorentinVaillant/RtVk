implementing rt_utils;

import "../bindings";

import "../utils";
import "../scene_data";
import "../payloads";
import "../attributs";
import "../scene_data";
import "../materials";

public void shadeDebug(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{
    payload.color = Color((attr.normal + 1) / 2.f, 1.f);
    return;
}

public void shade(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{
    // return shadeDebug(ray_in, payload, attr);
    payload.depth--;
    if (payload.depth <= 0)
    {
        payload.color = Color(0);
        return;
    }
    let brdf = materialBuffer[attr.material_index];

    payload.color = brdf.albedo;
}

