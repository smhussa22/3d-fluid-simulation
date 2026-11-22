#include "Shader.hh"
#include <d3dcompiler.h>
#include <print>
#include "check.hh"
Shader::Shader(const Graphics& graphics, const std::wstring& file_path)
{

    // compile the .hlsl file
    ::Microsoft::WRL::ComPtr<::ID3DBlob> blob {}; // contains the compiled bytes for the file if successful
    ::Microsoft::WRL::ComPtr<::ID3DBlob> errors {}; // contains the compiler error message if unsuccessful
    uint32_t flags { 0u };
    auto result { ::D3DCompileFromFile(file_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", flags, 0u, &blob, &errors) };
    
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

    // create on success
    check(graphics.device()->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_shader), "CreateComputeShader");

}

void Shader::use(const Graphics& graphics) const
{

    graphics.context()->CSSetShader(m_shader.Get(), nullptr, 0u);

}

void Shader::dispatch(const Graphics& graphics, uint32_t x, uint32_t y, uint32_t z) const
{
    
    graphics.context()->Dispatch(x, y, z);

}