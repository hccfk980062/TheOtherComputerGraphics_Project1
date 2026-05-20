#version 460 core

in  vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_screenTexture;
uniform int       u_pixelSize;   // mosaic block size in pixels (1 = passthrough)
uniform vec2      u_resolution;  // framebuffer size in pixels

void main()
{
    if (u_pixelSize <= 1)
    {
        FragColor = texture(u_screenTexture, TexCoords);
        return;
    }

    // Snap the pixel coordinate to the center of its mosaic block
    vec2 pixelCoord = TexCoords * u_resolution;
    vec2 snapped    = floor(pixelCoord / float(u_pixelSize)) * float(u_pixelSize)
                      + float(u_pixelSize) * 0.5;
    FragColor = texture(u_screenTexture, snapped / u_resolution);
}
