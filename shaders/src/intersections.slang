
import "../modules/bindings";
import "../modules/attributs";
import "../modules/scene_data";
import "../modules/geometry";
import "../modules/lights";

// -- Lights --
[shader("intersection")]
void lightIntersection()
{

    let light_index : uint = InstanceID();

    LightAttribut attr;

    RayDesc ray;
    ray.Direction = WorldRayDirection();
    ray.Origin = WorldRayOrigin();
    ray.TMin = RayTMin();
    ray.TMax = RayTCurrent();
    let light : Light = punctualLightBuffer[light_index];

    if (light.type == LightType::Point)
    {
        let light_point : LightPoint = light.get_point_light();
        attr.emision = light_point.emission;
        float t;
        if (!hit_sphere(ray, light_point.position, light_point.radius, t))
        {
            ReportHit(0.1, 0, attr);
            return;
        }

        ReportHit(t, 0, attr);
    }

    return;
}

[shader("intersection")]
void dummy()
{
    return;
}
