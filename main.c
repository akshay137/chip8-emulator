#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "chip8/cpu.h"

int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("%s\n", SDL_GetError());
		return -1;
	}
	
	int w = 640 / 1;
	int h = 320 / 1;
	SDL_Window* win = SDL_CreateWindow(args[1], 0, 0, w, h, 0);
	if (win == NULL)
	{
		printf("%s\n", SDL_GetError());
	}
	
	SDL_Surface* screen = SDL_GetWindowSurface(win);
	
	struct chip8_system c8;
	chip8_reset(&c8);
	chip8_load_rom_file(&c8, args[1]);
	
	SDL_Rect pix = {0, 0, w / 64, h / 32};
	
	int running = 1;
	int status = 1;
	while (running && status)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = 0;
			}
			
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE: running = 0; break;
					
					case SDLK_1: c8.key[0] = 1; break;
					case SDLK_2: c8.key[1] = 1; break;
					case SDLK_3: c8.key[2] = 1; break;
					case SDLK_4: c8.key[3] = 1; break;
					
					case SDLK_q: c8.key[4] = 1; break;
					case SDLK_w: c8.key[5] = 1; break;
					case SDLK_e: c8.key[6] = 1; break;
					case SDLK_r: c8.key[7] = 1; break;
					
					case SDLK_a: c8.key[8] = 1; break;
					case SDLK_s: c8.key[9] = 1; break;
					case SDLK_d: c8.key[10] = 1; break;
					case SDLK_f: c8.key[11] = 1; break;
					
					case SDLK_z: c8.key[12] = 1; break;
					case SDLK_x: c8.key[13] = 1; break;
					case SDLK_c: c8.key[14] = 1; break;
					case SDLK_v: c8.key[15] = 1; break;
				}
			}
			if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_1: c8.key[0] = 0; break;
					case SDLK_2: c8.key[1] = 0; break;
					case SDLK_3: c8.key[2] = 0; break;
					case SDLK_4: c8.key[3] = 0; break;
					
					case SDLK_q: c8.key[4] = 0; break;
					case SDLK_w: c8.key[5] = 0; break;
					case SDLK_e: c8.key[6] = 0; break;
					case SDLK_r: c8.key[7] = 0; break;
					
					case SDLK_a: c8.key[8] = 0; break;
					case SDLK_s: c8.key[9] = 0; break;
					case SDLK_d: c8.key[10] = 0; break;
					case SDLK_f: c8.key[11] = 0; break;
					
					case SDLK_z: c8.key[12] = 0; break;
					case SDLK_x: c8.key[13] = 0; break;
					case SDLK_c: c8.key[14] = 0; break;
					case SDLK_v: c8.key[15] = 0; break;
				}
			}
		}
		
		status = chip8_tick(&c8);
		
		//SDL_FillRect(screen, NULL, 0);
		for (int i = 0; i < 32; i++)
		{
			pix.y = i * pix.h;
			for (int j = 0; j < 64; j++)
			{
				pix.x = j * pix.w;
				uint32_t c = c8.display[(i * 64) + j];
				c == 0 ? (c = 0) : (c = -1);
				SDL_FillRect(screen, &pix, c);
			}
		}
		SDL_UpdateWindowSurface(win);
		SDL_Delay(1);
	}
	
	//chip8_dump(&c8);
	
	putc('\n', stdout);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
