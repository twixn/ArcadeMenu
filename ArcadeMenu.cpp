// ArcadeMenu.cpp : Defines the entry point for the console application.
//

#include "ArcadeMenu.h"
#include <stdio.h>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

#include <chrono>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>


void Log(std::string message)
{
	static bool firstrun=true;
	std::ofstream file;
	if(firstrun)
		file.open("Log.txt",std::ios::out);
	else
		file.open("Log.txt",std::ios::app);
	file << /*GetCurrentTimeStr() << " " <<*/ message << "\r\n" << std::endl;
	file.flush();
	file.close();
	firstrun=false;

}

void DebugLog(std::string message)
{
#if defined(DEBUG) || defined(_DEBUG)
   Log("DEBUG MESSAGE: "+message);
#endif
}


std::string Int2Str(int var)
{
	std::stringstream ss;
	std::string str;
	ss << var;
	ss >> str;
	return str;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

bool ArcadeMenu::init()
{
	//Initialization
	Log("Initializing Systems");

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Log(std::string("SDL could not initialize! SDL_Error: ") + SDL_GetError());
		return false;
	}
	else
	{
		if( !init_sound() )
			return false;
		
		if( !create_window())
			return false;
	}

	Log("SDL Initializing Success!");
	return true;
}


bool ArcadeMenu::init_sound()
{
	Log("Initializing sound");
	
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		Log(std::string("SDL could not initialize Sound! SDL_Error: ") + SDL_GetError());
		return false;
	}
	else
	{			
		//Initialize Audio
		initAudio();
		update_sound();
	}
	
	Log("SDL Sound Initializing Success!");
	return true;
}


bool ArcadeMenu::create_window()
{
	Log("Creating Window");
	
	//Create window
	Uint32 flags = SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/ | SDL_WINDOW_BORDERLESS;
#ifndef WIN32
	flags |= SDL_WINDOW_FULLSCREEN;
#endif

	window = SDL_CreateWindow("Arcade Menu", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, flags);
	if (window == NULL)
	{
		Log(std::string("Window could not be created! SDL_Error: ") + SDL_GetError());
		return false;
	}
	else
	{
		//Create renderer for window
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (renderer == NULL)
		{
			Log(std::string("Renderer could not be created! SDL Error: ") + SDL_GetError());
			return false;
		}
		else
		{
			//Initialize renderer color
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		}

		// hide cursor
		SDL_ShowCursor(SDL_DISABLE);
		SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
		
		// Presets
		max_item_pixel_height = max_item_height_percent * SCREEN_HEIGHT;
	}
	
	Log("Window Creation Success");
	return loadMedia();
}

bool ArcadeMenu::destroy_window()
{
	Log("Destroying Window");
	
	for (ArcadeMenuItem item : items)
	{
		SDL_FreeSurface(item.item_surface);
		item.item_surface = NULL;
	}
	items.clear();

	//Deallocate surface
	SDL_FreeSurface(background_surface);
	background_surface = NULL;

	SDL_FreeSurface(volume_up_surface);
	volume_up_surface = NULL;
	SDL_FreeSurface(volume_down_surface);
	volume_down_surface = NULL;
	
    //Free the music
	endAudio();

    //Destroy window    
    SDL_DestroyRenderer( renderer );
	renderer = NULL;
	
	//Destroy window
	SDL_DestroyWindow(window);
	window = NULL;
	
	return true;
}

