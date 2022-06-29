/////////////////////////////////////////////////////////////////////////////////////////
// Comp 369
// Assignment 3
// allegro-4.4.2-monolith-md-debug.lib
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "allegro.h"
#include "mappyal.h"

#define MODE GFX_AUTODETECT_WINDOWED
#define WIDTH 800
#define HEIGHT 600

#define GREEN makecol(0,128,0)
#define WHITE makecol(255,255,255)
#define BLACK makecol(0,0,0)
#define NUMSPRITES 75
#define BOTTOM 32000 - HEIGHT

int ret;
BITMAP *explode;
BITMAP *title;
BITMAP *player;
BITMAP *laser;
MIDI *music;
BITMAP *buffer;
BITMAP *asteroid;
BITMAP *spriteimg[64];

int music_playing = FALSE;
int game_running = FALSE;
int game_paused = FALSE;
int i;
int angle;
int laser_alive = FALSE;

int yoffset = BOTTOM;

int counter;
int score = 5;
int start;

class Sprite {
public:
	int rotation;
	int x, y;
	int width, height;
	int curframe, maxframe; 
	double yspeed;
	int framecount, framedelay;

	Sprite() {}

	Sprite(int a, int b, int w, int h, double s)
	{
		x = a;
		y = b;
		width = w;
		height = h;
		yspeed = s;

	}

};


Sprite player_sprite;

Sprite laser_sprite;

Sprite sprites[NUMSPRITES];



BITMAP *grabframe(BITMAP *source,
	int width, int height,
	int startx, int starty,
	int columns, int frame)
{
	BITMAP *temp = create_bitmap(width, height);

	int x = startx + (frame % columns) * width;
	int y = starty + (frame / columns) * height;

	blit(source, temp, x, y, 0, 0, width, height);

	return temp;
}

//Displays game name
void title_screen()
{
	title = load_bitmap("title_screen.bmp", NULL);
	blit(title, screen, 0, 0, 0, 0, title->w, title->h);
	destroy_bitmap(title);
	textprintf_ex(screen, font, 0, 0, 1, -1, "%dx%d", SCREEN_W, SCREEN_H);

	rest(2000);
}

void info_screen()
{
	clear_bitmap(screen);
	textout_centre_ex(screen, font, "About:", 400, 225, WHITE, 0);
	textout_centre_ex(screen, font, "You are traveling through space and come upon an asteroid field", 400, 250, WHITE, 0);
	textout_centre_ex(screen, font, "Destroy or dodge asteroids for 2 minutes to make it through alive.", 400, 275, WHITE, 0);
	textout_centre_ex(screen, font, "Press ctrl + m to pause/unpause the music", 400, 350, WHITE, 0);
	textout_centre_ex(screen, font, "Press ctrl + h to bring up the controls", 400, 375, WHITE, 0);
	textout_centre_ex(screen, font, "The arrow keys will control the ship and space will fire your laser", 400, 400, WHITE, 0);
	textout_centre_ex(screen, font, "(Hit ESC at any time to quit)", 400, 425, WHITE, 0);
	textout_centre_ex(screen, font, "Press any key to continue...", 400, 550, WHITE, 0);

}

void help_screen()
{
	game_running = FALSE;
	game_paused = TRUE;
	clear_bitmap(screen);
	clear_bitmap(buffer);
	textout_centre_ex(screen, font, "You have 5 lives", 400, 275, GREEN, 0);
	textout_centre_ex(screen, font, "Press ctrl + m to pause/unpause the music", 400, 300, GREEN, 0);
	textout_centre_ex(screen, font, "Press ctrl + h to bring up the controls", 400, 325, GREEN, 0);
	textout_centre_ex(screen, font, "The arrow keys will control the player and space fires the gun", 400, 350, GREEN, 0);
	textout_centre_ex(screen, font, "(Press ctrl + r to resume)", 400, 400, GREEN, 0);

}

void end_screen()
{
	clear_bitmap(buffer);
	if (score == 0)
	{
		textprintf_centre_ex(buffer, font, 400, 250, WHITE, 0, "Game Over");
		textprintf_centre_ex(buffer, font, 400, 300, WHITE, 0,
			"You survived: %d seconds", counter);
	}
	else
	{
		textprintf_centre_ex(buffer, font, 400, 300, WHITE, 0, "Well done you made it!");
	}
	textprintf_centre_ex(buffer, font, 400, 350, WHITE, 0, "Thanks for playing!");
	textprintf_centre_ex(buffer, font, 400, 400, WHITE, 0, "(Press ESC to Exit)");
	blit(buffer, screen, 0, 0, 0, 0, SCREEN_W - 1, SCREEN_H - 1);
}

