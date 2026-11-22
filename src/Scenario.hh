#ifndef SCENARIO_HH
#define SCENARIO_HH

#include <DirectXMath.h>

#include <vector>

// a cube of n x n x n particles, 0.1 m apart, in a tank just big enough to hold it (plus headroom above)
struct Scenario
{

    std::vector<::DirectX::XMFLOAT4> points {};   // starting particle positions
    ::DirectX::XMFLOAT3 bounds_min {};            // tank minimum corner
    ::DirectX::XMFLOAT3 bounds_max {};            // tank maximum corner

};

// builds the scenario; lift raises the cube above the floor (0 = resting on it), and the tank grows to keep the same headroom above the cube
Scenario make_scenario(int n, float lift = 0.0f);

#endif // SCENARIO_HH
