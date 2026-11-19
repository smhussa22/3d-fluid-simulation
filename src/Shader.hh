#ifndef SHADER_HH
#define SHADER_HH

#include "Graphics.hh"

#include <d3d11.h>
#include <wrl/client.h>

#include <string>

// a compiled HLSL compute shader: a program that runs on the GPU, one copy per piece of work (e.g. one per particle)
class Shader
{
public:

    // takes in the Graphics to create a GPU shader thru it
    // takes a wide string to the file path to load the .hlsl file because D3DCompileFromFile takes a wide char type.
    Shader(const Graphics& graphics, const std::wstring& file_path );
    ~Shader() = default;

    Shader (const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader (Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    // makes this the active compute shader, so the next dispatch runs it
    // iow, picks which shader the gpu will run next
    void use(const Graphics& graphics) const;

    // launches x * y * z thread groups of the active shader on the GPU (the number of groups, not threads)
    // iow, tells the gpu to start running the shader now
    void dispatch(const Graphics& graphics, uint32_t x, uint32_t y, uint32_t z) const;

private:
    ::Microsoft::WRL::ComPtr<::ID3D11ComputeShader> m_shader; // the compiled shader on the GPU

};

#endif // SHADER_HH