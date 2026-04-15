#pragma once

#include "FrameBuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "Scene/MainScene.h"

namespace CG
{
	class SceneRenderer
	{
	private:
		GLenum mode = GL_FILL; // GL_FILL or GL_LINE

		Shader* shaderProgram_particle;
		Shader* shaderProgram_trail;
		Shader* shaderProgram_worldObject;
		Framebuffer* viewportFramebuffer;
	public:

		auto Initialize(int width, int height) -> bool;

		void RenderScene(MainScene *scene);
		Framebuffer* getCurrentViewportFramebuffer();
	};
}