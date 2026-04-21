module lights;

import utils;
import pdf;
import scene_data;

public struct LightPoint
{
    public Vec4 emission;
    public Vec3 position;
    public float radius;

    void assertions()
    {
        static_assert(sizeof(This) == 32, "Size != 32");
    }

    public Vec3 generate_random_dir(inout uint seed, Vec3 origin)
    {
        let pos : Vec3 = SpherePdf().generate(seed) * radius + position;

        return normalize(pos - origin);
    }
};

public struct LightMesh
{
    public Vec4 emission;
    public uint start_index;
    public uint end_index;

    void assertions()
    {
        static_assert(sizeof(This) == 24, "Size != 24");
    }

    public Vec3 generate_random_dir(inout uint seed, Vec3 origin, StructuredBuffer<uint3> indexBuffer, StructuredBuffer<Vertex> vertexBuffer)
    {
        // take a random triangle
        uint base_index = (start_index + random(seed, 0, end_index - start_index)) / 3;
        uint3 idx = indexBuffer[base_index];

        let p0 = vertexBuffer[idx.x].position;
        let p1 = vertexBuffer[idx.y].position;
        let p2 = vertexBuffer[idx.z].position;

        // sample a random point in it

        let random_barycentric = random2(seed, 0, 1);

        let b = Vec3(1.0 - random_barycentric.x - random_barycentric.y,
                     random_barycentric.x,
                     random_barycentric.y);

        let pos = b.x * p0 + b.y * p1 + b.z * p2;

        // return the direction to this point
        return normalize(origin - pos);
    }
};

public struct LightSun
{
    public Vec4 emission;
    public Vec3 forward_vec;
    public float angle;

    void assertions()
    {
        static_assert(sizeof(This) == 32, "Size != 32");
    }

    public Vec3 generate_random_dir(inout uint seed, Vec3 origin)
    {
        let jitter_vector = random3(seed, -angle / 2.f, angle / 2.);
        return normalize(jitter_vector - forward_vec);
    }
};

public enum LightType : uint32_t
{
    Point = 0u,
    Mesh = 1u,
    Sun = 2u,
};

public struct Light
{
    public LightType type;
    public uint32_t start_index, end_index;
    public uint32_t _pad;
    
    public Vec4 emission;
    
    public Vec3 position_forward_vec;
    public float radius_angle;
    
    public void assertions()
    {
        constexpr size_t size = sizeof(This);
        static_assert(size == 48, "Size != 48");
    }

    // __init(LightPoint p)
    // {

    //     this.type = LightType::Point;
    //     this.emission = p.emission;
    //     this.position_forward_vec = p.position;
    //     this.radius_angle = p.radius;
    // }

    // __init(LightMesh m)
    // {
    //     this.type = LightType::Mesh;
    //     this.emission = m.emission;
    //     this.start_index = m.start_index;
    //     this.end_index = m.end_index;
    // }

    // __init(LightSun s)
    // {
    //     this.type = LightType::Sun;
    //     this.emission = s.emission;
    //     this.position_forward_vec = s.forward_vec;
    // }

    public LightPoint get_point_light()
    {
        return LightPoint(emission, position_forward_vec, radius_angle);
    }

    public LightMesh get_mesh_light()
    {
        return LightMesh(emission, start_index, end_index);
    }

    public LightSun get_sun_light()
    {
        return LightSun(emission, position_forward_vec, radius_angle);
    }
};
