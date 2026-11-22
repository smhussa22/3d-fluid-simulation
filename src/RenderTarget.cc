#include "RenderTarget.hh"

#include "check.hh"

RenderTarget::RenderTarget(const Graphics& graphics, int width, int height, ::DXGI_FORMAT format)
{

    // a texture that can be both drawn into and sampled by shaders
    ::D3D11_TEXTURE2D_DESC desc {};
    desc.Width = static_cast<::UINT>(width);
    desc.Height = static_cast<::UINT>(height);
    desc.MipLevels = 1u;
    desc.ArraySize = 1u;
    desc.Format = format;
    desc.SampleDesc.Count = 1u;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    check(graphics.device()->CreateTexture2D(&desc, nullptr, &m_texture), "CreateTexture2D (render target)");
    check(graphics.device()->CreateRenderTargetView(m_texture.Get(), nullptr, &m_rtv), "CreateRenderTargetView (render target)");
    check(graphics.device()->CreateShaderResourceView(m_texture.Get(), nullptr, &m_srv), "CreateShaderResourceView (render target)");

}

void RenderTarget::clear(const Graphics& graphics, float r, float g, float b, float a)
{

    const float color[4] { r, g, b, a };
    graphics.context()->ClearRenderTargetView(m_rtv.Get(), color);

}

::ID3D11RenderTargetView* RenderTarget::render_target_view() const
{

    return m_rtv.Get();

}

::ID3D11ShaderResourceView* RenderTarget::shader_resource_view() const
{

    return m_srv.Get();

}
