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

    public Vec3 generate_random_dir(inout uint seed, const in Vec3 origine, out float t)
    {
        let point = SpherePdf().generate(seed) * radius + position;
        t = length(point - origine);
        return (point - origine) / t; // normalize
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

    public Vec3 generate_random_dir(inout uint seed, const in Vec3 origine,out float t,StructuredBuffer<uint3> indexBuffer, StructuredBuffer<Vertex> vertexBuffer)
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

        let point =  b.x * p0 + b.y * p1 + b.z * p2;

        t = length(point - origine);
        return (point - origine) / t; // normalize
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

    public Vec3 generate_random_dir(inout uint seed, const in Vec3 origin)
    {
        let jitter_vector = random3(seed, -angle / 2.f, angle / 2.);
        return origin + (RayTCurrent() + 0.1) * (jitter_vector - forward_vec);
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

    public void debug_info()
    {
        if (type == LightType::Point)
        {
            printf("Emission : [%f, %f, %f, %f]\nPosition : [%f, %f, %f]\nRadius : %f\n", emission.x, emission.y, emission.z, emission.w, position_forward_vec.x, position_forward_vec.y, position_forward_vec.z, radius_angle);
        }
        else if (type == LightType::Mesh)
        {
            printf("Emission : [%f, %f, %f, %f]\nstart_id : %u; end_id : %u\n", emission.x, emission.y, emission.z, emission.w, start_index, end_index);
        }
        else
        {
            printf("Sun\n");
        }
    }

    public void assertions()
    {
        constexpr size_t size = sizeof(This);
        static_assert(size == 48, "Size != 48");
    }

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
