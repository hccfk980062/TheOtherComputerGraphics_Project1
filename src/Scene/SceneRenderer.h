#pragma once
#include <memory>

#include "FrameBuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "Scene/MainScene.h"

namespace CG
{
	class SceneRenderer
	{
	private:
		GLenum mode = GL_FILL; // GL_FILL or GL_LINE

		std::unique_ptr<Shader> shaderProgram_particle;
		std::unique_ptr<Shader> shaderProgram_trail;
		std::unique_ptr<Shader> shaderProgram_worldObject;
		std::unique_ptr<Framebuffer> viewportFramebuffer;
	public:

		auto Initialize(int width, int height) -> bool;

		void RenderScene(MainScene *scene);
		Framebuffer* getCurrentViewportFramebuffer();
	};
}