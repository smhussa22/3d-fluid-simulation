#ifndef CAMERA_COMMON_HLSLI
#define CAMERA_COMMON_HLSLI

cbuffer Camera : register(b0)
{
    row_major float4x4 view_projection;
    float4 camera_right;  // xyz = the camera's right direction in the world
    float4 camera_up;     // xyz = the camera's up direction in the world
    float4 camera_toward; // xyz = the direction from the scene toward the camera
    float4 camera_eye;    // xyz = where the camera is in the world
};

#endif
