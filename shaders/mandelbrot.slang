// image_display.slang


[[vk::binding(0, 0)]]
RWTexture3D<float4> result;

struct UniformData {
    uint2 resolution;
    float time;
};

[[vk::push_constant]]
ConstantBuffer<UniformData> unis;


[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    printf("runed\n");

    uint3 texel_coord = threadId.xyz;
    uint3 size;
    uint levels;
    result.GetDimensions(1, size.x, size.y, size.z, levels);

    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float4 color = float4(float3(0), 1);

        if (threadId.x != 0 && threadId.y != 0) {
            color.x = float(texel_coord.x) / (size.x);
            color.y = float(texel_coord.y) / (size.y);
        }

        result.Store(texel_coord, color);
    }
}
