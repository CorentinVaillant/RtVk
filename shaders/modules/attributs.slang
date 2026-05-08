module attributs;

import utils;
import scene_data;

// -- Attributs

public struct HitAttribut
{
    public static constexpr uint NO_MATERIAL_INDEX = ~0u;

    public float time;
    public NVec3 normal;
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
    public void set_face_normal(NVec3 ray_dir, NVec3 out_normal)
    {
        front_face = dot(ray_dir.vec, out_normal.vec) < 0;
        normal = front_face ? out_normal : out_normal.neg();
    
    }
}

public struct ScatterAttribut
{
    public Color attenuation = Color(0);
    public float pdf_val;
    public bool skip_pdf;
    public RayDesc skip_pdf_ray;
    public NVec3 out_dir;

    public RayDesc create_scatter_ray(Vec3 position) {
        RayDesc scatter_ray;
        scatter_ray.Origin = position;
        scatter_ray.Direction = out_dir.vec;
        scatter_ray.TMin = 0.0001f;
        scatter_ray.TMax = 10000.f;

        return scatter_ray;
    }
}

public struct LightAttribut {
    public uint index;
    public RayDesc ray;
}