bool ArcadeMenu::loadMedia()
{
	Log("Loading Media from Config");
	
	// First step is to load the XML config file.
	rapidxml::file<> xmlFile(CONFIG_FILE); // Default template is char
	rapidxml::xml_document<> doc;
	doc.parse<0>(xmlFile.data());

	// Next, we should have a background image in teh XML file.
	// Find and load it.
	std::string background_image_file = doc.first_node("background_image")->value();
	background_surface = SDL_LoadBMP(background_image_file.c_str());
	background_texture = SDL_CreateTextureFromSurface(renderer, background_surface);
	
	// Also, we'll have background noise.
	// Load all the background music one by one.
	background_music_list.clear();
	rapidxml::xml_node<> *musics_node = doc.first_node("background_musics");
	for (rapidxml::xml_node<> *pNode = musics_node->first_node("file"); pNode; pNode = pNode->next_sibling())
	{
		background_music_list.push_back(pNode->value());
	}
	music_id = rand() % background_music_list.size();

	// Load the activation keys, to allow for diff key configs
	activation_keys.clear();
	rapidxml::xml_node<> *keys_node = doc.first_node("activate_keys");
	for (rapidxml::xml_node<> *pNode = keys_node->first_node("key"); pNode; pNode = pNode->next_sibling())
	{
		std::string key = pNode->value();
		activation_keys.push_back(key.size() > 0 ? key[0] : 0);
	}
	
	// Load the selection sound filename.
	select_sound_filename = doc.first_node("select_sound")->value();

	// Load volume up and down buttons.
	std::string volume_up_image_file = doc.first_node("volume_up_image")->value();
	volume_up_surface = SDL_LoadBMP(volume_up_image_file.c_str());
	volume_up_texture = SDL_CreateTextureFromSurface(renderer, volume_up_surface);
	std::string volume_down_image_file = doc.first_node("volume_down_image")->value();
	volume_down_surface = SDL_LoadBMP(volume_down_image_file.c_str());
	volume_down_texture = SDL_CreateTextureFromSurface(renderer, volume_down_surface);

	// Next, go through all of the ArcadeMenu items.
	// Load all their details one by one.
	rapidxml::xml_node<> *items_node = doc.first_node("items");
	for (rapidxml::xml_node<> *pNode = items_node->first_node("item"); pNode; pNode = pNode->next_sibling())
	{
		std::string image = pNode->first_node("image")->value();
		std::string command = pNode->first_node("command")->value();

		// Populate the new item.
		ArcadeMenuItem newItem;
		newItem.name = image;
		newItem.item_surface = SDL_LoadBMP(image.c_str());
		newItem.command = command;

		// Generate any subsequent items from what we have.
		newItem.item_texture = SDL_CreateTextureFromSurface(renderer, newItem.item_surface);
		newItem.aspect = (float)newItem.item_surface->w / (float)newItem.item_surface->h;
		
		// Generate base render information
		newItem.base_pixel_height = max_item_pixel_height;
		newItem.base_pixel_width = max_item_pixel_height * newItem.aspect;
		
		items.push_back(newItem);
	}

	// Pre select the first item if we can.
	if (items.size() > 0)
		items[0].selected = true;
	
	Log(std::string("Media Loading status: Success"));
	return true;
}

void ArcadeMenu::close()
{
	Log("Deinitializing Systems");
	destroy_window();

	//Quit SDL subsystems
	SDL_Quit();
}


int ArcadeMenu::execute_command(std::string command)
{
	Log("Executing Command: " + command);
	return system(command.c_str());
}


void ArcadeMenu::update_sound()
{
	std::string val = Int2Str(master_volume);
	std::string command = ReplaceAll(master_volume_command, master_volume_command_key, val);

	Log("Executing Sound Command: " + command);
	system(command.c_str());
}

