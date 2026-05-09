module materials;

import "./utils";
import "./pdf";
import "./attributs";

public struct BRDF
{
    public Color attenuation;
    public float roughness;

    float pad1 = 0, pad2 = 0, pad3 = 0;

    typealias Pdf = MixPdf<ReflectPdf, CosinePdf>;

    public bool scatter(const RayDesc ray_in, const HitAttribut hit_attr, inout uint seed, out ScatterAttribut scatter_attr)
    {
        scatter_attr.attenuation = attenuation;
        scatter_attr.pdf_val = random_dir(ray_in, hit_attr, seed, scatter_attr.out_dir);
        scatter_attr.skip_pdf = false;
        return true;
    }

    public Pdf get_pdf(NVec3 dir_in, NVec3 normal)
    {
        static_assert(sizeof(This) == 32, "Size does not match !");

        let first_pdf = ReflectPdf(dir_in, normal);
        let seconf_pdf = CosinePdf(normal);
        return MixPdf<ReflectPdf, CosinePdf>(first_pdf, seconf_pdf, roughness);
    }

    public float random_dir(const RayDesc ray_in, const HitAttribut hit_attr, inout uint seed, out NVec3 dir_out)
    {

        let pdf = get_pdf(NVec3::from(ray_in.Direction), hit_attr.normal);

        dir_out = pdf.generate(seed);

        return pdf.value(dir_out);
    }

    public float distribution(const in RayDesc ray_in, const in RayDesc ray_out, const HitAttribut hit_attr)
    {
        let pdf = get_pdf(NVec3::from(ray_in.Direction), hit_attr.normal);
        return pdf.value(NVec3::from(ray_out.Direction));
    }
};

public struct Material
{
    // -- Attribut --

    public Color emission;
    public BRDF brdf;

    // -- Methods --

    public Color emited(const RayDesc ray, const HitAttribut hit_attr, Vec2 uv)
    {

        static_assert(sizeof(This) == 48, "Size is not matching !");

        return emission;
    }
}
