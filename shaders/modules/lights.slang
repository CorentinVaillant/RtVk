module lights;

import utils;
import pdf;
import scene_data;
import geometry;

public float luminance(Color c)
{
    // from https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
    let luminance_factors = Vec3(0.2126, 0.7152, 0.0722);
    return dot(c.rgb, luminance_factors);
}

public struct LightPoint
{
    public Vec4 emission;
    public Vec3 position;
    public float radius;

    void assertions()
    {
        static_assert(sizeof(This) == 32, "Size != 32");
    }

    public NVec3 generate_random_dir(inout uint seed, const in Vec3 origine, out float t)
    {
        let point = SpherePdf().generate(seed).vec * radius + position;
        t = length(point - origine);
        return NVec3((point - origine) / t); // normalize
    }

    public float pdf(RayDesc ray)
    {
        if (!hit_sphere(ray, this.position, this.radius))
            return 0.;

        float dist_sq = length_sq(position - ray.Origin);
        float cos_theta_max = sqrt(1 - radius * radius / dist_sq);
        float solid_angle = 2 * M_PI_F * (1 - cos_theta_max);

        return 1.0 / solid_angle;
    }
};

public struct LightMesh
{
    public Vec4 emission;
    public float area;
    public uint start_index;
    public uint end_index;

    void assertions()
    {
        static_assert(sizeof(This) == 24, "Size != 24");
    }

    public NVec3 generate_random_dir(inout uint seed, const in Vec3 origine, out float t, StructuredBuffer<uint3> indexBuffer, StructuredBuffer<Vertex> vertexBuffer)
    {
        // take a random triangle
        uint base_index = (start_index + random(seed, 0, end_index - start_index)) / 3;
        uint3 idx = indexBuffer[base_index];

        let p0 = vertexBuffer[idx.x].position;
        let p1 = vertexBuffer[idx.y].position;
        let p2 = vertexBuffer[idx.z].position;

        // sample a random point in it

        float u = random(seed, 0, 1);
        float v = random(seed, 0, 1);

        float su = sqrt(u);

        float b0 = 1.0 - su;
        float b1 = v * su;
        float b2 = 1.0 - b0 - b1;

        let point = b0 * p0 + b1 * p1 + b2 * p2;

        t = length(point - origine);
        return NVec3((point - origine) / t); // normalize
    }
};

public struct LightSun
{
    public Vec4 emission;
    public NVec3 forward_vec;
    public float angle;

    public NVec3 generate_random_dir(inout uint seed)
    {

        let L = -forward_vec.vec;

        float u = random(seed, 0.0, 1.0);
        float v = random(seed, 0.0, 1.0);

        float cos_theta = 1.0 - u * (1.0 - cos(angle));
        float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        float phi = 2.0 * M_PI_F * v;

        Vec3 up = abs(L.z) < 0.999 ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
        Vec3 T = normalize(cross(up, L));
        Vec3 B = cross(L, T);

        return NVec3(cos(phi) * sin_theta * T +
                     sin(phi) * sin_theta * B +
                     cos_theta * L);
    }

    bool hit(NVec3 direction)
    {
        let alignement = dot(-direction.vec, forward_vec.vec);
        return alignement > cos(angle);
    }

    bool hit(RayDesc ray)
    {
        return hit(NVec3::from(ray.Direction));
    }

    public float pdf(NVec3 direction)
    {
        return hit(direction) ? 1.f / (M_TWO_PI_F * (1 - cos(angle))) : 0.f;
    }

    public float pdf(RayDesc ray)
    {
        return pdf(NVec3::from(ray.Direction));
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
    public float radius_angle_area;

    public void debug_info()
    {
        if (type == LightType::Point)
        {
            printf("Emission : [%f, %f, %f, %f]\nPosition : [%f, %f, %f]\nRadius : %f\n", emission.x, emission.y, emission.z, emission.w, position_forward_vec.x, position_forward_vec.y, position_forward_vec.z, radius_angle_area);
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
        return LightPoint(emission, position_forward_vec, radius_angle_area);
    }

    public LightMesh get_mesh_light()
    {
        return LightMesh(emission, radius_angle_area, start_index, end_index);
    }

    public LightSun get_sun_light()
    {
        return LightSun(emission, NVec3::from(position_forward_vec), radius_angle_area);
    }

    public float pdf(RayDesc ray)
    {
        float result;

        switch (type)
        {
        case LightType::Point:
        {
            let pt = get_point_light();
            result = pt.pdf(ray);
            break;
        }
        case LightType::Sun:
        {
            let sun = get_sun_light();
            result = sun.pdf(ray);
            break;
        }
        case LightType::Mesh:
        {

            // The mesh pdf is handled with a traceray in closest_hit
            printf("[Warn ] The mesh pdf should be handled with a traceray in closest_hit\n");
            let mesh = get_mesh_light();
            result = -1;
            break;
        }
        }
        return result;
    }
};
