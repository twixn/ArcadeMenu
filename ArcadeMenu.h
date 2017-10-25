#ifndef HEADER_ARCADEMENU
#define HEADER_ARCADEMENU

//#define SCREEN_WIDTH 640
//#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define CONFIG_FILE "ArcadeMenuConfig.xml"

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <string>
#include <vector>
#include "audio.h"

struct ArcadeMenuItem
{
	std::string name;
	std::string command;

	//The image we will load and show on the screen
	SDL_Surface* item_surface = NULL;
	SDL_Texture* item_texture = NULL;
	
	// render details
	bool selected = false;
	float aspect = 1;
	int base_pixel_width = 0;
	int base_pixel_height = 0;
};

class ArcadeMenu
{
private:

	// The window we'll be rendering to
	SDL_Window* window = NULL;
	
	// The image we will load and show on the screen
	SDL_Surface* background_surface = NULL;
	SDL_Texture* background_texture = NULL;

	// The image we will load and shown for volume controls
	SDL_Surface* volume_up_surface = NULL;
	SDL_Surface* volume_down_surface = NULL;
	SDL_Texture* volume_up_texture = NULL;
	SDL_Texture* volume_down_texture = NULL;

	// The window renderer
	SDL_Renderer* renderer = NULL;
	
	// Arcade cabinet items
	std::vector<ArcadeMenuItem> items;
	int selected_id = 0;
	
	// Menu control 
	int max_item_pixel_height = 0;
	const float max_item_height_percent = 0.666f;
	const int border_size = 64;
	const float animation_speed = 5.0f;
	std::vector<SDL_Keycode> activation_keys;

	// Music list
	std::vector<std::string> background_music_list;
	int music_id = 0;
	
	// Sound information
	std::string select_sound_filename;
	const int sound_volume = SDL_MIX_MAXVOLUME / 2;
	int master_volume = 50;
	const int master_volume_delta = 10;
	const int master_volume_min = master_volume_delta;
	const int master_volume_max = 100;
	std::string master_volume_command = "amixer -D pulse sset Master <vol>%";
	std::string master_volume_command_key = "<vol>";
	float volume_up_change_timer = -1.0f;
	float volume_down_change_timer = -1.0f;
	const float volume_change_timer_max = 1.0f;
	
public:

	//Starts up SDL and creates window
	bool init();
	bool init_sound();
	bool create_window();
	bool destroy_window();

	//Loads media
	bool loadMedia();

	//Execute Menu
	bool run();

	//Frees media and shuts down SDL
	void close();
	
	// Executes the command for the menu item
	int execute_command(std::string command);
	void update_sound();
};


#endif //HEADER_ARCADEMENU
