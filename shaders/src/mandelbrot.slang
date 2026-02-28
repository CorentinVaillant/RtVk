// mandelbrot.slang

import "../modules/utils";

[[vk::binding(0, 0)]]
RWTexture2D<float4> result;

struct UniformData
{
    Vec2 start;
    float zoom;
    uint sample_per_pixel;
    uint power = 2;
    float palette_offset = 0;
};

[[vk::push_constant]]
ConstantBuffer<UniformData> unis;

float smooth_iter(Vec2 z, uint i)
{
    float log_zn = log(dot(z, z)) / 2.0;
    float nu = log(log_zn / log(2.0)) / log(2.0);
    return float(i) + 1 - nu;
}

Vec3 palette(float t, float palette_offset)
{
    return 0.5 + 0.5 * cos(6.2831 * (Vec3(0.0, 0.33, 0.67) + t + palette_offset));
}

Complexe z_n(Complexe z, uint pow, Complexe c)
{
    return pow_u(z, pow) + c;
}

static let MAX_ITER = 1000u;
Vec3 mandelbrot(Complexe c, Complexe start, uint pow, float palette_offset)
{
    Complexe z = start;

    for (uint i = 0; i < MAX_ITER; i++)
    {
        z = z_n(z, pow, c);

        if (z.mod_sq() >= 4.0)
        { // if the length is >= 2.
            float iter = smooth_iter(z.vec, i);
            return palette(iter / float(MAX_ITER), palette_offset);
        }
    }

    return Vec3(0);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{

    uint width, height;
    result.GetDimensions(width, height);

    uint3 texel_coord = threadId.xyz;
    Vec2 uv = (Vec2(texel_coord.xy) / width) -0.5;

    if (texel_coord.x <= width && texel_coord.y <= height)
    {
        Vec3 color = Vec3(0);

        Complexe z = Complexe(uv*unis.zoom);
        color = mandelbrot(z, Complexe(unis.start), unis.power, unis.palette_offset);
        

        result.Store(texel_coord.xy, Vec4(color, 1));
    }
}
