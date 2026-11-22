#ifndef RENDER_TARGET_HH
#define RENDER_TARGET_HH

#include "Graphics.hh"

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

// RAII wrapper around a texture that the GPU can draw into and that shaders can then read from (one stage's output is the next stage's input)
class RenderTarget
{

public:

    // creates a width x height texture in the given format (e.g. DXGI_FORMAT_R32_FLOAT for one float per pixel)
    RenderTarget(const Graphics& graphics, int width, int height, ::DXGI_FORMAT format);

    ~RenderTarget() = default;

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) = delete;
    RenderTarget& operator=(RenderTarget&&) = delete;

    // fills the whole texture with one value (a one-channel format only uses r)
    void clear(const Graphics& graphics, float r, float g, float b, float a);

    // the view used to draw into the texture
    ::ID3D11RenderTargetView* render_target_view() const;

    // the view used to read the texture in a shader
    ::ID3D11ShaderResourceView* shader_resource_view() const;

private:

    ::Microsoft::WRL::ComPtr<::ID3D11Texture2D> m_texture {};              // GPU memory holding the pixels
    ::Microsoft::WRL::ComPtr<::ID3D11RenderTargetView> m_rtv {};           // view for drawing into it
    ::Microsoft::WRL::ComPtr<::ID3D11ShaderResourceView> m_srv {};         // view for reading it in shaders

};

#endif // RENDER_TARGET_HH
