#ifndef CAMERA_HH
#define CAMERA_HH

#include <DirectXMath.h>

#include "Window.hh"

#include <cmath>

// yaw, pitch, and roll: https://songho.ca/opengl/gl_camera.html?utm_source=chatgpt.com#:~:text=z%3B%0A%0A%20%20%20%20return%20matrix%3B%0A%7D-,Camera%20Rotation%20(Pitch%2C%20Yaw%2C%20Roll),-Camera%20Rotations%3B%20Pitch
// spherical to cartesian coordinate converison: https://www.getzenquery.com/tools/spherical-coordinate-converter
class Camera
{
public:

    Camera() = default;
    ~Camera() = default;
    
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    // reads the mouse from the window and updates yaw, pitch and distance (call once per frame)
    void update(Window& window);

    // builds view * projection for the current camera, ready to upload to the constant buffer
    // takes camera position and builds the matrices ready to render hte scene
    ::DirectX::XMFLOAT4X4 viewProjection(float aspect) const;

private:

    ::DirectX::XMFLOAT3 m_target { 0, 0, 0 }; // the point in 3D space the camera looks at
    float m_yaw { 0.0f };                     // horizontal rotation angle around the target
    float m_pitch { 0.4f };                   // vertical rotation angle around the target
    float m_distance { 5.0f };                // distance between the camera and the target

    // need these two to calculate how far the mouse has moved while dragging the camera
    double m_last_x { 0.0 };                  // stores mouse cursor's x from the last frame, for measuring drag
    double m_last_y { 0.0 };                  // stores mouse cursor's cursor y from the last frame

    // the camera's position in the world, worked out from yaw, pitch, distance and the target
    ::DirectX::XMVECTOR _eye() const
    {

        float x { m_distance * std::cos(m_pitch) * std::sin(m_yaw) };
        float y { m_distance * std::sin(m_pitch) };
        float z { m_distance * std::cos(m_pitch) * std::cos(m_yaw) };

        ::DirectX::XMVECTOR eye { ::DirectX::XMVectorSet(m_target.x + x, m_target.y + y, m_target.z + z, 1.0f) };
        return eye;

    }

};

#endif // CAMERA_HH