int inside(int x, int y, int left, int top, int right, int bottom)
{
	if (x > left && x < right && y > top && y < bottom)
		return 1;
	else
		return 0;
}

int collided(Sprite first, Sprite second, int border)
{
	//get width/height of both sprites
	int width1 = first.x + first.width;
	int height1 = first.y + first.height;
	int width2 = second.x + second.width;
	int height2 = second.y + second.height;

	//see if corners of first are inside second boundary
	if (inside(first.x, first.y, second.x + border,
		second.y + border, width2 - border, height2 - border))
		return 1;
	if (inside(first.x, height1, second.x + border,
		second.y + border, width2 - border, height2 - border))
		return 1;
	if (inside(width1, first.y, second.x + border,
		second.y + border, width2 - border, height2 - border))
		return 1;
	if (inside(width1, height1, second.x + border,
		second.y + border, width2 - border, height2 - border))
		return 1;

	//no collisions?
	return 0;
}

void start_music()
{
	midi_resume();
	music_playing = TRUE;
}

void stop_music()
{
	midi_pause();
	music_playing = FALSE;
	rest(10);
}

void loadsprite(void)
{
	BITMAP *temp;
	temp = load_bitmap("asteroid.bmp", NULL);
	for (i = 0; i < 64; i++)
	{
		spriteimg[i] = grabframe(temp, 64, 64, 0, 0, 8, i);
	}

	for (i = 0; i < NUMSPRITES; i++)
	{
		sprites[i] = Sprite(rand() % 700, (rand() % 2500) -2500, spriteimg[0]->w, spriteimg[0]->h, rand() % 4 + 3);
		sprites[i].curframe = rand() % 64;
		sprites[i].framecount = 0;
		sprites[i].framedelay = rand() % 5 + 1;
	}

	player_sprite = Sprite(SCREEN_W / 2 - player->w / 2, SCREEN_H - (player->h *2), player->w, player->h, 0);

	laser_sprite = Sprite(player_sprite.x, player_sprite.y, laser->w, laser->h, 0);

	destroy_bitmap(temp);
}

void erasesprite(BITMAP *dest, Sprite spr)
{
	//erase the sprite
	blit(buffer, dest, spr.x, spr.y, spr.x, spr.y,
		spr.width, spr.height);
}



