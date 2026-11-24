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

void Camera::set_view(const ::DirectX::XMFLOAT3& target, float distance, float pitch)
{

    m_target = target;
    m_distance = distance;
    m_pitch = pitch;

}

Camera::ShaderData Camera::shader_data(float aspect) const
{

    // forward points from the camera to its target; in a left-handed system right = up x forward, and the camera's own up = forward x right
    const ::DirectX::XMVECTOR target { ::DirectX::XMVectorSet(m_target.x, m_target.y, m_target.z, 1.0f) };
    const ::DirectX::XMVECTOR forward { ::DirectX::XMVector3Normalize(::DirectX::XMVectorSubtract(target, _eye())) };
    const ::DirectX::XMVECTOR right { ::DirectX::XMVector3Normalize(::DirectX::XMVector3Cross(::DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward)) };
    const ::DirectX::XMVECTOR up { ::DirectX::XMVector3Cross(forward, right) };

    ShaderData data {};
    data.view_projection = viewProjection(aspect);
    ::DirectX::XMStoreFloat4(&data.right, right);
    ::DirectX::XMStoreFloat4(&data.up, up);
    ::DirectX::XMStoreFloat4(&data.toward, ::DirectX::XMVectorNegate(forward));
    ::DirectX::XMStoreFloat4(&data.eye, _eye());
    return data;

}

::DirectX::XMFLOAT4X4 Camera::viewProjection(float aspect) const
{

    ::DirectX::XMVECTOR target { ::DirectX::XMVectorSet(m_target.x, m_target.y, m_target.z, 1.0f) };
    ::DirectX::XMVECTOR up { ::DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
    ::DirectX::XMMATRIX view { ::DirectX::XMMatrixLookAtLH(_eye(), target, up) };
    ::DirectX::XMMATRIX projection { ::DirectX::XMMatrixPerspectiveFovLH(::DirectX::XMConvertToRadians(fov_degrees), aspect, 0.1f, 1500.0f ) };

    ::DirectX::XMFLOAT4X4 result {};
    ::DirectX::XMStoreFloat4x4(&result, view * projection);
    return result;


}