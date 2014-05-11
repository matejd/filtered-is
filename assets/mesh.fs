varying vec3 vnormal;
varying vec3 vview;

uniform sampler2D env;
uniform float gamma;
uniform vec3 whs[42];

uniform float roughness;
uniform float lod;
uniform vec3 kd;
uniform vec3 F0;

/*const float roughness = 0.05;*/
/*const float lod = 3.5;*/
/*const vec3 kd = vec3(0.0, 0.0, 0.0);*/
/*const vec3 F0 = vec3(0.5, 0.5, 0.5);*/

/*uniform int numSamples;*/
const int NumSamples = 30;
const float PI = 3.14159265;

vec3 lookupLi(vec3 wj, vec3 wn, vec3 wh)
{
    return samplePanorama(env, wj, lod);
}

vec3 evaluateF(vec3 F0, float cosThetad)
{
    return F0 + (1.0-F0) * pow(1.0-cosThetad, 5.0);
}

float evaluateG(vec3 wj, vec3 wo, vec3 wh, vec3 wn, float roughness)
{
    /*return dot(wn, wj) * dot(wn, wo);*/
    /*return 1.0;*/

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

    // Fresnel only (assuming perfect mirror)
    /*vec3 wr = 2.0*dot(wo,wn)*wn - wo;*/
    /*vec3 Fn = evaluateF(F0, dot(wo, wn));*/
    /*vec3 Li = samplePanorama(env, wr, 1.4);*/
    /*vec3 spec = Fn*Li;*/
    vec3 lambert = (kd/PI) * samplePanorama(env, wn, 5.0);

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

    vec3 linear = lambert + spec;

    // Reinhard (global)
    vec3 color = linear / (linear + 1.0);
    vec3 unused = wn + wo + kd + F0*roughness + whs[0];
    gl_FragColor.rgb = unused*0.000001 + pow(color, vec3(1.0/gamma));
    /*gl_FragColor.rgb = pow(color, vec3(1.0/gamma));*/
}