int main(void)
{
	allegro_init();
	install_timer();
	install_keyboard();
	srand(time(NULL));
	set_color_depth(16);

	ret = set_gfx_mode(MODE, WIDTH, HEIGHT, 0, 0);
	if (ret != 0) {
		allegro_message(allegro_error);
		return 1;
	}
	install_timer();
	install_keyboard();

	//load the Mappy file
	if (MapLoad("space-BG.fmp") != 0)
	{
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Can't find space-BG.fmp");
		return 1;
	}

	//Music
	if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
		allegro_message("Error initialising sound system\n%s\n", allegro_error);
		return 1;
	}

	/* read in the MIDI file */
	music = load_midi("mm2-wilystage.mid");
	if (!music) {
		allegro_message("Error loading Midi file");
		return 1;
	}

	if (play_midi(music, -1) != 0) {
		allegro_message("Error playing Midi\n%s", allegro_error);
		return 1;
	}
	music_playing = TRUE;

	buffer = create_bitmap(SCREEN_W, SCREEN_H);
	clear(buffer);

	//load sprites
	player = load_bitmap("ship_sprite.bmp", NULL);
	laser = load_bitmap("laser.bmp", NULL);
	explode = load_bitmap("explode.bmp", NULL);

	title_screen();

	info_screen();

	while (!keypressed());
	clear_bitmap(screen);
	game_running = TRUE;
	loadsprite();

	//draw map with single layer
	MapDrawBG(buffer, 0, yoffset, 0, 0, SCREEN_W - 1, SCREEN_H - 1);

	//angle = rand() % 350 + 1;
	blit(buffer, screen, 0, 0, 0, 0, SCREEN_W - 1, SCREEN_H - 1);


	//main loop
	while (!key[KEY_ESC])
	{
		if (score == 0)
		{
			game_running = FALSE;
			end_screen();
		}
		if (counter >= 120)
		{
			game_running = FALSE;
			end_screen();
		}
		if (key[KEY_M] && key[KEY_LCONTROL])
		{
			if (music_playing == TRUE)
			{
				stop_music();
			}
			else
			{
				start_music();
			}
		}
		if (key[KEY_LCONTROL] && key[KEY_H])
		{
			help_screen();

		}
		if (key[KEY_LCONTROL] && key[KEY_R] && game_paused == TRUE)
		{
			while (!keypressed());
			clear_bitmap(screen);
			game_paused = FALSE;
			game_running = TRUE;
		}
		if (game_running == TRUE)
		{

			//Draw Map
			if (yoffset > 0)
				yoffset -= 3;
			MapDrawBG(buffer, 0, yoffset, 0, 0, SCREEN_W - 1, SCREEN_H - 1);


			//Draw Player sprite
			erasesprite(buffer, player_sprite);
			draw_sprite(buffer, player, player_sprite.x, player_sprite.y);

			if (key[KEY_SPACE] && laser_alive == FALSE)
			{
				laser_sprite.x = player_sprite.x + (player_sprite.width / 2) - 6;
				laser_sprite.y = player_sprite.y - 25;
				draw_sprite(buffer, laser, laser_sprite.x, laser_sprite.y);
				laser_alive = TRUE;
			}

			if (laser_alive == TRUE)
			{
				erasesprite(buffer, laser_sprite);
				laser_sprite.y -= 20;
				draw_sprite(buffer, laser, laser_sprite.x, laser_sprite.y);
				if (laser_sprite.y <= -50)
					laser_alive = FALSE;
			}
			if (laser_alive == FALSE)
			{
				laser_sprite.y = NULL; 
				laser_sprite.x = NULL;
			}
				
			//Clear sprites
			for (i = 0; i < NUMSPRITES; i++)
			{
				erasesprite(buffer, sprites[i]);
			}

			//Draw asteroids
			for (i = 0; i < NUMSPRITES; i++)
			{
				draw_sprite(buffer, spriteimg[sprites[i].curframe], sprites[i].x, sprites[i].y);
				if (sprites[i].framecount++ > sprites[i].framedelay){
					sprites[i].framecount = 0;
					if (sprites[i].curframe <= 0)
						sprites[i].curframe = 63;
					else
						sprites[i].curframe -= 1;
				}
			}

			for (i = 0; i < NUMSPRITES; i++)
			{
				if (collided(laser_sprite, sprites[i], 0))
				{
					erasesprite(buffer, sprites[i]);
					erasesprite(buffer, laser_sprite);
					draw_sprite(buffer, explode, sprites[i].x + 15, sprites[i].y + 15);
					laser_alive = FALSE;
					sprites[i].y = -5000;
				}
			}

			for (i = 0; i < NUMSPRITES; i++)
			{
				if (collided(sprites[i], player_sprite, 0))
				{
					erasesprite(buffer, sprites[i]);
					erasesprite(buffer, player_sprite);
					stretch_sprite(buffer, explode, player_sprite.x - 10, player_sprite.y - 10, explode->w * 3, explode->h * 3);
					score--;
					sprites[i].y = -2500;
				}
			}

			if (clock() > start + 1000)
			{
				counter++;
				start = clock();
			}

			textprintf_centre_ex(buffer, font, 200, 15, WHITE, 0,
				"Time: %d", counter);
			textprintf_centre_ex(buffer, font, 500, 15, WHITE, 0,
				"Lives: %d", score);


			if (key[KEY_LEFT] && (player_sprite.x > 10))
			{
				player_sprite.x = player_sprite.x - 4;
			}
			if (key[KEY_RIGHT] && (player_sprite.x < 725))
			{
				player_sprite.x = player_sprite.x + 4;
			}

			//move sprites
			for (i = 0; i < NUMSPRITES; i++)
			{
				sprites[i].y += sprites[i].yspeed;
				if (sprites[i].y >= 1000)
				{
					sprites[i].y = -2500;
				}
			}


			acquire_screen();
			blit(buffer, screen, 0, 0, 0, 0, SCREEN_W - 1, SCREEN_H - 1);
			release_screen();
			rest(10);
		}


	}

	stop_midi();
	MapFreeMem();
	destroy_midi(music);
	destroy_bitmap(player);
	remove_sound();
	allegro_exit();
	return 0;
}
END_OF_MAIN()