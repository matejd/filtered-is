attribute vec3 position;
attribute vec3 normal;

uniform mat4 mvp;
varying vec3 vnormal;

void main()
{
    vnormal = position;
    gl_Position = mvp * vec4(position, 1.0);
}
