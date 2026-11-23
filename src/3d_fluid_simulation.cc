#include "Graphics.hh"
#include "Window.hh"
#include "Buffer.hh"
#include "Shader.hh"
#include "RenderShader.hh"
#include "ConstantBuffer.hh"
#include "Camera.hh"
#include "RenderTarget.hh"
#include "Gui.hh"
#include "Benchmark.hh"
#include "GpuSimulation.hh"
#include "Scenario.hh"

#include <algorithm>
#include <cmath>
#include <exception>
#include <print>
#include <string_view>
#include <utility>
#include <vector>
#include <cstdint>


#include <DirectXMath.h>

int main(int argc, char** argv)
try
{

    // open a 1280x720 window and bring up Direct3D 11 on it
    Window window { 1280, 720, "3D Fluid Simulation" };
    Graphics graphics { window.hwnd(), window.width(), window.height() };
    std::println("Direct3D 11 ready, feature level {:#x}", static_cast<unsigned>(graphics.featureLevel()));

    // "--benchmark" skips the interactive window loop: it checks the GPU against the CPU, times both, prints a table, and exits
    if (argc > 1 && std::string_view { argv[1] } == "--benchmark")
    {

        run_benchmark(graphics);
        return 0;

    }

    // "--particles N" fills a tank with a cube of about N particles (n x n x n), dropped from a little above the floor; with no argument the small default block is used
    int requested_particles { 0 };
    float particle_spacing { 0.1f }; // how far apart the particles start, in meters; smaller = smaller particles = finer water (needs more particles to fill the same volume)

    for (int i { 1 }; i + 1 < argc; ++i)
    {

        if (std::string_view { argv[i] } == "--particles")
        {

            requested_particles = std::stoi(argv[i + 1]);

        }

        if (std::string_view { argv[i] } == "--spacing")
        {

            particle_spacing = std::stof(argv[i + 1]);

        }

    }

    // "--profile N" drops N particles and prints how long each GPU pass takes over time, then exits
    for (int i { 1 }; i + 1 < argc; ++i)
    {

        if (std::string_view { argv[i] } == "--profile")
        {

            run_profile(graphics, std::stoi(argv[i + 1]));
            return 0;

        }

    }

    std::vector<::DirectX::XMFLOAT4> points {};
    ::DirectX::XMFLOAT3 bounds_min { -0.6f, -0.6f, -0.6f };
    ::DirectX::XMFLOAT3 bounds_max { 0.6f, 0.6f, 0.6f };

    if (requested_particles > 0)
    {

        const int side { std::max(1, static_cast<int>(std::lround(std::cbrt(static_cast<double>(requested_particles))))) };
        Scenario scenario { make_scenario(side, 0.6f, particle_spacing) };
        points = std::move(scenario.points);
        bounds_min = scenario.bounds_min;
        bounds_max = scenario.bounds_max;
        std::println("{} particles ({} x {} x {} cube) in a {:.1f} x {:.1f} x {:.1f} m tank", points.size(), side, side, side, bounds_max.x - bounds_min.x, bounds_max.y - bounds_min.y, bounds_max.z - bounds_min.z);

    }
    else
    {

        for (int x { 0 }; x < 10; ++x)
        {

            for (int y { 0 }; y < 10; ++y)
            {

                for (int z { 0 }; z < 10; ++z)
                {

                    points.emplace_back(-0.45f + x * 0.1f, -0.45f + y * 0.1f, -0.45f + z * 0.1f, 1.0f);

                }

            }

        }

    }

    // hand the starting positions to the GPU simulation, which keeps them in its own buffers
    const std::size_t particle_count { points.size() };
    // how big each drawn sphere is: a bit bigger than half the spacing, so neighboring spheres overlap and fill the gaps between particles
    const float particle_radius { 0.75f * particle_spacing };
    GpuSimulation simulation { graphics, std::move(points), bounds_min, bounds_max, particle_spacing };
    RenderShader render_shader { graphics, L"shaders/particles.hlsl" };

    // the depth pass: the spheres drawn again, writing each pixel's distance from the camera into a texture (0 where there is no water); the debug shader shows that texture on screen
    RenderTarget depth_target { graphics, window.width(), window.height(), DXGI_FORMAT_R32_FLOAT };
    RenderShader depth_shader { graphics, L"shaders/particle_depth.hlsl" };
    RenderShader show_depth_shader { graphics, L"shaders/show_depth.hlsl" };
    Gui::ViewOptions view {};

    // the blur: two more depth textures (the first pass writes into the second, the second pass writes the smoothed result into the third), and its settings
    RenderTarget blur_target { graphics, window.width(), window.height(), DXGI_FORMAT_R32_FLOAT };
    RenderTarget smooth_target { graphics, window.width(), window.height(), DXGI_FORMAT_R32_FLOAT };
    RenderShader blur_shader { graphics, L"shaders/blur_depth.hlsl" };

    struct BlurParams
    {

        float direction[2];     // (1, 0) blurs along x, (0, 1) along y
        float blur_radii;       // how far the blur reaches, in particle radii as they look on screen
        float depth_sigma;      // weight falloff with depth difference, in meters
        float particle_radius;  // world-space radius of one particle
        float focal_pixels;     // pixels per meter at one meter distance
        float padding[2];       // fills the second 16 bytes

    };

    ConstantBuffer blur_params_buffer { graphics, sizeof(BlurParams) };

    // the normals: a texture of surface directions made from the smoothed depth, and the numbers the shader needs to turn a pixel and its depth back into a 3D position
    RenderTarget normal_target { graphics, window.width(), window.height(), DXGI_FORMAT_R16G16B16A16_FLOAT };
    RenderShader normals_shader { graphics, L"shaders/normals.hlsl" };
    RenderShader show_normals_shader { graphics, L"shaders/show_normals.hlsl" };

    struct NormalParams
    {

        float tan_half_fov[2];   // how far the view spreads sideways (x, with aspect) and upward (y) per meter of distance
        float screen_size[2];    // the window size in pixels

    };

    const float tan_half_fov_y { std::tan(::DirectX::XMConvertToRadians(Camera::fov_degrees) * 0.5f) };
    const float focal_pixels { static_cast<float>(window.height()) / (2.0f * tan_half_fov_y) };
    const float aspect { static_cast<float>(window.width()) / static_cast<float>(window.height()) };
    const NormalParams normal_params { { tan_half_fov_y * aspect, tan_half_fov_y }, { static_cast<float>(window.width()), static_cast<float>(window.height()) } };
    ConstantBuffer normal_params_buffer { graphics, sizeof(NormalParams) };
    normal_params_buffer.write(graphics, &normal_params);

    // the water look: a texture of how much water the view passes through, a texture of the scene behind the water (floor and sky), and the shaders that make them and combine everything
    RenderTarget thickness_target { graphics, window.width(), window.height(), DXGI_FORMAT_R16_FLOAT };
    RenderTarget background_target { graphics, window.width(), window.height(), DXGI_FORMAT_R8G8B8A8_UNORM };
    RenderShader thickness_shader { graphics, L"shaders/particle_thickness.hlsl" };
    RenderShader background_shader { graphics, L"shaders/background.hlsl" };
    RenderShader water_shader { graphics, L"shaders/water.hlsl" };

    struct WaterParams
    {

        float refraction_strength;   // how far the background shifts behind thick, tilted water, in pixels
        float absorption_strength;   // how quickly water darkens and tints with thickness
        float floor_y;               // height of the floor (the bottom of the tank)
        float padding;               // fills the 16 bytes

    };

    ConstantBuffer water_params_buffer { graphics, sizeof(WaterParams) };

    // one blur pass: read `from`, write the smoothed result into `to`, looking along (x, y)
    const auto blur_pass = [&](const RenderTarget& from, RenderTarget& to, float x, float y)
    {

        const BlurParams params { { x, y }, view.blur_radii, view.blur_depth_radii * particle_radius, particle_radius, focal_pixels, { 0.0f, 0.0f } };
        blur_params_buffer.write(graphics, &params);
        graphics.set_render_target(to.render_target_view(), false);

        ::ID3D11ShaderResourceView* source { from.shader_resource_view() };
        graphics.context()->PSSetShaderResources(0u, 1u, &source);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        graphics.context()->Draw(3u, 0u);

        ::ID3D11ShaderResourceView* no_source { nullptr };
        graphics.context()->PSSetShaderResources(0u, 1u, &no_source);

    };

    for (int i { 1 }; i < argc; ++i)
    {

        if (std::string_view { argv[i] } == "--show-depth")
        {

            view.show_depth = true;

        }

        if (std::string_view { argv[i] } == "--spheres")
        {

            view.surface = false;

        }

        if (std::string_view { argv[i] } == "--show-normals")
        {

            view.show_normals = true;

        }

        if (std::string_view { argv[i] } == "--no-blur")
        {

            view.blur_enabled = false;

        }

    }

    // the tank outline: a shader that draws a box's 12 edges, and the box corners it reads
    RenderShader box_shader { graphics, L"shaders/box.hlsl" };
    ConstantBuffer box_buffer { graphics, 2uz * sizeof(::DirectX::XMFLOAT4) };
    const ::DirectX::XMFLOAT4 box_corners[2] { { simulation.boundsMin().x, simulation.boundsMin().y, simulation.boundsMin().z, 0.0f }, { simulation.boundsMax().x, simulation.boundsMax().y, simulation.boundsMax().z, 0.0f } };
    box_buffer.write(graphics, box_corners);

    // orbit camera, plus the constant buffer that carries its matrix to the shader
    Camera camera {};
    camera.frame(bounds_min, bounds_max);
    ConstantBuffer camera_buffer { graphics, sizeof(Camera::ShaderData) };

    // how the particles are drawn: sphere radius, the light, and the water color (the layout matches the ParticleParams cbuffer in particles.hlsl)
    struct ParticleParams
    {

        float radius;                       // world-space radius of each drawn sphere
        float padding[3];                   // fills the first 16 bytes
        ::DirectX::XMFLOAT4 light_direction; // direction toward the light (xyz)
        ::DirectX::XMFLOAT4 color;           // base color of the water (rgb)

    };

    const ParticleParams particle_params { particle_radius, { 0.0f, 0.0f, 0.0f }, { 0.4f, 0.8f, 0.45f, 0.0f }, { 0.25f, 0.55f, 1.0f, 1.0f } };
    ConstantBuffer particle_params_buffer { graphics, sizeof(ParticleParams) };
    particle_params_buffer.write(graphics, &particle_params);
    Gui gui { window, graphics };

    // the physics advances in fixed-size steps, however fast frames are drawn
    // the largest safe step shrinks with the smoothing radius (a pressure wave must not cross more than a fraction of it per step): 0.3 * radius / sound speed, where the sound speed here is sqrt(stiffness)
    const double step_time { std::min(1.0 / 480.0, 0.3 * static_cast<double>(simulation.kernel_radius()) / std::sqrt(static_cast<double>(simulation.settings().stiffness))) };
    double previous_time { window.time() };
    constexpr int max_steps_per_frame { 16 }; // if the physics falls behind, run slower than real time instead of spiraling
    double accumulator { 0.0 };
    bool r_was_down { false };

    // keep running until closed (on escape key), keep redrawing and handle input
    while (!window.shouldClose())
    {

        if (window.key(GLFW_KEY_ESCAPE))
        {

            window.close();

        }

        // measure the last frame (capped so a long stall doesn't make the physics race to catch up), then run as many fixed steps as that time allows
        const double now { window.time() };
        accumulator += std::min(now - previous_time, 0.05);
        previous_time = now;

        int steps_this_frame { 0 };

        while (accumulator >= step_time && steps_this_frame < max_steps_per_frame)
        {

            simulation.step(graphics, static_cast<float>(step_time));
            accumulator -= step_time;
            ++steps_this_frame;

        }

        // hit the cap: drop the time we couldn't simulate so it doesn't pile up
        if (accumulator >= step_time)
        {

            accumulator = 0.0;

        }

        // move the camera from mouse input unless the mouse is over the panel (scroll is discarded there so it doesn't build up), then upload its new matrix
        if (!gui.wantsMouse())
        {

            camera.update(window);

        }
        else
        {

            window.takeScroll();

        }


        const Camera::ShaderData camera_data { camera.shader_data(static_cast<float>(window.width()) / static_cast<float>(window.height())) };
        camera_buffer.write(graphics, &camera_data);

        graphics.beginFrame(0.0f, 0.0f, 0.0f, 1.0f);
        gui.beginFrame();
        gui.drawFps();

        // restart the simulation when Reset is clicked or R is pressed (once per press)
        const bool r_is_down { window.key(GLFW_KEY_R) };

        if (gui.drawControls(simulation.settings(), view) || (r_is_down && !r_was_down))
        {

            simulation.reset(graphics);

        }

        r_was_down = r_is_down;

        // everything the sphere passes share: the particle positions, the camera, and the sphere settings
        ::ID3D11ShaderResourceView* srv { simulation.position_buffer().shader_resource_view() };
        camera_buffer.bind_vertex(graphics, 0u);
        camera_buffer.bind_pixel(graphics, 0u);
        particle_params_buffer.bind_vertex(graphics, 2u);
        particle_params_buffer.bind_pixel(graphics, 2u);
        graphics.context()->VSSetShaderResources(0u, 1u, &srv);
        graphics.context()->IASetInputLayout(nullptr);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // the water surface and both debug views are built from the same maps (depth, smoothed depth, normals), so build them whenever any of those is showing
        const bool needs_maps { view.surface || view.show_depth || view.show_normals };
        const bool show_surface { view.surface && !view.show_depth && !view.show_normals };

        if (needs_maps)
        {

            // depth pass: draw the spheres into the depth texture, then go back to drawing into the window with a fresh depth buffer
            depth_target.clear(graphics, 0.0f, 0.0f, 0.0f, 0.0f);
            graphics.set_render_target(depth_target.render_target_view());
            graphics.clear_depth();
            depth_shader.use(graphics);
            graphics.context()->DrawInstanced(4u, static_cast<::UINT>(particle_count), 0u, 0u);

            // smooth the depth: blur along x into the second texture, then along y into the third
            if (view.blur_enabled)
            {

                blur_shader.use(graphics);
                blur_params_buffer.bind_pixel(graphics, 3u);
                blur_pass(depth_target, blur_target, 1.0f, 0.0f);
                blur_pass(blur_target, smooth_target, 0.0f, 1.0f);

            }

            // normals: read the smoothed depth (or the raw depth if blurring is off) and write each pixel's surface direction
            ::ID3D11ShaderResourceView* depth_for_normals { view.blur_enabled ? smooth_target.shader_resource_view() : depth_target.shader_resource_view() };
            normals_shader.use(graphics);
            normal_params_buffer.bind_pixel(graphics, 3u);
            graphics.set_render_target(normal_target.render_target_view(), false);
            graphics.context()->PSSetShaderResources(0u, 1u, &depth_for_normals);
            graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphics.context()->Draw(3u, 0u);

            ::ID3D11ShaderResourceView* no_depth { nullptr };
            graphics.context()->PSSetShaderResources(0u, 1u, &no_depth);

            graphics.bind_back_buffer();
            graphics.clear_depth();
            graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        }

        if (view.show_normals)
        {

            // debug view: the normals as colors, covering the window
            ::ID3D11ShaderResourceView* normal_view { normal_target.shader_resource_view() };
            graphics.bind_back_buffer(false);
            show_normals_shader.use(graphics);
            graphics.context()->PSSetShaderResources(0u, 1u, &normal_view);
            graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphics.context()->Draw(3u, 0u);

            ::ID3D11ShaderResourceView* no_normals { nullptr };
            graphics.context()->PSSetShaderResources(0u, 1u, &no_normals);
            graphics.bind_back_buffer();

        }
        else if (view.show_depth)
        {

            // debug view: the depth texture as grayscale bands, covering the window (then let go of the texture so the next depth pass can draw into it again)
            ::ID3D11ShaderResourceView* depth_view { view.blur_enabled ? smooth_target.shader_resource_view() : depth_target.shader_resource_view() };
            graphics.bind_back_buffer(false);
            show_depth_shader.use(graphics);
            graphics.context()->PSSetShaderResources(0u, 1u, &depth_view);
            graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphics.context()->Draw(3u, 0u);

            ::ID3D11ShaderResourceView* no_depth_view { nullptr };
            graphics.context()->PSSetShaderResources(0u, 1u, &no_depth_view);
            graphics.bind_back_buffer();

        }
        else if (show_surface)
        {

            // thickness: every sphere adds the length of the line of sight through it (blending is switched to add), giving how much water the view passes through at each pixel
            thickness_target.clear(graphics, 0.0f, 0.0f, 0.0f, 0.0f);
            graphics.set_render_target(thickness_target.render_target_view(), false);
            graphics.set_additive_blending(true);
            thickness_shader.use(graphics);
            graphics.context()->DrawInstanced(4u, static_cast<::UINT>(particle_count), 0u, 0u);
            graphics.set_additive_blending(false);

            // the settings the background and water shaders read
            const WaterParams water_params { view.refraction_strength, view.absorption_strength, simulation.boundsMin().y, 0.0f };
            water_params_buffer.write(graphics, &water_params);
            water_params_buffer.bind_pixel(graphics, 4u);
            normal_params_buffer.bind_pixel(graphics, 3u);
            graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // background: the floor and sky, drawn into a texture so the water can bend and tint what is behind it
            graphics.set_render_target(background_target.render_target_view(), false);
            background_shader.use(graphics);
            graphics.context()->Draw(3u, 0u);

            // the water: combine the normals, the thickness and the background into the final picture in the window
            ::ID3D11ShaderResourceView* water_inputs[3] { normal_target.shader_resource_view(), thickness_target.shader_resource_view(), background_target.shader_resource_view() };
            graphics.bind_back_buffer(false);
            water_shader.use(graphics);
            graphics.context()->PSSetShaderResources(0u, 3u, water_inputs);
            graphics.context()->Draw(3u, 0u);

            ::ID3D11ShaderResourceView* no_inputs[3] { nullptr, nullptr, nullptr };
            graphics.context()->PSSetShaderResources(0u, 3u, no_inputs);
            graphics.bind_back_buffer();

        }
        else
        {

            // draw every particle as a lit sphere (one camera-facing quad each), reading positions straight from the buffer the compute shaders write
            render_shader.use(graphics);
            graphics.context()->DrawInstanced(4u, static_cast<::UINT>(particle_count), 0u, 0u);

        }

        // draw the tank outline as 12 lines, on top of whatever is showing
        box_shader.use(graphics);
        box_buffer.bind_vertex(graphics, 1u);
        graphics.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        graphics.context()->Draw(24u, 0u);
        // draw the on-screen overlay on top of the scene
        gui.render();
        
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