#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"

#include <exception>
#include <print>
#include <vector>
#include <cstdint>

int main()
try
{

    // open a 1280x720 window and bring up Direct3D 11 on it
    Window window { 1280, 720, "3D Fluid Simulation" };
    Graphics graphics { window.hwnd(), window.width(), window.height() };
    std::println("Direct3D 11 ready, feature level {:#x}", static_cast<unsigned>(graphics.featureLevel()));

    std::vector<uint32_t> data(1000, 0);
    Buffer buffer { graphics, sizeof(uint32_t), 1000, data.data() };
    Shader shader { graphics, L"shaders/test.hlsl" };
    ::ID3D11UnorderedAccessView* uav { buffer.uav() };
    graphics.context()->CSSetUnorderedAccessViews(0u, 1u, &uav, nullptr);

    // run the test shader over the buffer, then read the results back and check them
    shader.use(graphics);
    shader.dispatch(graphics, 4u, 1u, 1u);
    buffer.read(graphics, data.data());
    std::println("data[0] = {}, data[1] = {}, data[999] = {}", data[0], data[1], data[999]);

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
