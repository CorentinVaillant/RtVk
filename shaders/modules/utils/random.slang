implementing utils;

__include "types";

// -- this part is get from : https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/tree/master/ray_tracing_jitter_cam

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
public uint tea(uint val0, uint val1)
{
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for (uint n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint prev)
{
    return (float(lcg(prev)) / float(0x01000000));
}

// --

public float random(inout uint seed, float min, float max)
{
    return min + (max - min) * rnd(seed);
}

public Vec2 random2(inout uint seed, float min, float max)
{
    return Vec2(random(seed, min, max), random(seed, min, max));
}

public Vec3 random3(inout uint seed, float min, float max)
{
    return Vec3(random(seed, min, max), random(seed, min, max), random(seed, min, max));
}

public Vec2 random_in_unit_disk(inout uint seed)
{
    let theta = random(seed, 0, M_TWO_PI_F);
    let ro = sqrt(random(seed, 0, 1));
    return ro * Vec2(cos(theta), sin(theta));
}

public Vec3 random_unit(inout uint seed)
{
    // using polar cordinate
    let phi = random(seed, 0, M_TWO_PI_F);
    let cos_theta = random(seed, -1.0, 1.0);
    let sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    let x = cos(phi) * sin_theta;
    let y = sin(phi) * sin_theta;
    let z = cos_theta;

    return Vec3(x, y, z);
}

public Vec3 random_cosine_hemisphere(inout uint seed)
{
    let v = random2(seed, 0, 1);

    let phi = M_TWO_PI_F * v.x;

    return Vec3(
        cos(phi) * sqrt(v.y),
        sin(phi) * sqrt(v.y),
        sqrt(1 - v.y));
}
