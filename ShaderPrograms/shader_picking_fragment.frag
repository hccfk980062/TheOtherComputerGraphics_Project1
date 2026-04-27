#version 330 core
out vec4 fragColor;

// 將物件 ID 編碼為 24 位元 RGB 顏色（R<<16 | G<<8 | B）
// 背景清除色為純白（0xFFFFFF），此值在 CPU 端解碼後視為無效 ID（無物件）
uniform int objectID;

void main()
{
    // 從 int 拆解出三個 8-bit 通道，轉換為 0.0~1.0 的 float
    float r = float((objectID >> 16) & 0xFF) / 255.0;
    float g = float((objectID >> 8)  & 0xFF) / 255.0;
    float b = float( objectID        & 0xFF) / 255.0;
    fragColor = vec4(r, g, b, 1.0);
}
