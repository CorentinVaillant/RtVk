implementing rt_utils;

import "../utils";

public interface IPdf
{
    /// @brief Return the probability value of the function at `direction`
    public float value(Vec3 direction);

    /// @brief Generate a random value follwing the distribution
    public Vec3 generate(inout uint seed);
};

public struct SpherePdf : IPdf
{

    public float value(Vec3 direction)
    {
        return 1.f/M_FOUR_PI_F;
    }

    public Vec3 generate(inout uint seed)
    {
        return random_unit(seed);
    }
};

public struct CosinePdf : IPdf
{
    ONB base;

    public __init(Vec3 normal)
    {
        base = ONB(normal);
    }

    public float value(Vec3 direction)
    {
        float cos_theta = dot(normalize(direction), base.w);
        return max(0.f, cos_theta / M_PI_F);
    }

    public Vec3 generate(inout uint seed)
    {
        return base.transform(random_cosine_hemisphere(seed));
    }
};

public struct ReflectPdf : IPdf
{
    Vec3 reflect_dir;

    __init(Vec3 dir_in, Vec3 normal)
    {
        reflect_dir = normalize(reflect(dir_in, normal));
    }

    public float value(Vec3 direction)
    {
        static constexpr float EPSILONE = 1e-8;
        if (length_sq(direction - reflect_dir) < EPSILONE)
            return 1.0f;
        else
            return 0.f;
    }

    public Vec3 generate(inout uint seed)
    {
        return reflect_dir;
    }
};

public struct MixPdf<FirstPdf, SecondPdf> : IPdf
    where FirstPdf : IPdf
    where SecondPdf : IPdf
{
    FirstPdf first_pdf;
    SecondPdf second_pdf;
    float mix;

    __init(FirstPdf first_pdf, SecondPdf second_pdf, float mix)
    {
        this.first_pdf = first_pdf;
        this.second_pdf = second_pdf;
        this.mix = clamp(mix, 0, 1);
    }

    public float value(Vec3 direction)
    {
        return (1.f - mix) * first_pdf.value(direction) + mix * second_pdf.value(direction);
    }

    public Vec3 generate(inout uint seed)
    {
        let u = random(seed, 0.f, 1.f);
        if (mix <= u)
            return first_pdf.generate(seed);
        else
            return second_pdf.generate(seed);
    }
}
