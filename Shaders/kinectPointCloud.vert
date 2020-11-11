#version 430 core

layout(location = 0) in vec2 tpos;

const float allignOffsX = 0.015;
const float allignOffsY = -0.005;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

uniform float tan_half_depth_hfov;
uniform float tan_half_depth_vfov;
uniform float tan_half_color_hfov;
uniform float tan_half_color_vfov;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float far_plane;

// frustum corners
uniform vec3 f00;
uniform vec3 f10;
uniform vec3 f11;
uniform vec3 f01;

out vec4 vcolor;

void main()
{
    vec2 cpos = vec2(0, 0);
    cpos.x = allignOffsX + 0.5 + (tpos.x - 0.5) * tan_half_depth_hfov / tan_half_color_hfov; 
    cpos.y = allignOffsY + 0.5 + (tpos.y - 0.5) * tan_half_depth_vfov / tan_half_color_vfov; 
    //TODO: add bounds check?? texture wrap probably works for now
    vcolor = texture(colorTex, vec2(cpos.x, 1 - cpos.y));
    float depth = 65.535 * texture(depthTex, vec2(tpos.x, 1 - tpos.y)).r / far_plane;
    vec3 pos = depth * mix(mix(f00, f10, tpos.x), mix(f01, f11, tpos.x), tpos.y);
    gl_Position = projection * view * model * vec4(pos, 1);
}
