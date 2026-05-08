module geometry;

import utils;
import attributs;

public bool hit_sphere(RayDesc ray, Vec3 pos, float radius)
{
    // fancy quadratic formula
    Vec3 oc = pos - ray.Origin;
    float a = length_sq(ray.Direction);
    float h = dot(ray.Direction, oc);
    float c = length_sq(oc) - radius * radius;

    float discr = h * h - a * c;
    if (discr < 0.f)
        return false; // No hit

    float delta = sqrt(discr);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!contains_open(ray.TMin, ray.TMax, root))
    {
        root = (h + delta) / a; // testing the conjugate
        if (!contains_open(ray.TMin, ray.TMax, root))
            return false; // roots out of range
    }
    // HIT !
    return true;
}

public bool hit_sphere(RayDesc ray, Vec3 pos, float radius, out float t)
{
    t = ray.TMax;
    // fancy quadratic formula
    Vec3 oc = pos - ray.Origin;
    float a = length_sq(ray.Direction);
    float h = dot(ray.Direction, oc);
    float c = length_sq(oc) - radius * radius;

    float discr = h * h - a * c;
    if (discr < 0.f)
        return false; // No hit

    float delta = sqrt(discr);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!contains_open(ray.TMin, ray.TMax, root))
    {
        root = (h + delta) / a; // testing the conjugate
        if (!contains_open(ray.TMin, ray.TMax, root))
            return false; // roots out of range
    }
    // HIT !
    t = root;
    return true;
}

public bool hit_sphere(RayDesc ray, Vec3 pos, float radius, inout HitAttribut hit_record)
{
    // fancy quadratic formula
    Vec3 oc = pos - ray.Origin;
    float a = length_sq(ray.Direction);
    float h = dot(ray.Direction, oc);
    float c = length_sq(oc) - radius * radius;

    float discr = h * h - a * c;
    if (discr < 0.f)
        return false; // No hit

    float delta = sqrt(discr);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!contains_open(ray.TMin, ray.TMax, root))
    {
        root = (h + delta) / a; // testing the conjugate
        if (!contains_open(ray.TMin, ray.TMax, root))
            return false; // roots out of range
    }
    // HIT !
    hit_record.time = root;
    let hit_p = hit_record.position;
    NVec3 out_normal = NVec3((hit_p - pos) / radius);
    hit_record.set_face_normal(NVec3(ray.Direction), out_normal);
    float theta = acos(-hit_p.y);
    float phi = atan2(-hit_p.z, hit_p.x) + M_PI_F;
    hit_record.u = phi / M_TWO_PI_F;
    hit_record.v = theta / M_PI_F;
    return true;
}
