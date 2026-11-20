#ifndef RENDER_SHADER_HH
#define RENDER_SHADER_HH

#include "Graphics.hh"

#include <d3d11.h>
#include <wrl/client.h>

#include <string>

// a vertex shader and pixel shader pair compiled from one hlsl file: the vertex shader places each point, the pixel shader colors it
class RenderShader
{
public:

    // compiles vs_main and ps_main from the file and creates both shaders on the gpu through the Graphics device
    RenderShader(const Graphics& graphics, const std::wstring& file_path);
    ~RenderShader() = default;

    RenderShader (const RenderShader&) = delete;
    RenderShader& operator=(const RenderShader&) = delete;

    RenderShader(RenderShader&&) = delete;
    RenderShader& operator=(RenderShader&&) = delete;

    // makes this pair the active vertex and pixel shaders for the next draw
    void use(const Graphics& graphics) const;

private:
    ::Microsoft::WRL::ComPtr<::ID3D11VertexShader> m_vertex_shader {}; // runs once per vertex (particle) and decides where it lands on screen
    ::Microsoft::WRL::ComPtr<::ID3D11PixelShader> m_pixel_shader {}; // runs once per covered pixel and decides its color
    
};

#endif // RENDER_SHADER_HH