attribute vec3 position;
attribute vec3 normal;

uniform mat4 mvp;
uniform vec3 viewOrigin;

varying vec3 vnormal;
varying vec3 vview;

void main()
{
    // There is no model transform!
    vnormal = normal;
    vview = viewOrigin - position;
    gl_Position = mvp * vec4(position, 1.0);
}