bool ArcadeMenu::run()
{
	bool running = true;
	
	std::chrono::time_point<std::chrono::system_clock> last_time = std::chrono::system_clock::now();
	float frame_time = 0;
	float animation_time = 0;
	bool key_down = false;
		
	//Event handler
	SDL_Event e;

	while (running)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				running = false;
			}
			//User presses a key
			else if (e.type == SDL_KEYDOWN)
			{
				if(!key_down)
				{
						
					//Select surfaces based on key press
					int exit_code = 0;
					std::string command = items[selected_id].command;
					
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						break;

					case SDLK_DOWN:
						break;

					case SDLK_LEFT:
						animation_time = 0;
						selected_id--;
						if (selected_id < 0)
							selected_id = 0;
						
						playSound(select_sound_filename.c_str(), sound_volume);
						break;

					case SDLK_RIGHT:
						animation_time = 0;
						selected_id++;
						if (selected_id >= items.size())
							selected_id = items.size() - 1;
						
						playSound(select_sound_filename.c_str(), sound_volume);
						break;

					case SDLK_RETURN:	
						close();	
						exit_code = execute_command(command);	
						init();
						break;

					case SDLK_r: //UP
						master_volume += master_volume_delta;
						if (master_volume > master_volume_max)
							master_volume = master_volume_max;
						update_sound();
						volume_up_change_timer = volume_change_timer_max;
						volume_down_change_timer = -1;
						break;

					case SDLK_f: //DOWN
						master_volume -= master_volume_delta;
						if (master_volume < master_volume_min)
							master_volume = master_volume_min;
						update_sound();
						volume_up_change_timer = -1;
						volume_down_change_timer = volume_change_timer_max;
						break;

					default:
						break;
					}


					for (int i = 0; i < activation_keys.size(); ++i)
					{
						if (activation_keys[i] == e.key.keysym.sym)
						{
							close();
							exit_code = execute_command(command);
							init();
						}
					}

					for (int i = 0; i < items.size(); ++i)
					{
						items[i].selected = i == selected_id;
					}
				}
				
				key_down = true;
			}
			else
			{
				key_down = false;
			}
		}

		// Music test
		if (!musicPlaying() && background_music_list.size()>0)
		{
			playMusic(background_music_list[music_id++].c_str(), sound_volume, 0);

			music_id = music_id % background_music_list.size();
		}

		//Clear screen
		SDL_RenderClear(renderer);

		//Render texture to screen
		SDL_Rect screenrect;
		screenrect.w = SCREEN_WIDTH;
		screenrect.h = SCREEN_HEIGHT;
		screenrect.x = 0;
		screenrect.y = 0;
		SDL_RenderCopy(renderer, background_texture, NULL, &screenrect);

		//Apply the image
		//SDL_BlitScaled(background_surface, NULL, screen_surface, &screen_surface->clip_rect);

		float menu_start_offset = 0;
		SDL_Rect dstrect;

		dstrect.x = screenrect.w / 2;
		for (ArcadeMenuItem item : items)
		{

			if (item.selected)
			{
				menu_start_offset -= (item.base_pixel_width + border_size) / 2;
				break;
			}
			else
			{
				menu_start_offset -= (item.base_pixel_width + border_size) ;
			}
				
		}

		dstrect.x += menu_start_offset;

		for (ArcadeMenuItem item : items)
		{
			dstrect.w = item.base_pixel_width;
			dstrect.h = item.base_pixel_height;
			dstrect.y = screenrect.h / 2 - item.base_pixel_height / 2;

			SDL_Rect final_dstrect = dstrect;

			if (item.selected)
			{
				animation_time += (frame_time * animation_speed);
				int animated_border = border_size + border_size * sin(animation_time);
				final_dstrect.w += animated_border * item.aspect;
				final_dstrect.h += animated_border;
				final_dstrect.x -= animated_border / 2 * item.aspect;
				final_dstrect.y -= animated_border / 2;
			}

			//Apply the image
			//SDL_BlitScaled(item.item_surface, NULL, screen_surface, &final_dstrect);
			SDL_RenderCopy(renderer, item.item_texture, NULL, &final_dstrect);

			dstrect.x += item.base_pixel_width + border_size;
		}

		//Update the surface
		//SDL_UpdateWindowSurface(window);

		SDL_Rect volume_rect;
		volume_rect.w = 128;
		volume_rect.h = 64;
		volume_rect.x = screenrect.w - volume_rect.w;
		volume_rect.y = screenrect.h - volume_rect.h;

		if (volume_up_change_timer > 0)
		{
			volume_up_change_timer -= frame_time;
			SDL_RenderCopy(renderer, volume_up_texture, NULL, &volume_rect);
		}
		if (volume_down_change_timer > 0)
		{
			volume_down_change_timer -= frame_time;
			SDL_RenderCopy(renderer, volume_down_texture, NULL, &volume_rect);
		}

		//Update screen
		SDL_RenderPresent(renderer);

		// FPS counter
		std::chrono::time_point<std::chrono::system_clock> new_time = std::chrono::system_clock::now();

		std::chrono::duration<double> elapsed_time = new_time - last_time;
		frame_time = elapsed_time.count();

		last_time = std::chrono::system_clock::now();
	}

	return true;
}


int main(int argc, char* args[])
{
	ArcadeMenu arcade;

	if (arcade.init())
	{
		arcade.run();

		arcade.close();
	}

	return 0;
}

