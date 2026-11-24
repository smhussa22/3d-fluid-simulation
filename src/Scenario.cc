#include "Scenario.hh"

#include <algorithm>
#include <cmath>

Scenario make_scenario(int n, float lift, float spacing)
{

    Scenario scenario {};
    scenario.points.reserve(static_cast<std::size_t>(n) * static_cast<std::size_t>(n) * static_cast<std::size_t>(n));
    const float half_width { static_cast<float>(n) * spacing * 0.5f };
    const float margin { std::max(0.3f, 4.0f * spacing) }; // gap between the cube and the tank walls

    for (int x { 0 }; x < n; ++x)
    {

        for (int y { 0 }; y < n; ++y)
        {

            for (int z { 0 }; z < n; ++z)
            {

                scenario.points.emplace_back(-half_width + 0.5f * spacing + static_cast<float>(x) * spacing, 0.5f * spacing + lift + static_cast<float>(y) * spacing, -half_width + 0.5f * spacing + static_cast<float>(z) * spacing, 1.0f);

            }

        }

    }

    scenario.bounds_min = { -half_width - margin, 0.0f, -half_width - margin };
    scenario.bounds_max = { half_width + margin, static_cast<float>(n) * spacing + 1.0f + lift, half_width + margin };
    return scenario;

}

Scenario make_beach_scenario(float spacing)
{

    // the tank: 10 m long (x), 3 m wide (z), with the beach starting halfway and rising at about 8 degrees; the water is 0.6 m deep
    constexpr float length { 10.0f };
    constexpr float width { 3.0f };
    constexpr float height { 2.0f };
    constexpr float water_depth { 0.6f };
    const float beach_start { 0.5f * length };
    const float beach_angle { 0.14f };

    Scenario scenario {};
    scenario.bounds_min = { 0.0f, 0.0f, -0.5f * width };
    scenario.bounds_max = { length, height, 0.5f * width };
    scenario.beach = true;
    scenario.beach_start = beach_start;
    scenario.beach_angle = beach_angle;

    // fill every lattice spot that is above the ground and below the water level (the ground is flat, then rises along the beach)
    const float tan_angle { std::tan(beach_angle) };

    for (float x { 0.5f * spacing }; x < length; x += spacing)
    {

        const float ground { std::max(0.0f, (x - beach_start) * tan_angle) };

        for (float y { ground + 0.5f * spacing }; y < water_depth; y += spacing)
        {

            for (float z { -0.5f * width + 0.5f * spacing }; z < 0.5f * width; z += spacing)
            {

                scenario.points.emplace_back(x, y, z, 1.0f);

            }

        }

    }

    return scenario;

}
