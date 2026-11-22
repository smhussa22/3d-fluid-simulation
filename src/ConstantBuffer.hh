#ifndef CONSTANT_BUFFER_HH
#define CONSTANT_BUFFER_HH

#include "Graphics.hh"

#include <d3d11.h>
#include <wrl/client.h>

#include <cstddef>

// RAII wrapper around a small GPU buffer of values shared by a whole draw call (e.g. the camera matrix)
class ConstantBuffer
{

public:

    // creates a constant buffer of the given size in bytes (rounded up to a multiple of 16, which the GPU requires)
    ConstantBuffer(const Graphics& graphics, std::size_t bytes);

    ~ConstantBuffer() = default;

    ConstantBuffer(const ConstantBuffer&) = delete;
    ConstantBuffer& operator=(const ConstantBuffer&) = delete;
    ConstantBuffer(ConstantBuffer&&) = delete;
    ConstantBuffer& operator=(ConstantBuffer&&) = delete;

    // uploads new values over the whole buffer (data must hold at least the size given at creation)
    void write(const Graphics& graphics, const void* data);

    // attaches the buffer to a vertex shader slot, matching register(bN) in the hlsl
    void bind_vertex(const Graphics& graphics, ::UINT slot) const;

    // attaches the buffer to a pixel shader slot, matching register(bN) in the hlsl
    void bind_pixel(const Graphics& graphics, ::UINT slot) const;

    // attaches the buffer to a compute shader slot, matching register(bN) in the hlsl
    void bind_compute(const Graphics& graphics, ::UINT slot) const;

    // the underlying buffer resource
    ::ID3D11Buffer* buffer() const;

private:

    ::Microsoft::WRL::ComPtr<::ID3D11Buffer> m_buffer {}; // GPU memory holding the values

};

#endif // CONSTANT_BUFFER_HH
