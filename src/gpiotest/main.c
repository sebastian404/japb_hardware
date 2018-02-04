/*******************************************************************************
Project  : japb_hardware
Module   : main.c
Version  : 0.1
Date     : 2017-08-06
Authors  : Sebastain Robinson
Company  :
Comments : GPIO tester
*******************************************************************************/

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <SDL/SDL.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <a13_gpio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// SDL colors
SDL_Color white = {255, 255, 255};
SDL_Color cyan = {0, 255, 255};

//Global variables
int t, tl = 0, frequency = 1000 / 100, temp;
int repeat, i, quit = 0;
int playing = 1;
int pin;
char text[80];
char message[80];
SDL_Event event;
SDL_Surface *screen, *textSurface;
SDL_Rect rect;
SDL_Surface *bmpfont;
SDL_Rect area;

Uint32 get_pixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
  case 1:
    return *p;

  case 2:
    return *(Uint16 *)p;

  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;

  case 4:
    return *(Uint32 *)p;

  default:
    return 0;
  }
}

void put_pixel(SDL_Surface *_ima, int x, int y, Uint32 pixel)
{
  int bpp = _ima->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)_ima->pixels + y * _ima->pitch + x * bpp;

  switch (bpp)
  {
  case 1:
    *p = pixel;
    break;

  case 2:
    *(Uint16 *)p = pixel;
    break;

  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
    {
      p[0] = (pixel >> 16) & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = pixel & 0xff;
    }
    else
    {
      p[0] = pixel & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = (pixel >> 16) & 0xff;
    }
    break;

  case 4:
    *(Uint32 *) p = pixel;
    break;
  }
}

void replaceColor (SDL_Surface * src, Uint32 target, Uint32 replacement, SDL_Rect pick)
{
  int x;
  int y;

  if (SDL_MUSTLOCK (src))
  {
    if (SDL_LockSurface (src) < 0)
      return;
  }

  for (x = pick.x; x < (pick.x + pick.w); x ++)
  {
    for (y = pick.y; y < (pick.y + pick.h); y ++)
    {
      if (get_pixel (src, x, y) == target)
        put_pixel (src, x, y, replacement);
    }
  }

  if (SDL_MUSTLOCK (src))
    SDL_UnlockSurface (src);
}

void DrawChar(SDL_Surface *screen, SDL_Surface *bmpfont, int X, int Y, int w, int h, int asciicode, SDL_Color color)
{
  SDL_Rect pick;

  pick.x = (asciicode % 32) * w;
  pick.y = (asciicode / 32) * h;
  pick.w = w;
  pick.h = h;
  area.x = X;
  area.y = Y;
  area.w = w;
  area.h = h;

  replaceColor (bmpfont, SDL_MapRGB (bmpfont->format, 255, 255, 255), SDL_MapRGB (bmpfont->format, color.r, color.g, color.b), pick);

  SDL_BlitSurface(bmpfont, &pick, screen, &area);
}

void Drawstring(SDL_Surface *screen, int X, int Y, char text[], SDL_Color color)
{
  int i = 0;
  int asciicode;
  area.x = X;
  area.y = Y;

  for (i = 0; i < 80; i++) {
    asciicode = text[i];
    if (asciicode == 0 ) {
      break;
    }
    DrawChar(screen, bmpfont, area.x, area.y, 8, 16, asciicode, color);
    area.x = area.x + 8;
  }
}

void DrawRect(int x, int y, int width, int height, int color)
{
  rect.x = x;
  rect.y = y;
  rect.w = width;
  rect.h = height;
  SDL_FillRect(screen, &rect, color);
}

int fps_sync (void)
{
  t = SDL_GetTicks ();

  if (t - tl >= frequency)
  {
    temp = (t - tl) / frequency;
    tl += temp * frequency;
    return temp;
  }
  else
  {
    SDL_Delay (frequency - (t - tl));
    tl += frequency;
    return 1;
  }
}

int mainLoop()
{
  while (!quit)
  {
    SDL_PollEvent(&event);
    repeat = fps_sync ();
    for (i = 0; i < repeat; i ++)
    {
      if (event.type == SDL_QUIT)
        quit = 1;
    }

    // Blank Canvas
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

    // header
    sprintf(message, "GPIO PORTS");
    Drawstring(screen, 0, 0, message, cyan);

    sprintf(message, "         ");
    for (pin = 0; pin < 16; pin++) {
      sprintf(message, "%s %d ", message, pin / 10);
    }
    Drawstring(screen, 0, 16, message, cyan);

    sprintf(message, "Pin    : ");
    for (pin = 0; pin < 16; pin++) {
      sprintf(message, "%s %d ", message, pin % 10);

    }
    Drawstring(screen, 0, 32, message, cyan);


    // all GPIO pins
    sprintf(message, "Port A : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_A_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 48, message, white);

    sprintf(message, "Port B : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_B_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 64, message, white);

    sprintf(message, "Port C : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_C_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 80 , message, white);

    sprintf(message, "Port D : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_D_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 96, message, white);

    sprintf(message, "Port E : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_E_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 112, message, white);

    sprintf(message, "Port F : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_F_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 128, message, white);

    sprintf(message, "Port G : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_G_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 144, message, white);

    sprintf(message, "Port H : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_H_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 160, message, white);

    sprintf(message, "Port I : ");
    for (pin = 0; pin < 16; pin++) {
      if (my_gpio_get_input(SUNXI_PORT_I_BASE, pin) == 0)
      {
        sprintf(message, "%s %d ", message, 1);
      }
      else
      {
        sprintf(message, "%s %d ", message, 0);
      }

    }
    Drawstring(screen, 0, 176, message, white);

    // flip framebuffer
    SDL_Flip(screen);
  }
}

int main()
{
  // load font
  bmpfont = SDL_LoadBMP("dosemu.bmp");
  if (bmpfont == NULL) {
    printf("Can't load font: %s\n", SDL_GetError());
    exit(1);
  }

  // init GPIO subsystem
  if (my_gpio_init() != 0) {
    printf("gpio_init ERROR\r\n");
    exit(-1);
  }

  // init SDL subsystem
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Can't initialize SDL: %s\n", SDL_GetError());
    exit(-1);
  }
  atexit(SDL_Quit);

  // init screen
  screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
  if (screen == NULL) {
    fprintf(stderr, "Can't initialize SDL: %s\n", SDL_GetError());
    exit(-1);
  }

  // main loop
  mainLoop();

  // tear down SDL
  SDL_Quit();

  return (0);
}



