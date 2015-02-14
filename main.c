/*
 * main.c
 *
 *  Created on: 14. feb. 2015
 *      Author: Oskar
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "HackCPU.h"

#include "SDL/SDL.h"

#define RAMLENGTH 32*1024
#define ROMLENGTH 32*1024
#define SCREENOFFSET 0x4000
#define SCREENLENGTH 8*1024
#define KEYBOARDOFFSET 0x6000

int16_t ram[RAMLENGTH];
int16_t rom[ROMLENGTH];
uint16_t programEnd;

int main(int argc, char* argv[])
{
	SDL_Surface* screen;
	SDL_Event event;
	int16_t key = 0;

	// Initialize the SDL library
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return 1;
	}


	atexit(SDL_Quit); // Clean up on exit

	screen = SDL_SetVideoMode(512, 256, 8, SDL_SWSURFACE); // Initialize the display in a 512x256 1-bit palettized mode, requesting a software surface
	if (screen == NULL)
	{
		fprintf(stderr, "Couldn't set SDL video mode: %s\n", SDL_GetError());
		return 2;
	}

	SDL_EnableUNICODE(1);

	bool quit = false;
	while(!quit)
	{
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYUP:
				key = 0;
				break;
			case SDL_KEYDOWN:
				key = (int16_t)event.key.keysym.sym;
				break;
			}
		}

		size_t count = 0;
		do
		{
			programEnd += count;
			count = fread(rom + programEnd, 2, 1, stdin);
		} while (count > 0 && programEnd < ROMLENGTH);

		static struct Outputs out = {};
		int16_t inM = 0;

		// read (and write if writeM is true) data memory
		if (out.addressM < SCREENOFFSET)
		{
			if (out.writeM) ram[out.addressM] = out.outM;
			inM = ram[out.addressM];
		}
		// read (and write if writeM is true) screen memory
		else if (out.addressM < KEYBOARDOFFSET)
		{
			if (SDL_MUSTLOCK(screen)) if (SDL_LockSurface(screen) < 0) {
				fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
				return 3;
			}

			if (out.writeM) ((int16_t*)screen->pixels)[out.addressM - SCREENOFFSET] = out.outM;
			inM = ((int16_t*)screen->pixels)[out.addressM - SCREENOFFSET];

			if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
			SDL_UpdateRect(screen, 0, 0, 512, 256);
		}
		// read keyboard memory
		else if (out.addressM == KEYBOARDOFFSET)
		{
			inM = key;
		}

		out = emulate(inM, rom[out.pc], false);
	}

	fwrite(ram, 2, RAMLENGTH, stdout);
	fwrite(screen->pixels, 2, SCREENLENGTH, stdout);
	fwrite(&key, 2, 1, stdout);

	SDL_FreeSurface(screen);

	return 0;
}
