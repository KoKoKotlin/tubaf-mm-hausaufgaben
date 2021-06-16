#version 410
layout(location = 0) in vec4 v_position;
layout(location = 1) in vec3 v_color;
layout(location = 2) in vec2 v_texCoords;


out vec3 f_color;
out vec2 texCoord;
uniform float time;

// source: https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    gl_Position = v_position;
    vec3 hsv = rgb2hsv(v_color);
    hsv.x = (hsv.x + time);
    if (hsv.x > 1.0) hsv.x -= 1.0;
    vec3 rgb = hsv2rgb(hsv);
    f_color = rgb;
    texCoord = v_texCoords;
}