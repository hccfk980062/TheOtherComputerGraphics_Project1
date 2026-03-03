#include <imgui.h>
#include <imgui_internal.h> // Optional: Only needed if you want perfectly accurate internal track bounds, but we'll use public API.
#include <vector>
#include <cmath>

namespace CG {

    // Helper function to create a slider with magnetic pins
    bool SliderFloatWithPins(const char* label, float* v, float v_min, float v_max, const std::vector<float>& pins, float snap_threshold = 0.05f) {

        // 1. Draw the standard slider
        bool changed = ImGui::SliderFloat(label, v, v_min, v_max);

        // 2. Fetch drawing information
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Get the top-left coordinate of the slider
        ImVec2 p_min = ImGui::GetItemRectMin();

        // ImGui::GetItemRectMax() includes the label. To get just the slider bar's width,
        // we use ImGui::CalcItemWidth().
        float slider_width = ImGui::CalcItemWidth();
        ImVec2 p_max = ImVec2(p_min.x + slider_width, ImGui::GetItemRectMax().y);

        // 3. Draw the visual pins
        for (float pin : pins) {
            // Ignore pins outside the slider's range
            if (pin < v_min || pin > v_max) continue;

            // Calculate normalized position (0.0 to 1.0)
            float t = (pin - v_min) / (v_max - v_min);

            // Calculate screen X coordinate for the pin
            float pin_x = p_min.x + (t * slider_width);

            // Draw a small vertical line for the pin over the slider
            draw_list->AddLine(
                ImVec2(pin_x, p_min.y + 2.0f),       // Slight padding from top
                ImVec2(pin_x, p_max.y - 2.0f),       // Slight padding from bottom
                IM_COL32(200, 200, 200, 255),        // Pin color (Light Gray)
                2.0f                                 // Pin thickness
            );
        }

        // 4. Snapping logic (Magnetic effect)
        // We only override the value while the item is actively being grabbed/dragged
        if (ImGui::IsItemActive()) {
            float closest_pin = *v;
            float min_dist = (v_max - v_min);

            // Find the closest pin
            for (float pin : pins) {
                float dist = std::abs(*v - pin);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_pin = pin;
                }
            }

            // If the current value is within our snapping threshold, snap to it!
            float range = v_max - v_min;
            if (min_dist <= (snap_threshold * range)) {
                *v = closest_pin;
                // Optional: Force update the UI visually if you snapped
                changed = true;
            }
        }

        return changed;
    }
}