#include "Scenario.hh"

#include <algorithm>

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
