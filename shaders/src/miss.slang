import "../modules/bindings";
import "../modules/payloads";
import "../modules/rt_utils";
import "../modules/utils";

[shader("miss")]
void generalMiss(inout RayPayload payload)
{
    shade_miss(payload);
}

[shader("miss")]
void pdfMiss(inout PdfPayload payload)
{
    payload.hit = false;
    payload.pdf_val = 0.f;
    return;
}
