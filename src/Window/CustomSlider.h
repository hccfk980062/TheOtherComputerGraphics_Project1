#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <cmath>

namespace CG {

    // 帶有「磁吸刻度」的浮點滑桿
    // 在 pins 中指定的值附近，滑桿會自動吸附（snap）到該刻度，方便精準對齊關鍵幀時間點
    bool SliderFloatWithPins(const char* label, float* v, float v_min, float v_max,
                             const std::vector<float>& pins, float snap_threshold = 0.05f)
    {
        // 1. 繪製標準滑桿
        bool changed = ImGui::SliderFloat(label, v, v_min, v_max);

        // 2. 取得滑桿的螢幕空間邊界，用於計算刻度線的 X 座標
        ImDrawList* draw_list   = ImGui::GetWindowDrawList();
        ImVec2      p_min       = ImGui::GetItemRectMin();
        float       slider_width = ImGui::CalcItemWidth();  // 不含標籤的寬度
        ImVec2      p_max       = ImVec2(p_min.x + slider_width, ImGui::GetItemRectMax().y);

        // 3. 繪製刻度線（超出範圍的 pin 跳過）
        for (float pin : pins)
        {
            if (pin < v_min || pin > v_max) continue;

            float t     = (pin - v_min) / (v_max - v_min);  // 正規化位置 0~1
            float pin_x = p_min.x + (t * slider_width);

            draw_list->AddLine(
                ImVec2(pin_x, p_min.y + 2.0f),
                ImVec2(pin_x, p_max.y - 2.0f),
                IM_COL32(200, 200, 200, 255),  // 淺灰色刻度線
                2.0f
            );
        }

        // 4. 磁吸邏輯：只在拖曳中（IsItemActive）才覆蓋值，避免影響靜止狀態
        if (ImGui::IsItemActive())
        {
            float closest_pin = *v;
            float min_dist    = (v_max - v_min);

            // 找距離當前值最近的 pin
            for (float pin : pins)
            {
                float dist = std::abs(*v - pin);
                if (dist < min_dist)
                {
                    min_dist    = dist;
                    closest_pin = pin;
                }
            }

            // 距離小於閾值時吸附：snap_threshold 是相對於整個範圍的比例
            float range = v_max - v_min;
            if (min_dist <= (snap_threshold * range))
            {
                *v     = closest_pin;
                changed = true;
            }
        }

        return changed;
    }
}
