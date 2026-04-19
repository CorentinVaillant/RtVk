module attributs;

import utils;
import scene_data;

// -- Attributs

public struct HitAttribut
{
    public static constexpr uint NO_MATERIAL_INDEX = ~0u;

    public float time;
    public Vec3 normal;
    public float u;
    public float v;
    public PrimitiveType type;
    public bool front_face;
    public uint material_index;

    public property uv : Vec2
    {
        get {
            return Vec2(u, v);
        }
        set(Vec2 uv) {
            u = uv.x;
            v = uv.y;
        }
    }

    public property position : Vec3
    {
        get {
            return WorldRayOrigin() + time * WorldRayDirection();
        }
    }

    [mutating]
    public void set_face_normal(Vec3 ray_dir, Vec3 out_normal)
    {
        front_face = dot(ray_dir, out_normal) < 0;
        normal = front_face ? out_normal : -out_normal;
    }
}

public struct ScatterAttribut
{
    public Color attenuation = Color(0);
    public float pdf_val;
    public bool skip_pdf;
    public RayDesc skip_pdf_ray;
    public Vec3 out_dir;

    public RayDesc create_scatter_ray(Vec3 position) {
        RayDesc scatter_ray;
        scatter_ray.Origin = position;
        scatter_ray.Direction = out_dir;
        scatter_ray.TMin = 0.0001f;
        scatter_ray.TMax = 10000.f;

        return scatter_ray;
    }
}

public struct LightAttribut {
    public Color emision;
}