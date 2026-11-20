#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"
#include "RenderShader.hh"
#include "ConstantBuffer.hh"
#include "Camera.hh"
#include "Gui.hh"

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

    // orbit camera, plus the constant buffer that carries its matrix to the shader
    Camera camera {};
    ConstantBuffer camera_buffer { graphics, sizeof(::DirectX::XMFLOAT4X4) };
    Gui gui { window, graphics };

    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        // move the camera from mouse input, then upload its new matrix
        camera.update(window);
        const ::DirectX::XMFLOAT4X4 view_projection { camera.viewProjection(static_cast<float>(window.width()) / static_cast<float>(window.height())) };
        camera_buffer.write(graphics, &view_projection);

        graphics.beginFrame(0.0f, 0.0f, 0.0f, 1.0f);
        gui.beginFrame();
        gui.drawFps();

        // draw every particle as a point, reading positions from the buffer
        ::ID3D11ShaderResourceView* srv { particles.srv() };
        render_shader.use(graphics);
        camera_buffer.bindVertex(graphics, 0u);
        graphics.context()->VSSetShaderResources(0u, 1u, &srv);
        graphics.context()->IASetInputLayout(nullptr);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        graphics.context()->Draw(static_cast<::UINT>(points.size()), 0u);

        // draw the on-screen overlay on top of the scene
        gui.render();
        
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