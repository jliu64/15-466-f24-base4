#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <string>

#define FONT_SIZE 12
#define MARGIN (FONT_SIZE * .5)

// Based off of code from lecture/Discord by Jim McCann
// and the Harfbuzz tutorial (https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c)
// and the Freetype tutorial (https://freetype.org/freetype2/docs/tutorial/step1.html)
// and the OpenGL text rendering tutorial (https://learnopengl.com/In-Practice/Text-Rendering)

PlayMode::PlayMode() {
	// Shape using Harfbuzz
    FT_Library ft_library;
    FT_Face ft_face;
    FT_Error ft_error;

    ft_error = FT_Init_FreeType(&ft_library);
    //if (ft_error != 0) abort();
    ft_error = FT_New_Face(ft_library, "dist/GentiumBookPlus-Regular.ttf", 0, &ft_face);
    //if (ft_error != 0) abort();
    //ft_error = FT_Set_Char_Size(ft_face, 0, FONT_SIZE*64, 300, 300);
    //if (ft_error != 0) abort();
	FT_Set_Pixel_Sizes(ft_face, 0, 24);

    hb_font_t *hb_font;
    hb_font = hb_ft_font_create(ft_face, NULL);

    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create();
    hb_buffer_add_utf8(hb_buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);

    hb_shape(hb_font, hb_buffer, NULL, 0);

    unsigned int len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    // Print raw buffer contents
    printf ("Raw buffer contents:\n");
    for (unsigned int i = 0; i < len; i++)
    {
        hb_codepoint_t gid   = info[i].codepoint;
        unsigned int cluster = info[i].cluster;
        double x_advance = pos[i].x_advance / 64.;
        double y_advance = pos[i].y_advance / 64.;
        double x_offset  = pos[i].x_offset / 64.;
        double y_offset  = pos[i].y_offset / 64.;

        char glyphname[32];
        hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

        printf ("glyph='%s'	cluster=%d	advance=(%g,%g)	offset=(%g,%g)\n",
            glyphname, cluster, x_advance, y_advance, x_offset, y_offset);
    }

    printf ("Absolute positions:\n");
    {
        double current_x = 0;
        double current_y = 0;
        for (unsigned int i = 0; i < len; i++)
        {
            hb_codepoint_t gid   = info[i].codepoint;
            unsigned int cluster = info[i].cluster;
            double x_position = current_x + pos[i].x_offset / 64.;
            double y_position = current_y + pos[i].y_offset / 64.;


            char glyphname[32];
            hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

            printf ("glyph='%s'	cluster=%d	position=(%g,%g)\n",
                glyphname, cluster, x_position, y_position);

            current_x += pos[i].x_advance / 64.;
            current_y += pos[i].y_advance / 64.;
        }
    }

    // Set up OpenGL textures and render using Freetype
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
    glUseProgram(texture_program->program);
    glUniformMatrix4fv(texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, glm::value_ptr(projection));

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

	// Create textures for common characters
    for (unsigned char c = 0; c < 128; c++) {
        // load character glyph 
        if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
        {
            std::cout << "Error: Failed to load glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            ft_face->glyph->bitmap.width,
            ft_face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ft_face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture, 
            glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
            glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top),
            ft_face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GL_ERRORS();

	// Initialize game text and state
	position.x = 0.0f;
	position.y = 0.0f;
	text_repo.insert(std::pair<float, char *>(0.0f, "You stand in the middle of the dungeon.\n"
		"A single beam of sunlight pours from a small crack in the ceiling high above,\n"
		"illuminating a circular room lined with ancient stone sarcophagi.\n"
		"There are doorways in each of the four cardinal directions.\n"
		"Move with WASD."));
	text_repo.insert(std::pair<float, char *>(10.0f, "You are north of your starting position.\n"
		"It's a dead end.\n"
		"TODO: Make an actual game"));
	text_repo.insert(std::pair<float, char *>(-10.0f, "You are south of your starting position.\n"
		"It's a dead end.\n"
		"TODO: Make an actual game"));
	text_repo.insert(std::pair<float, char *>(1.0f, "You are east of your starting position.\n"
		"It's a dead end.\n"
		"TODO: Make an actual game"));
	text_repo.insert(std::pair<float, char *>(-1.0f, "You are west of your starting position.\n"
		"It's a dead end.\n"
		"TODO: Make an actual game"));
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			exit(0);
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}
	return false;
}

void PlayMode::update(float elapsed) {
	if (position.y == 0.0f) {
		position.x -= left.downs;
		position.x += right.downs;
		if (position.x < 0.0f) position.x = -1.0f;
		if (position.x > 0.0f) position.x = 1.0f;
	}
	if (position.x == 0.0f) {
		position.y -= down.downs;
		position.y += up.downs;
		if (position.y < 0.0f) position.y = -1.0f;
		if (position.y > 0.0f) position.y = 1.0f;
	}

	float pos_index = position.x + 10 * position.y;
	text = text_repo[pos_index];

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	x = 20.0f;
	y = 550.0f;
	glClearColor(0.12f, 0.12f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	// activate corresponding render state	
    glUseProgram(texture_program->program);
	//glUniformMatrix4fv(texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform3f(glGetUniformLocation(texture_program->program, "textColor"), 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
	std::string str_text = text;
    std::string::const_iterator c;
	int line = 0;
    for (c = str_text.begin(); c != str_text.end(); c++)
    {
        Character ch = Characters[*c];
		if (ch.TextureID == 11) {
			line++;
			x = 20.0f;
			y = 550.0f - 25 * line;
			continue;
		}

        float xpos = x + ch.Bearing.x;
        float ypos = y - (ch.Size.y - ch.Bearing.y);

        float w = (float) ch.Size.x;
        float h = (float) ch.Size.y;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();
}
