#include "js.h"
#include <SDL.h>
#include <SDL_rotozoom.h>
SDL_Surface *screen;
Uint32 color;
void initGFX()
{
	SDL_Init(SDL_INIT_VIDEO);
	printf("ready");
	screen=SDL_SetVideoMode(1024,768,32,SDL_HWSURFACE|SDL_DOUBLEBUF);
}


void rect(int x,int y,int w,int h)
{
	SDL_Rect r;
	r.x=x;
	r.y=y;
	r.w=w;
	r.h=h;
	SDL_FillRect(screen,&r,color);
}
void getpixel(SDL_Surface *surface, int x, int y)
{
	x&=surface->w-1;
	y&=surface->h-1;
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		color= *p;

	case 2:
		color= *(Uint16 *)p;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			color= p[0] << 16 | p[1] << 8 | p[2];
		else
			color= p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		color= *(Uint32 *)p;

	default:
		color=0;       /* shouldn't happen, but avoids warnings */
	}
}

void putpixel(int x, int y)
{
	SDL_Rect r;
	r.x=x;
	r.y=y;
	r.w=r.h=1;
	SDL_FillRect(screen,&r,color);
	return;
}

void bindColor( int r,int g,int b )
{
	color=SDL_MapRGB(screen->format,r,g,b);
}

void flipGFX()
{
	SDL_Flip(screen);
}

int getWidth()
{
	return screen->w;
}

extern int getHeight()
{
	return screen->h;
}
