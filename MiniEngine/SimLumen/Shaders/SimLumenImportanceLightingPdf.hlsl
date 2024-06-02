#define LIGHT_PDF_GROUP_SIZE 8

groupshared float group_light_pdf[LIGHT_PDF_GROUP_SIZE * LIGHT_PDF_GROUP_SIZE * 4];

RWTexture2D<float> RWLightPdf;
Texture2D<float> probe_depth: register(t0);