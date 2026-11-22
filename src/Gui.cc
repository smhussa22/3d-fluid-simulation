#include "Gui.hh"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_glfw.h>

Gui::Gui(const Window& window, const Graphics& graphics)
{

    // create the context, then connect it to the window for input and to the device for drawing
    IMGUI_CHECKVERSION();
    ::ImGui::CreateContext();
    ::ImGui_ImplGlfw_InitForOther(window.handle(), true);
    ::ImGui_ImplDX11_Init(graphics.device(), graphics.context());

}

Gui::~Gui()
{

    ::ImGui_ImplDX11_Shutdown();
    ::ImGui_ImplGlfw_Shutdown();
    ::ImGui::DestroyContext();

}

void Gui::beginFrame()
{

    ::ImGui_ImplDX11_NewFrame();
    ::ImGui_ImplGlfw_NewFrame();
    ::ImGui::NewFrame();

}

void Gui::drawFps()
{

    // pin a small see-through box to the top-right corner (pivot at its own top-right so it hugs the edge)
    const ::ImGuiIO& io { ::ImGui::GetIO() };
    ::ImGui::SetNextWindowPos(::ImVec2 { io.DisplaySize.x - 10.0f, 10.0f }, ImGuiCond_Always, ::ImVec2 { 1.0f, 0.0f });
    ::ImGui::SetNextWindowBgAlpha(0.35f);

    const ::ImGuiWindowFlags flags { ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs };

    if (::ImGui::Begin("fps", nullptr, flags))
    {

        ::ImGui::Text("%.0f FPS", io.Framerate);

    }

    ::ImGui::End();

}

bool Gui::drawControls(Simulation::Settings& settings, bool& show_depth)
{

    bool reset_clicked { false };
    ::ImGui::SetNextWindowPos(::ImVec2 { 10.0f, 10.0f }, ImGuiCond_Always);

    if (::ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {

        // sliders for the physics settings (log scale so small values are easy to hit)
        ::ImGui::SliderFloat("Viscosity", &settings.viscosity, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ::ImGui::SliderFloat("Stiffness", &settings.stiffness, 0.0f, 500.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ::ImGui::SliderFloat("Damping", &settings.damping, 0.0f, 1.0f, "%.2f");
        ::ImGui::SliderFloat("Gravity", &settings.gravity, -20.0f, 0.0f, "%.1f");

        ::ImGui::Checkbox("Show depth texture", &show_depth);

        if (::ImGui::Button("Reset (R)"))
        {

            reset_clicked = true;

        }

    }

    ::ImGui::End();
    return reset_clicked;

}

bool Gui::wantsMouse() const
{

    return ::ImGui::GetIO().WantCaptureMouse;

}

void Gui::render()
{

    ::ImGui::Render();
    ::ImGui_ImplDX11_RenderDrawData(::ImGui::GetDrawData());

}
