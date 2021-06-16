#version 410
precision mediump float;

in lowp vec3 f_color;
in vec2 texCoord;
out vec3 out_color;

uniform sampler2D tex;
uniform float time;

void main()
{
    vec2 v_centered_texture_coords = vec2(texCoord.x - 0.5, texCoord.y - 0.5);
    vec2 v_rotated_texCoords = vec2(cos(-time/2) * v_centered_texture_coords.x - sin(-time/2) * v_centered_texture_coords.y,
                                    sin(-time/2) * v_centered_texture_coords.x + cos(-time/2) * v_centered_texture_coords.y);
    vec2 v_translated_rotated_texCoords = vec2(v_rotated_texCoords.x + 0.5, v_rotated_texCoords.y + 0.5);

    vec4 tex_color = texture(tex, v_translated_rotated_texCoords);
    if (tex_color.x == 0 && tex_color.y == 0 && tex_color.z == 0) {
        out_color = f_color;
    } else {
        out_color = vec3(texture(tex, v_translated_rotated_texCoords) * vec4(f_color, 1.0));
    }
}