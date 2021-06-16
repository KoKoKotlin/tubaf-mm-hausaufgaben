#version 410
precision mediump float;

in lowp vec3 f_color;
in vec2 texCoord;
out vec3 out_color;

uniform sampler2D tex;

void main()
{
    vec4 tex_color = texture(tex, texCoord);
    if (tex_color.x == 0 && tex_color.y == 0 && tex_color.z == 0) {
        out_color = f_color;
    } else {
        out_color = vec3(texture(tex, texCoord) * vec4(f_color, 1.0));
    }
}