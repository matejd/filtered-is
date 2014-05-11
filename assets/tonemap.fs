varying vec2 vuv;
uniform sampler2D fb;
uniform vec2 uvOff;
uniform float gamma;

void main()
{
    vec3 linear = texture2D(fb, vuv*vec2(0.5, 1.0) + uvOff).rgb;
    gl_FragColor = vec4(pow(linear, vec3(1.0 / gamma)), 0.0);
}
