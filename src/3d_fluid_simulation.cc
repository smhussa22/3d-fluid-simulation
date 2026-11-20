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

    // view matrix; transforms world coordinates into camera-relative coordinates
    // set up a fixed camera...
    const ::DirectX::XMMATRIX view_matrix 
    { 
        ::DirectX::XMMatrixLookAtLH(::DirectX::XMVectorSet(5.0f, 5.0f, 5.0f, 1.0f),  // ...at this coordinates (x,y,z) (theres a 4th w in the code but ignore it)
        ::DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),                              // ...looking at these coordinates
        ::DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))                              // ...where these coordinates are considered vertically upwards
    };

    // projection matrix; transforms camera-space coordinates into clip space AND applies perspective so distant objects appear smaller
    const ::DirectX::XMMATRIX projection_matrix 
    { 
        // 60 deg vertical fov, how wide the cameras viewing angle is vertically
        // 1280/720 aspect ratio; just the window size and how wide the viewing area should be relative to the height
        // 0.1 units near clipping plane (the boundary that determines which parts of 3d geometry are visible and which parts should be discarded)
            // iow geometry closer than 0.1 units gets "clipped" (discarded since it falls outside the camera's viewing boundaries)
        // 100 units far clipping distance, geometry farther than 100 units from teh camera gets clipped
        // CLIPPING PLANE DIAGRAM: https://www.researchgate.net/figure/Principles-and-parameters-of-perspective-projection-The-red-and-yellow-balls-represent_fig4_395502841
        ::DirectX::XMMatrixPerspectiveFovLH(::DirectX::XMConvertToRadians(60.0f), 1280.0f / 720.0f, 0.1f, 100.0f) 
    };
    
    // view projection; we take the WORLD - TO - CAMERA transformation from the view matrix,
    // and the CAMERA SPACE TO CLIP SPACE perspective projection from the projection matrix, and combine them
    // to transform world coordinates into clip space.
    ::DirectX::XMFLOAT4X4 view_projection_matrix {};
    ::DirectX::XMStoreFloat4x4(&view_projection_matrix, view_matrix * projection_matrix);
    ConstantBuffer camera { graphics, sizeof(::DirectX::XMFLOAT4X4) };
    camera.write(graphics, &view_projection_matrix);

    // tl;dr The view matrix uses your camera's position, target, and up direction 
    // to calculate a coordinate transformation that makes the rest of the scene appear from the camera's chosen perspective.

    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        graphics.beginFrame(0.0f, 0.0f, 0.0f, 1.0f);

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
