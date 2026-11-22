#include "ConstantBuffer.hh"

#include "check.hh"

ConstantBuffer::ConstantBuffer(const Graphics& graphics, std::size_t bytes)
{

    // constant buffer sizes must be a multiple of 16 bytes
    const std::size_t padded_bytes { (bytes + 15uz) / 16uz * 16uz };

    ::D3D11_BUFFER_DESC desc {};
    desc.ByteWidth = static_cast<::UINT>(padded_bytes);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    check(graphics.device()->CreateBuffer(&desc, nullptr, &m_buffer), "CreateBuffer (constant)");

}

void ConstantBuffer::write(const Graphics& graphics, const void* data)
{

    graphics.context()->UpdateSubresource(m_buffer.Get(), 0u, nullptr, data, 0u, 0u);

}

void ConstantBuffer::bind_vertex(const Graphics& graphics, ::UINT slot) const
{

    ::ID3D11Buffer* buffer { m_buffer.Get() };
    graphics.context()->VSSetConstantBuffers(slot, 1u, &buffer);

}

void ConstantBuffer::bind_pixel(const Graphics& graphics, ::UINT slot) const
{

    ::ID3D11Buffer* buffer { m_buffer.Get() };
    graphics.context()->PSSetConstantBuffers(slot, 1u, &buffer);

}

void ConstantBuffer::bind_compute(const Graphics& graphics, ::UINT slot) const
{

    ::ID3D11Buffer* buffer { m_buffer.Get() };
    graphics.context()->CSSetConstantBuffers(slot, 1u, &buffer);

}

::ID3D11Buffer* ConstantBuffer::buffer() const
{

    return m_buffer.Get();

}
