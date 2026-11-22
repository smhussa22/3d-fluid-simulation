#include "RenderShader.hh"

#include "check.hh"

#include <d3dcompiler.h>

#include <cstdint>
#include <print>

namespace
{

    // compiles one entry point of an hlsl file for the given shader target (e.g. vs_5_0) and returns the compiled bytes
    ::Microsoft::WRL::ComPtr<::ID3DBlob> _compile(const std::wstring& file_path, const char* entry, const char* target)
    {

        ::Microsoft::WRL::ComPtr<::ID3DBlob> blob {}; // contains the compiled bytes for the file if successful
        ::Microsoft::WRL::ComPtr<::ID3DBlob> errors {}; // contains the compiler error message if unsuccessful
        uint32_t flags { 0u };
        auto result { ::D3DCompileFromFile(file_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0u, &blob, &errors) };

        // if it failed, print the compiler's message, then stop
        if (FAILED(result))
        {

            if (errors != nullptr)
            {

                std::println("{}", static_cast<const char*>(errors->GetBufferPointer()));

            }

            // throws
            check(result, "D3DCompileFromFile"); 

        }

        return blob;

    }

}

RenderShader::RenderShader(const Graphics& graphics, const std::wstring& file_path)
{

    // compile the vertex and pixel shader entry points from the same file
    auto vertex_blob { _compile(file_path, "vs_main", "vs_5_0") };
    auto pixel_blob { _compile(file_path, "ps_main", "ps_5_0") };

    // create both shaders on the gpu
    check(graphics.device()->CreateVertexShader(vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), nullptr, &m_vertex_shader), "CreateVertexShader");
    check(graphics.device()->CreatePixelShader(pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize(), nullptr, &m_pixel_shader), "CreatePixelShader");

}

void RenderShader::use(const Graphics& graphics) const
{

    graphics.context()->VSSetShader(m_vertex_shader.Get(), nullptr, 0u);
    graphics.context()->PSSetShader(m_pixel_shader.Get(), nullptr, 0u);

}
