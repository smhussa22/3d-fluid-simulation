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
        float blur_radii { 2.5f };           // how far the blur reaches, in particle radii as they look on screen (so it scales with particle size and distance)
        float blur_depth_radii { 1.5f };     // how different two depths can be, in particle radii, and still be blended together
        float spray_threshold { 0.3f };      // particles whose density is below this fraction of the rest density count as spray, not surface
        float spray_size { 0.5f };           // radius of a drawn spray droplet, in particle radii
        float thickness_fade { 3.0f };       // water thinner than this many particle diameters fades toward transparent and white
        float foam_speed { 2.0f };           // particle speed in m/s where foam starts (full foam at twice this)
        float foam_amount { 0.8f };          // how white thin, fast water gets
        bool ocean { false };                // draw the procedural ocean instead of the particle tank
        float wind_degrees { 35.0f };        // direction the ocean waves travel, in degrees around the up axis
        float wave_height { 1.5f };          // height of the biggest wave, trough to crest, in meters
        float wavelength { 40.0f };          // distance between crests of the biggest wave, in meters
        float choppiness { 0.8f };           // 0 = rounded waves, 1 = sharp crests

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
