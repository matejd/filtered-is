varying vec3 vnormal;
uniform sampler2D env;
uniform float gamma;

void main()
{
    vec3 dir = normalize(vnormal);
    vec3 linear = samplePanorama(env, dir, 0.5);
    vec3 color = linear / (linear + 1.0);
    gl_FragColor.rgb = pow(color, vec3(1.0/gamma));
}
