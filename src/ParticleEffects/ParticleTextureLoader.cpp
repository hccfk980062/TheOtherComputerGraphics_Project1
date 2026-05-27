#include "ParticleEffects/ParticleTextureLoader.h"

// stb_image declarations only (STB_IMAGE_IMPLEMENTATION is compiled once in ModelLoader.cpp)
#include "stb_image.h"

#include <unordered_map>
#include <cstdio>

namespace CG
{
    // ── Per-session texture cache ─────────────────────────────────────────────
    static std::unordered_map<std::string, GLuint> s_cache;

    GLuint LoadParticleTexture(const std::string& path)
    {
        if (path.empty()) return 0;

        // Return cached ID if already uploaded
        auto it = s_cache.find(path);
        if (it != s_cache.end())
            return it->second;

        stbi_set_flip_vertically_on_load(false);  // billboard UVs are already correct
        int w = 0, h = 0, ch = 0;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (!data)
        {
            fprintf(stderr, "[ParticleTextureLoader] Failed to load: %s\n", path.c_str());
            return 0;
        }

        GLenum fmt = GL_RGBA;
        if      (ch == 1) fmt = GL_RED;
        else if (ch == 3) fmt = GL_RGB;
        else if (ch == 4) fmt = GL_RGBA;

        GLuint id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
        s_cache[path] = id;
        return id;
    }
}
