#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

// Based on code from lecture/Discord by Jim McCann

//Shader program that draws transformed, lit, textured vertices tinted with vertex colors:
struct LitColorTextureProgram {
	LitColorTextureProgram();
	~LitColorTextureProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint Position_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	//Uniform (per-invocation variable) locations:
	GLuint CLIP_FROM_LOCAL_mat4 = -1U;

	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< LitColorTextureProgram > texture_program;

//For convenient scene-graph setup, copy this object:
// NOTE: by default, has texture bound to 1-pixel white texture -- so it's okay to use with vertex-color-only meshes.
extern Scene::Drawable::Pipeline lit_color_texture_program_pipeline;
