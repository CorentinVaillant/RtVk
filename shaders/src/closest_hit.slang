import "../modules/bindings";
import "../modules/utils";
import "../modules/rt_utils";
import "../modules/scene_data";
import "../modules/materials";
import "../modules/payloads";
import "../modules/attributs";
import "../modules/lights";

[shader("closesthit")]
void meshHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr_in)
{
    uint tri_index = PrimitiveIndex();
    RendererInstance instance = instances[InstanceID()];
    uint3 idx = indexBuffer[tri_index + instance.indexOffset];

    Vertex v0 = vertexBuffer[idx.x];
    Vertex v1 = vertexBuffer[idx.y];
    Vertex v2 = vertexBuffer[idx.z];

    Vec3 b = Vec3(1.0 - attr_in.barycentrics.x - attr_in.barycentrics.y,
                  attr_in.barycentrics.x,
                  attr_in.barycentrics.y);

    // Simple interpolation
    HitAttribut attr;
    attr.time = RayTCurrent();
    attr.type = PrimitiveType::Mesh;

    Vec3 interp_normal = b.x * v0.normal + b.y * v1.normal + b.z * v2.normal;
    attr.u = b.x * v0.u + b.y * v1.u + b.z * v2.u;
    attr.v = b.x * v0.v + b.y * v1.v + b.z * v2.v;

    attr.set_face_normal(NVec3::from(WorldRayDirection()), NVec3::from(interp_normal));
    attr.material_index = instance.materialId;

    RayDesc ray;
    ray.Direction = WorldRayDirection();
    ray.Origin = WorldRayOrigin();
    ray.TMax = 10000;
    ray.TMin = RayTMin();

    shade(ray, payload, attr);
}

[shader("closesthit")]
void meshPdfHit(inout PdfPayload payload, in BuiltInTriangleIntersectionAttributes attr_in)
{
    payload.hit = false;

    let ray_dir = WorldRayDirection();
    let ray_org = WorldRayOrigin();

    // get vertices pos :
    uint tri_index = PrimitiveIndex();
    RendererInstance instance = instances[InstanceID()];
    uint3 idx = indexBuffer[tri_index + instance.indexOffset];

    // Test if it is a light source
    Material Material = materialBuffer[instance.materialId];
    if (payload.min_light_val > luminance(Material.emission))
        return;

    Vertex v0 = vertexBuffer[idx.x];
    Vertex v1 = vertexBuffer[idx.y];
    Vertex v2 = vertexBuffer[idx.z];

    Vec3 b = Vec3(1.0 - attr_in.barycentrics.x - attr_in.barycentrics.y,
                  attr_in.barycentrics.x,
                  attr_in.barycentrics.y);

    // compute the normal
    Vec3 normal = b.x * v0.normal + b.y * v1.normal + b.z * v2.normal;
    // compute the area of the triangle
    Vec3 e1 = v1.position - v0.position;
    Vec3 e2 = v2.position - v0.position;

    let area = 0.5 * length(cross(e1, e2));

    // compute the pdf val
    let dist_sq = sq(RayTCurrent() * length(ray_dir));
    let cos_val = abs(dot(normalize(ray_dir), normalize(normal)));

    if (cos_val < 1e-8f)
    {
        payload.pdf_val = 0.f;
        return;
    }

    payload.hit_point = b.x * v0.position + b.y * v1.position + b.z * v2.position;

    payload.pdf_val = dist_sq / (cos_val * area);

    payload.hit = true;
}

[shader("closesthit")]
void lightPdf(inout PdfPayload payload, in LightAttribut attr_in)
{
    let light = punctualLightBuffer[attr_in.index];

    if (payload.min_light_val > luminance(light.emission))
    {
        payload.hit = false;
        return;
    }
    payload.hit = true;
    payload.hit_point = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    RayDesc ray;
    ray.Direction = WorldRayDirection();
    ray.Origin = WorldRayOrigin();
    ray.TMax = 10000.;
    ray.TMin = 0.0001;
    payload.pdf_val = light.pdf(ray);
}

[shader("closesthit")]
void lightHit(inout RayPayload payload, in LightAttribut attr_in)
{
    payload.color = punctualLightBuffer[attr_in.index].emission;
}
