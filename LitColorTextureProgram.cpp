#include "LitColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

// Based on code from lecture/Discord by Jim McCann

Scene::Drawable::Pipeline lit_color_texture_program_pipeline;

Load< LitColorTextureProgram > texture_program(LoadTagEarly, []() -> LitColorTextureProgram const * {
	LitColorTextureProgram *ret = new LitColorTextureProgram();
	return ret;
});

LitColorTextureProgram::LitColorTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
		"layout (location = 0) in vec4 vertex;\n"
		"out vec2 TexCoords;\n"
		"uniform mat4 projection;\n"
		"void main() {\n"
		"	gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
		"	TexCoords = vertex.zw;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"uniform sampler2D text;\n"
		"uniform vec3 textColor;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
		"	color = vec4(textColor, 1.0) * sampled;\n"
		"}\n"
	);

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "vertex");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoords");

	//look up the locations of uniforms:
	CLIP_FROM_LOCAL_mat4 = glGetUniformLocation(program, "projection");

	GLuint TEX_sampler2D = glGetUniformLocation(program, "text");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now

}

LitColorTextureProgram::~LitColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

