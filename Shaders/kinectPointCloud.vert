#version 430 core

layout(location = 0) in vec2 tpos;

// depth camera texture 
uniform sampler2D depthTex;

// fovs for depth camera 
uniform float tan_half_depth_hfov;
uniform float tan_half_depth_vfov;

// color camera texture 
uniform sampler2D colorTex;

// fovs for color camera 
uniform float tan_half_color_hfov;
uniform float tan_half_color_vfov;

// proportional offset to account for spacing between cameras
uniform float offsX = 0.0;
uniform float offsY = 0.0;

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

out vec4 vcolor;

void main()
{
    // depth is normalized into the range from the camera to the far plane 
    // Note: 65.535m = (2^16 - 1)mm / 1000(mm/m)
    float depth = 65.535 * texture(depthTex, vec2(tpos.x, 1 - tpos.y)).r / far_plane;
    
    // align 
    vec2 cpos = vec2(0, 0);
    cpos.x = 0.5 + (tpos.x - 0.5) * tan_half_depth_hfov / tan_half_color_hfov + offsX / (2 * depth * tan_half_color_hfov);
    cpos.y = 0.5 + (tpos.y - 0.5) * tan_half_depth_vfov / tan_half_color_vfov + offsY / (2 * depth * tan_half_color_vfov);
    //cpos.x = 0.5 + 0.5 * ((2 * tpos.x - 1) * tan_half_depth_hfov + alignOffsX) / tan_half_color_hfov; 
    //cpos.y = 0.5 + 0.5 * ((2 * tpos.y - 1) * tan_half_depth_vfov + alignOffsY) / tan_half_color_vfov; 
    
    // TODO: add bounds check?? texture wrap probably works for now
    vcolor = texture(colorTex, vec2(cpos.x, 1 - cpos.y));

    vec3 pos = depth * mix(mix(f00, f10, tpos.x), mix(f01, f11, tpos.x), tpos.y);
    //vec4 pos_cs = view * model * vec4(pos, 1);
    //gl_PointSize = clamp(10 - 0.1f * length(pos_cs.xyz), 1, 10); 
    gl_Position = projection * view * model * vec4(pos, 1);
}
