#ifndef SCENARIO_HH
#define SCENARIO_HH

#include <DirectXMath.h>

#include <vector>

// a cube of n x n x n particles, `spacing` meters apart (smaller spacing = smaller particles = finer detail), in a tank just big enough to hold it (plus headroom above)
struct Scenario
{

    std::vector<::DirectX::XMFLOAT4> points {};   // starting particle positions
    ::DirectX::XMFLOAT3 bounds_min {};            // tank minimum corner
    ::DirectX::XMFLOAT3 bounds_max {};            // tank maximum corner
    bool beach { false };                         // whether the floor slopes up into a beach
    float beach_start { 0.0f };                   // x where the beach starts rising
    float beach_angle { 0.0f };                   // slope of the beach, in radians

};

// a long tank with deep water at the low-x end and a sandy beach sloping up at the other; the water starts level, so the wave-maker paddle at the deep end makes the waves
Scenario make_beach_scenario(float spacing);

// builds the scenario; lift raises the cube above the floor (0 = resting on it), and the tank grows to keep the same headroom above the cube
Scenario make_scenario(int n, float lift = 0.0f, float spacing = 0.1f);

#endif // SCENARIO_HH
