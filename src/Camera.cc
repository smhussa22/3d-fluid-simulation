#include "Camera.hh"
#include <utility>
#include <algorithm>

void Camera::update(Window& window)
{

    // hold current cursor positions
    std::pair<double, double> current_cursor_position { window.cursor() };
    if (window.mouseButton(GLFW_MOUSE_BUTTON_LEFT))
    {

        double delta_x { m_last_x - current_cursor_position.first };
        double delta_y { m_last_y - current_cursor_position.second };

        m_yaw -= delta_x * 0.005f;
        m_pitch += delta_y * 0.005f;

        m_pitch = std::clamp(m_pitch, -1.5f, 1.5f);

    }

    m_last_x = current_cursor_position.first;
    m_last_y = current_cursor_position.second; 

    auto scroll { window.takeScroll() };

    m_distance *= (1.0f - 0.1f * scroll);
    m_distance = std::max(m_distance, 0.5f);

}

void Camera::frame(const ::DirectX::XMFLOAT3& box_min, const ::DirectX::XMFLOAT3& box_max)
{

    // look at the middle of the box
    m_target = { (box_min.x + box_max.x) * 0.5f, (box_min.y + box_max.y) * 0.5f, (box_min.z + box_max.z) * 0.5f };

    // with a 60 degree vertical view, a sphere around the box fits when the camera is twice its radius away; 1.2 adds a margin
    const float radius { 0.5f * std::sqrt((box_max.x - box_min.x) * (box_max.x - box_min.x) + (box_max.y - box_min.y) * (box_max.y - box_min.y) + (box_max.z - box_min.z) * (box_max.z - box_min.z)) };
    m_distance = 2.4f * radius;

}

::DirectX::XMFLOAT4X4 Camera::viewProjection(float aspect) const
{

    ::DirectX::XMVECTOR target { ::DirectX::XMVectorSet(m_target.x, m_target.y, m_target.z, 1.0f) };
    ::DirectX::XMVECTOR up { ::DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
    ::DirectX::XMMATRIX view { ::DirectX::XMMatrixLookAtLH(_eye(), target, up) };
    ::DirectX::XMMATRIX projection { ::DirectX::XMMatrixPerspectiveFovLH(::DirectX::XMConvertToRadians(60.0f), aspect, 0.1f, 100.0f ) };

    ::DirectX::XMFLOAT4X4 result {};
    ::DirectX::XMStoreFloat4x4(&result, view * projection);
    return result;


}