#pragma once

#include "Shader/Shader.h"
#include "Scene/MainScene.h"
#include "FrameBuffer/Framebuffer.h"

namespace CG
{
	class SceneRenderer
	{
	private:
		GLenum mode = GL_FILL; // GL_FILL or GL_LINE

		Shader* defaultShader;
		Shader* shaderProgram_worldObject;
		Framebuffer* framebuffer;
	public:

		auto Initialize(int width, int height) -> bool;

		void RenderScene(MainScene *scene);
		Framebuffer* getCurrentFramebuffer();
	};
}