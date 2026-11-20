#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"
#include "RenderShader.hh"
#include "ConstantBuffer.hh"

#include <exception>
#include <print>
#include <vector>
#include <cstdint>


#include <DirectXMath.h>

int main()
try
{

    // open a 1280x720 window and bring up Direct3D 11 on it
    Window window { 1280, 720, "3D Fluid Simulation" };
    Graphics graphics { window.hwnd(), window.width(), window.height() };
    std::println("Direct3D 11 ready, feature level {:#x}", static_cast<unsigned>(graphics.featureLevel()));

    std::vector<::DirectX::XMFLOAT4> points {};

    // yikes
    for (int x {-10}; x < 10; ++x)
    {
        for (int y {-10}; y < 10; ++y)
        {
            for (int z {-10}; z < 10; ++z)
            {
                points.emplace_back(x * 0.1f, y * 0.1f, z * 0.1f, 1.0f);
            }
        }
    }

    Buffer particles { graphics, sizeof(::DirectX::XMFLOAT4), 8000, points.data() };
    RenderShader render_shader { graphics, L"shaders/particles.hlsl" };

    // set up a fixed camera at
    const ::DirectX::XMMATRIX view 
    { 
        ::DirectX::XMMatrixLookAtLH(::DirectX::XMVectorSet(3.0f, 2.0f, -5.0f, 1.0f), // where the camera is, the eye
        ::DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),                              // the point it looks at
        ::DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))                              // which way is up
    };
    const ::DirectX::XMMATRIX projection { ::DirectX::XMMatrixPerspectiveFovLH(::DirectX::XMConvertToRadians(60.0f), 1280.0f / 720.0f, 0.1f, 100.0f) };
    ::DirectX::XMFLOAT4X4 view_projection {};
    ::DirectX::XMStoreFloat4x4(&view_projection, view * projection);
    ConstantBuffer camera { graphics, sizeof(::DirectX::XMFLOAT4X4) };
    camera.write(graphics, &view_projection);

    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        graphics.beginFrame(0.1f, 0.1f, 0.15f, 1.0f);

        // draw every particle as a point, reading positions from the buffer
        ::ID3D11ShaderResourceView* srv { particles.srv() };
        render_shader.use(graphics);
        camera.bindVertex(graphics, 0u);
        graphics.context()->VSSetShaderResources(0u, 1u, &srv);
        graphics.context()->IASetInputLayout(nullptr);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        graphics.context()->Draw(static_cast<::UINT>(points.size()), 0u);
        
        graphics.present(false);
        window.pollEvents();

    }

} 
catch (const std::exception& e)
{

    // print the failure instead of crashing silently
    std::println("error: {}", e.what());
    return 1;

}
