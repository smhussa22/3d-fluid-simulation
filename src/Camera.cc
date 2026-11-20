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