module pdf;

import "./utils";

public interface IPdf
{
    /// @brief Return the probability value of the function at `direction`
    public float value(NVec3 direction);

    /// @brief Generate a random value follwing the distribution
    public NVec3 generate(inout uint seed);
};

public struct SpherePdf : IPdf
{

    public float value(NVec3 _)
    {
        return 1.f / M_FOUR_PI_F;
    }

    public NVec3 generate(inout uint seed)
    {
        return random_unit(seed);
    }
};

public struct CosinePdf : IPdf
{
    ONB base;

    public __init(NVec3 normal)
    {
        base = ONB(normal);
    }

    public float value(NVec3 direction)
    {
        float cos_theta = dot(direction.vec, base.w.vec);
        return max(0.f, cos_theta / M_PI_F);
    }

    public NVec3 generate(inout uint seed)
    {
        return NVec3(base.transform(random_cosine_hemisphere(seed)));
    }
};

public struct ReflectPdf : IPdf
{
    NVec3 reflect_dir;

    public __init(NVec3 dir_in, NVec3 normal)
    {
        reflect_dir = NVec3(reflect(dir_in.vec, normal.vec));
    }

    public __init(Vec3 dir_in, Vec3 normal)
    {
        reflect_dir = NVec3::from(reflect(dir_in, normal));
    }

    /// @param 
    /// - direction a normal vec
    public float value(NVec3 direction)
    {
        static constexpr float EPSILONE = 1e-2;
        if (abs(dot(direction.vec, reflect_dir.vec)) > 1 - EPSILONE)
            return 1.0f;
        else
            return 0.f;
    }

    public NVec3 generate(inout uint seed)
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

    public __init(FirstPdf first_pdf, SecondPdf second_pdf, float mix)
    {
        this.first_pdf = first_pdf;
        this.second_pdf = second_pdf;
        this.mix = clamp(mix, 0, 1);
    }

    public float value(NVec3 direction)
    {
        return (1.f - mix) * first_pdf.value(direction) + mix * second_pdf.value(direction);
    }

    public NVec3 generate(inout uint seed)
    {
        let u = random(seed, 0.f, 1.f);
        if (u < mix)
            return first_pdf.generate(seed);
        else
            return second_pdf.generate(seed);
    }
}
