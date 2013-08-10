#include <stdio.h>
#include <SDL.h>
#include <vector>
#include "..\..\enginepp\enginepp\vec2.h"
#include <SDL_rotozoom.h>
using namespace std;
SDL_Surface *screen;
void rect(int x,int y,int w,int h,Uint32 c)
{
	SDL_Rect r;
	r.x=x;
	r.y=y;
	r.w=w;
	r.h=h;
	SDL_FillRect(screen,&r,c);
}
// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
// intersect the intersection point may be stored in the floats i_x and i_y.
char get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
						   float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y)
{
	float s1_x, s1_y, s2_x, s2_y;
	s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
	s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

	float s, t;
	s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Collision detected
		if (i_x != NULL)
			*i_x = p0_x + (t * s1_x);
		if (i_y != NULL)
			*i_y = p0_y + (t * s1_y);
		return 1;
	}

	return 0; // No collision
}
//Compute the cross product AB x AC
double CrossProduct(vec2f pointA, vec2f pointB,vec2f pointC)
{
	vec2f AB;
	vec2f AC ;
	AB.x = pointB.x - pointA.x;
	AB.y = pointB.y - pointA.y;
	AC.x = pointC.x - pointA.x;
	AC.y = pointC.y - pointA.y;
	double cross = AB.x * AC.y - AB.y * AC.x;

	return cross;
}

//Compute the distance from A to B
double Distance(vec2f pointA, vec2f pointB)
{
	double d1 = pointA.x - pointB.x;
	double d2 = pointA.y - pointB.y;

	return sqrt(d1 * d1 + d2 * d2);
}

//Compute the distance from AB to C
//if isSegment is true, AB is a segment, not a line.
float LineToPointDistance2D(vec2f pointA,vec2f pointB, vec2f pointC)
{
	float dist = CrossProduct(pointA, pointB, pointC) / Distance(pointA, pointB);
	return fabs(dist);
} 
class Wall
{
public:
	vec2f a,b;
	float z1,z2;
	Uint32 c;
	Wall(float xa,float ya,float xb,float yb,int r,int g,int _b)
	{
		a(xa,ya);
		b(xb,yb);
		a*=2;
		b*=2;
		z1=5;
		z2=-5;
		c=SDL_MapRGB(screen->format,r,g,_b);
	}
};

void wrap( float &angle ) 
{
	while(angle<0)
		angle+=M_PI*2;
	while(angle>M_PI*2)
		angle-=M_PI*2;
}

#define DEG2RAD 3.1415/180.f
vector<Wall> walls;
vec2f camera;
float cameraAngle=0; 
float hFOV=90*DEG2RAD;
float vFOV=150*DEG2RAD;
float vFrustumSlope=tan(vFOV/2);
int main(int argc, char **argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	screen=SDL_SetVideoMode(1024,768,32,SDL_HWSURFACE|SDL_DOUBLEBUF);
	walls.push_back(Wall(7,-7,7,7,255,0,0));
	walls.push_back(Wall(7,7,-7,7,0,0,255));
	walls.push_back(Wall(-7,7,-7,-7,0,255,0));
	walls.push_back(Wall(-7,-7,7,-7,255,255,0));
	bool done=0;
	while(!done)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_QUIT:
					done=1;
					break;
				case SDL_KEYDOWN:
					switch(e.key.keysym.sym)
					{
					case SDLK_ESCAPE:
						done=1;
						break;
					case SDLK_UP:
						camera.x+=cos(cameraAngle);
						camera.y+=sin(cameraAngle);
						break;
					case SDLK_DOWN:
						camera.x-=cos(cameraAngle);
						camera.y-=sin(cameraAngle);
						break;
					}
					break;
			}
		}
		Uint8 *keystate=SDL_GetKeyState(0);
		if(keystate[SDLK_LEFT])
		{
			cameraAngle+=.03;
		}
		if(keystate[SDLK_RIGHT])
		{
			cameraAngle-=.03;
		}
		SDL_FillRect(screen,0,0);

		{
			vec2f c(cos(cameraAngle-M_PI/2)*1000,sin(cameraAngle-M_PI/2)*1000);
			for(int x=0;x<screen->w;x++)
			{
				float angle=(float)(screen->w-x)/(float)screen->w*hFOV-hFOV/2+cameraAngle;
				int visibleBottom=screen->h;
				int visibleTop=0;
				for(int i=0;i<walls.size();i++)
				{
					vec2f a=walls[i].a+camera;
					vec2f b=walls[i].b+camera;
					vec2f p;
					if(!get_line_intersection(a.x,a.y,b.x,b.y,0,0,cos(angle)*1000,sin(angle)*1000,&p.x,&p.y))
						continue;
					Uint32 color=walls[i].c;
					float dist=LineToPointDistance2D(vec2f(0),c,p);
					float frustumHeight=vFrustumSlope*dist;
					float z1=walls[i].z1;
					float z2=walls[i].z2;
					int sz1=z1/frustumHeight*screen->h+screen->h/2;
					int sz2=z2/frustumHeight*screen->h+screen->h/2;
					if(sz1<visibleBottom)
					{
						rect(x,sz1,1,sz1-visibleBottom,SDL_MapRGB(screen->format,96,96,96));
						visibleBottom=sz1;
					}
					if(sz2>visibleTop)
					{
						rect(x,visibleTop,1,visibleTop-sz2,SDL_MapRGB(screen->format,192,192,192));
						visibleTop=sz2;
					}
					rect(x,visibleTop,1,visibleBottom-visibleTop,color);
					break;
				}
			}
		}

		SDL_Flip(screen);
	}
	SDL_Quit();
	return 0;
}