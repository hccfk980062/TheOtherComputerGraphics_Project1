#pragma once
#include <string>
#include <GL/glew.h>

namespace CG
{
    /// Load an image file (PNG/JPG/JPEG/BMP/TGA) and upload it to GPU.
    /// Results are cached by absolute path — repeated calls with the same path
    /// return the cached GL texture ID immediately without re-reading the file.
    ///
    /// @param path  Absolute (or CWD-relative) path to the image file.
    /// @return      Non-zero GL texture ID on success; 0 on failure.
    GLuint LoadParticleTexture(const std::string& path);
}
