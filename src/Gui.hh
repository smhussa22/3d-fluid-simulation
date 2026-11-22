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

        bool surface { true };               // draw the water as a smooth refractive surface (off = draw each particle as a sphere)
        float refraction_strength { 8.0f }; // how far the background shifts behind thick, tilted water, in pixels
        float absorption_strength { 1.0f };  // how quickly water darkens and tints with thickness
        bool show_depth { false };           // draw the depth texture over the scene (debug view)
        bool show_normals { false };         // draw the surface normals as colors over the scene (debug view; takes priority over the depth view)
        bool blur_enabled { true };          // smooth the depth texture before showing or using it
        int blur_radius { 20 };              // how many pixels the blur reaches out on each side
        float blur_spatial_sigma { 10.0f };   // how quickly the blur weight falls off with pixel distance
        float blur_depth_sigma { 0.15f };     // how quickly the blur weight falls off with depth difference, in meters

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
