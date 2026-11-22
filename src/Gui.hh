#ifndef GUI_HH
#define GUI_HH

#include "Graphics.hh"
#include "Simulation.hh"
#include "Window.hh"

// owns the Dear ImGui context and its GLFW + Direct3D 11 backends, for on-screen text and controls
class Gui
{

public:

    // what to show and how the depth smoothing is set up (changed from the controls panel)
    struct ViewOptions
    {

        bool show_depth { false };           // draw the depth texture over the scene (debug view)
        bool blur_enabled { true };          // smooth the depth texture before showing or using it
        int blur_radius { 12 };              // how many pixels the blur reaches out on each side
        float blur_spatial_sigma { 6.0f };   // how quickly the blur weight falls off with pixel distance
        float blur_depth_sigma { 0.1f };     // how quickly the blur weight falls off with depth difference, in meters

    };

    // creates the ImGui context and hooks it up to the window (input) and the Direct3D device (drawing)
    Gui(const Window& window, const Graphics& graphics);

    // shuts the backends down and destroys the ImGui context
    ~Gui();

    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;

    // starts a new ImGui frame; call once per frame before any ImGui widgets
    void beginFrame();

    // shows the current frames per second in a small overlay in the top-right corner
    void drawFps();

    // shows the controls panel in the top-left corner with sliders for the simulation settings; returns true on the frame Reset is clicked
    bool drawControls(Simulation::Settings& settings, ViewOptions& view);

    // true while the mouse is over the panel (or dragging it), so the scene should ignore the mouse
    bool wantsMouse() const;

    // draws everything built this frame on top of the scene; call after the scene, before present
    void render();

};

#endif // GUI_HH
