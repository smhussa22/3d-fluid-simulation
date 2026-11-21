#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"
#include "RenderShader.hh"
#include "ConstantBuffer.hh"
#include "Camera.hh"
#include "Gui.hh"
#include "Simulation.hh"

#include <algorithm>
#include <exception>
#include <print>
#include <utility>
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
    for (int x {-5}; x < 5; ++x)
    {
        for (int y {-5}; y < 5; ++y)
        {
            for (int z {-5}; z < 5; ++z)
            {
                points.emplace_back(x * 0.1f, y * 0.1f, z * 0.1f, 1.0f);
            }
        }
    }

    // upload the starting positions, then hand them over to the simulation
    const std::size_t particle_count { points.size() };
    Buffer particles { graphics, sizeof(::DirectX::XMFLOAT4), particle_count, points.data() };
    Simulation simulation { std::move(points) };
    RenderShader render_shader { graphics, L"shaders/particles.hlsl" };

    // orbit camera, plus the constant buffer that carries its matrix to the shader
    Camera camera {};
    ConstantBuffer camera_buffer { graphics, sizeof(::DirectX::XMFLOAT4X4) };
    Gui gui { window, graphics };

    // the physics advances in fixed-size steps, however fast frames are drawn
    constexpr double step_time { 1.0 / 120.0 };
    double previous_time { window.time() };
    double accumulator { 0.0 };
    bool r_was_down { false };

    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        // measure the last frame (capped so a long stall doesn't make the physics race to catch up), then run as many fixed steps as that time allows
        const double now { window.time() };
        accumulator += std::min(now - previous_time, 0.25);
        previous_time = now;

        while (accumulator >= step_time)
        {

            simulation.step(static_cast<float>(step_time));
            accumulator -= step_time;

        }

        // send the updated particle positions to the gpu buffer the renderer reads
        particles.write(graphics, simulation.positions().data());

        // move the camera from mouse input unless the mouse is over the panel (scroll is discarded there so it doesn't build up), then upload its new matrix
        if (!gui.wantsMouse())
        {

            camera.update(window);

        }
        else
        {

            window.takeScroll();

        }


        const ::DirectX::XMFLOAT4X4 view_projection { camera.viewProjection(static_cast<float>(window.width()) / static_cast<float>(window.height())) };
        camera_buffer.write(graphics, &view_projection);

        graphics.beginFrame(0.0f, 0.0f, 0.0f, 1.0f);
        gui.beginFrame();
        gui.drawFps();

        // restart the simulation when Reset is clicked or R is pressed (once per press)
        const bool r_is_down { window.key(GLFW_KEY_R) };

        if (gui.drawControls() || (r_is_down && !r_was_down))
        {

            simulation.reset();

        }

        r_was_down = r_is_down;

        // draw every particle as a point, reading positions from the buffer
        ::ID3D11ShaderResourceView* srv { particles.srv() };
        render_shader.use(graphics);
        camera_buffer.bindVertex(graphics, 0u);
        graphics.context()->VSSetShaderResources(0u, 1u, &srv);
        graphics.context()->IASetInputLayout(nullptr);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        graphics.context()->Draw(static_cast<::UINT>(particle_count), 0u);

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