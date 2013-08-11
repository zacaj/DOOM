#pragma once

struct Color
{
	int r,b,g;
	Color(){}
	Color(int _r,int _b,int _g)
	{
		r=_r;
		g=_g;
		b=_b;
	}
};
extern "C" {
	extern void initGFX();
	extern void rect(int x,int y,int w,int h);
//	extern Color getpixel(SDL_Surface *surface, int x, int y);
	extern void putpixel(int x, int y);
	extern void bindColor(int r,int g,int b);
	extern void flipGFX();
	extern int getWidth();
	extern int getHeight();
}
