#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>

#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include <iostream>
#include FT_FREETYPE_H

// Based on code from lecture/Discord by Jim McCann
struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	struct Character {
        unsigned int TextureID;  // ID handle of the glyph texture
        glm::ivec2   Size;       // Size of glyph
        glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
        FT_Pos Advance;    // Offset to advance to next glyph
    };

	std::map<char, Character> Characters;
	float x = 0.0f;
	float y = 0.0f;

	unsigned int VAO, VBO;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	char *text = "You stand in the middle of the dungeon.\n"
		"A single beam of sunlight pours from a small crack in the ceiling high above,\n"
		"illuminating a circular room lined with ancient stone sarcophagi.\n"
		"There are doorways in each of the four cardinal directions.\n"
		"Move with WASD.";
	glm::vec2 position;
	std::map<float, char *> text_repo;

};
