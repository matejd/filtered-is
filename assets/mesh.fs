varying vec3 vnormal;
varying vec3 vview;

uniform sampler2D env;
uniform float gamma;
// Sampling half-vectors wh can be done on the GPU, allowing
// per-pixel variability.
uniform vec3 whs[50];

uniform float roughness;
uniform float lod;
uniform vec3 kd;
uniform vec3 F0;

// WebGL GLSL requires constant loop indices!
const int NumSamples = 50;
const float PI = 3.14159265;

vec3 lookupLi(vec3 wj, vec3 wn, vec3 wh)
{
    // lod should depend on sample probability, but in
    // practice it's very much empirical. Here we're
    // allowing the user to change lod manually, so that
    // aliasing when undersampling is clearly visible.
    return samplePanorama(env, wj, lod);
}

vec3 evaluateF(vec3 F0, float cosThetad)
{
    return F0 + (1.0-F0) * pow(1.0-cosThetad, 5.0);
}

float evaluateG(vec3 wj, vec3 wo, vec3 wh, vec3 wn, float roughness)
{
    // Kelemen-Kalos
    const float eps = 0.01;
    float din = max(dot(wj, wn), eps);
    float don = max(dot(wo, wn), eps);
    float dio = max(dot(wj, wo), eps);
    return 2.0 * din * don / (1.0 + dio);
}

void main()
{
    vec3 wn = normalize(vnormal);
    vec3 wo = normalize(vview);

    // Pick a tangent space
    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    vec3 tangent = normalize(cross(worldUp, wn));
    vec3 bitangent = cross(wn, tangent);

    vec3 spec = vec3(0.0);
    for (int j = 0; j < NumSamples; j++) {
        vec3 wh = whs[j];
        wh = tangent * wh.x + bitangent * wh.y + wn * wh.z; // To world space
        vec3 wj = 2.0*dot(wh, wo)*wh - wo;
        float eps = 0.01;
        float djn = max(dot(wj, wn), eps);
        float don = max(dot(wo, wn), eps);
        float djh = max(dot(wj, wh), eps);
        float dhn = max(dot(wh, wn), eps);
        if (djn > eps) {
            vec3 L = lookupLi(wj, wn, wh);
            vec3 F = evaluateF(F0, djh);
            float G = evaluateG(wj, wo, wh, wn, roughness);
            spec += L * G * djh / (don * dhn) * F;
        }
    }
    spec /= float(NumSamples);

    vec3 lambert = (kd/PI) * samplePanorama(env, wn, 5.0); // Most blurred mip is a good approximation of irradiance
    vec3 linear = lambert + spec;

    // Reinhard (global)
    vec3 color = linear / (linear + 1.0);
    vec3 unused = wn + wo + kd + F0*roughness + whs[0];
    gl_FragColor.rgb = unused*0.000001 + pow(color, vec3(1.0/gamma));
}
