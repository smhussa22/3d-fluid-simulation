#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"

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
    for (int x {0}; x <= 20; ++x)
    {
        for (int y {0}; y <= 20; ++y)
        {
            for (int z {0}; z <= 20; ++z)
            {
                points.emplace_back(x, y, z, 1.0f);
            }
        }
    }

    Buffer buffer { graphics, sizeof(::DirectX::XMFLOAT4), 8000, points.data() };


    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        graphics.beginFrame(0.1f, 0.1f, 0.15f, 1.0f);
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
