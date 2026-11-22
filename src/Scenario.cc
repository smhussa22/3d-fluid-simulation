#include "Scenario.hh"

Scenario make_scenario(int n, float lift)
{

    Scenario scenario {};
    scenario.points.reserve(static_cast<std::size_t>(n) * static_cast<std::size_t>(n) * static_cast<std::size_t>(n));
    const float half_width { static_cast<float>(n) * 0.05f };

    for (int x { 0 }; x < n; ++x)
    {

        for (int y { 0 }; y < n; ++y)
        {

            for (int z { 0 }; z < n; ++z)
            {

                scenario.points.emplace_back(-half_width + 0.05f + static_cast<float>(x) * 0.1f, 0.05f + lift + static_cast<float>(y) * 0.1f, -half_width + 0.05f + static_cast<float>(z) * 0.1f, 1.0f);

            }

        }

    }

    scenario.bounds_min = { -half_width - 0.3f, 0.0f, -half_width - 0.3f };
    scenario.bounds_max = { half_width + 0.3f, static_cast<float>(n) * 0.1f + 1.0f + lift, half_width + 0.3f };
    return scenario;

}
