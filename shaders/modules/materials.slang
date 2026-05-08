module materials;

import "./utils";
import "./pdf";
import "./attributs";

public struct Brdf
{
    // -- Attribut --

    public Color albedo;
    public Color emission;
    public float roughness;

    float pad1 = 0, pad2 = 0, pad3 = 0;

    // -- Methods --

    public Color emited(const RayDesc ray, const HitAttribut hit_attr, Vec2 uv)
    {
        return emission;
    }

    public bool scatter(const RayDesc ray_in, const HitAttribut hit_attr, inout uint seed, out ScatterAttribut scatter_attr)
    {
        scatter_attr.attenuation = albedo;
        scatter_attr.pdf_val = scattering_pdf(ray_in, hit_attr, seed, scatter_attr.out_dir);
        scatter_attr.skip_pdf = false;
        return true;
    }

    public float scattering_pdf(const RayDesc ray_in, const HitAttribut hit_attr, inout uint seed, out NVec3 dir_out)
    {
        let first_pdf = ReflectPdf(NVec3::from(ray_in.Direction), hit_attr.normal);
        let seconf_pdf = CosinePdf(hit_attr.normal);
        let pdf = MixPdf<ReflectPdf, CosinePdf>(first_pdf, seconf_pdf, roughness);

        dir_out = pdf.generate(seed);

        return pdf.value(dir_out);
    }

    public float scattering_pdf_value(const in RayDesc ray_in, const in RayDesc ray_out, const HitAttribut hit_attr)
    {
        let first_pdf = ReflectPdf(NVec3::from(ray_in.Direction), hit_attr.normal);
        let seconf_pdf = CosinePdf(hit_attr.normal);
        let pdf = MixPdf<ReflectPdf, CosinePdf>(first_pdf, seconf_pdf, roughness);
        let result = pdf.value(NVec3(ray_out.Direction));
        return result;
    }
}
