#version 430 core

#define EPSILON 1e-4

// input vertex data 
layout(location = 0) in ivec2 ipos;

// output to fragment shader to render point cloud 
flat out vec4 color;
// decide if to filter this point 
flat out int reject;

// depth camera texture 
layout(binding = 0) uniform sampler2D depthTex;

// fovs for depth camera 
uniform float tan_half_depth_hfov;
uniform float tan_half_depth_vfov;

// color camera texture 
layout(binding = 1) uniform sampler2D colorTex;

// fovs for color camera 
uniform float tan_half_color_hfov;
uniform float tan_half_color_vfov;

// texture to filter point cloud by 
layout(binding = 2) uniform sampler2D filterTex;

// TODO: find schematics with exact offset and fix as constants
// proportional offset to account for spacing between cameras
uniform float offsX = 0.014;
uniform float offsY = -0.004;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// far plane distance given in meters 
uniform float far_plane;

// far plane frustum corners
uniform vec3 f00;
uniform vec3 f10;
uniform vec3 f11;
uniform vec3 f01;

// number of points 
uniform int points_width;
uniform int points_height;

void main()
{
    vec2 tpos = vec2(float(ipos.x) / float(points_width), float(ipos.y) / float(points_height));
    
    // depth is normalized into the range from the camera to the far plane 
    // Note: 65.535m = (2^16 - 1)mm / 1000(mm/m)
    float depth = 65.535 * texture(depthTex, vec2(tpos.x, 1 - tpos.y)).r / far_plane;
    
    // align 
    // TODO: add bounds check?? texture wrap probably works for now
    // TODO: fix cpos.y negation step (in all related files too)
    vec2 cpos = vec2(0, 0);
    cpos.x = 0.5 + (tpos.x - 0.5) * tan_half_depth_hfov / tan_half_color_hfov + offsX / (2 * depth * tan_half_color_hfov);
    cpos.y = 0.5 + (tpos.y - 0.5) * tan_half_depth_vfov / tan_half_color_vfov + offsY / (2 * depth * tan_half_color_vfov);
    
    vec4 pos = model * vec4(depth * mix(mix(f00, f10, tpos.x), mix(f01, f11, tpos.x), tpos.y), 1);

    // retrieve color for this point
    color = texture(colorTex, vec2(cpos.x, 1 - cpos.y));
    // retrieve filter value 
    reject = int(texture(filterTex, vec2(cpos.x, 1 - cpos.y)).r < EPSILON);

    gl_Position = projection * view * pos;
}
