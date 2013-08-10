#include <stdio.h>
#include <SDL.h>
#include <vector>
#include "..\..\enginepp\enginepp\vec2.h"
#include <SDL_rotozoom.h>
#ifndef _MBCS
#include <emscripten.h>
#include <SDL/SDL_image.h>
#else
#define IMG_Load SDL_LoadBMP
#define freopen freopen_s
#define fscanf fscanf_s
#endif
#include <assert.h>
#include <string>
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
float get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
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
		return t;
	}

	return -1; // No collision
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
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
	x&=surface->w-1;
	y&=surface->h-1;
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	//if(x<0 || y<0 || x>surface->w || y>surface->h)
	//	return;
	SDL_Rect r;
	r.x=x;
	r.y=y;
	r.w=r.h=1;
	SDL_FillRect(surface,&r,pixel);
	return;
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}
SDL_Surface *texture;
class Sector;
class Wall
{
public:
	vec2f a,b;
	SDL_Surface *texture;
	Sector *sector;
	Sector *portal;
	int si,pi;
	Wall(vec2f _a,vec2f _b)
	{
		a=_a;
		b=_b;
		portal=NULL;
		sector=NULL;
		texture=NULL;
	}
};
struct Triangle
{
	vec2f a,b,c;
	Triangle(vec2f _a,vec2f _b,vec2f _c)
	{
		a=_a;
		b=_b;
		c=_c;
	}
	float sign(vec2f p1, vec2f p2, vec2f p3)
	{
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	}
	bool pointIsIn(vec2f pt)
	{
		bool b1, b2, b3;

		b1 = sign(pt, a, b) < 0.0f;
		b2 = sign(pt, b, c) < 0.0f;
		b3 = sign(pt, c, a) < 0.0f;

		return ((b1 == b2) && (b2 == b3));
	}
};
class Sector
{
public:
	vec2f p;
	float bottom,top;
	Uint32 floorColor,ceilingColor;
	vector<Wall*> walls;
	vector<Triangle> tris;
	bool pointIsIn(vec2f p)
	{
		for(auto tri:tris)
			if(tri.pointIsIn(p))
				return 1;
		return 0;
	}
};
vector<Wall*> walls;
vector<Sector*> sectors;
Sector *getSector(vec2f p)
{
	for(auto s:sectors)
		if(s->pointIsIn(p))
			return s;
	return NULL;
}
class Entity
{
public:
	vec2f p;
	float z;
	string str;
	Sector *s;
	virtual void update(){}
	Entity()
	{
		s=getSector(p);
	}
};
vector<Entity*> entities;
class Player:public Entity
{
public:
	float angle;
	Player(vec2f _p)
	{
		p=_p;
		angle=0;
		z=5;
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
vec2f camera;
float cameraAngle=0; 
float hFOV=90*DEG2RAD;
float vFOV=hFOV*3/4*DEG2RAD;
float vFrustumSlope=tan(vFOV/2);
float cameraHeight=0;
	bool done=0;
	Uint8 keystate[1500];
void main_loop()
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
				case SDLK_w:
					camera.x+=cos(cameraAngle);
					camera.y+=sin(cameraAngle);
					break;
				case SDLK_DOWN:
					camera.x-=cos(cameraAngle);
					camera.y-=sin(cameraAngle);
					break;
					default:
					break;
				}
					keystate[e.key.keysym.sym]=1;
				break;
			case SDL_KEYUP:
					keystate[e.key.keysym.sym]=0;
					break;
				
		}
	}
	if(keystate[SDLK_LEFT])
	{
		cameraAngle+=.03;
	}
	if(keystate[SDLK_RIGHT])
	{
		cameraAngle-=.03;
	}
	if(keystate[SDLK_RSHIFT])
		cameraHeight+=.1;
	if(keystate[SDLK_RCTRL])
	{
		cameraHeight-=.1;
		if(cameraHeight<0)
			cameraHeight=0;
	}
	SDL_FillRect(screen,0,SDL_MapRGB(screen->format,255,255,0));
	{
		vec2f c(-sin(cameraAngle),cos(cameraAngle));
		for(int x=0;x<screen->w;x++)
		{
			float angle=(float)(screen->w-x)/(float)screen->w*hFOV-hFOV/2+cameraAngle;
			int visibleBottom=screen->h;
			int visibleTop=0;
#if 0
			for(int i=0;i<walls.size();i++)
			{
				vec2f a=walls[i].a-camera;
				vec2f b=walls[i].b-camera;
				vec2f p;
				float percent;
				if((percent=get_line_intersection(a.x,a.y,b.x,b.y,0,0,cos(angle)*1000,sin(angle)*1000,&p.x,&p.y))==-1)
					continue;
				Uint32 color=walls[i].c;
				float dist=LineToPointDistance2D(vec2f(0),c,p);
				float frustumHeight=vFrustumSlope*dist;
				float z1=walls[i].z1+cameraHeight;
				float z2=walls[i].z2+cameraHeight;
				int sz1=(z1/dist*.5+.5)*screen->h;
				int sz2=(z2/dist*.5+.5)*screen->h;
				//SDL_LockSurface(screen);
				Uint32 c=SDL_MapRGB(screen->format,96,96,96);
				if(sz1<visibleBottom)
				{
					rect(x,sz1,1,sz1-visibleBottom,SDL_MapRGB(screen->format,96,96,96));
					for(int y=sz1;y<visibleBottom;y++)
					{
						/*float currentDist=(.5)/((float)y/screen->h-.5)+acos(vFOV/2)*cameraHeight;
						float weight = (currentDist) / (dist);

						float currentFloorX = weight * (p.x+camera.x) + (1 - weight) * camera.x;
						float currentFloorY = weight * (p.y+camera.y) + (1 - weight) * camera.y;*/
					//	putpixel(screen,x,y,c);
					}
					visibleBottom=sz1;
				}
				//SDL_UnlockSurface(screen);
				if(sz2>visibleTop)
				{
					rect(x,visibleTop,1,sz2-visibleTop,SDL_MapRGB(screen->format,192,192,192));
					visibleTop=sz2;
				}
				rect(x,visibleTop,1,visibleBottom-visibleTop,color);
				/*int u=(float)(texture->w)*percent;
				for(int y=visibleTop;y<visibleBottom;y++)
				{
					int v=(float)(y-sz2)/(float)(sz1-sz2)*texture->h;
					putpixel(screen,x,y,getpixel(texture,u,v));
				}*/
				break;
			}
#endif
		}
	}
		SDL_Flip(screen);
}
void load(const char *path);
int main(int argc, char **argv)
{
	for(int i=0;i<1500;i++)
		keystate[i]=0;
	printf("ready");
	SDL_Init(SDL_INIT_VIDEO);
	printf("ready");
	screen=SDL_SetVideoMode(1024,768,32,SDL_HWSURFACE|SDL_DOUBLEBUF);
	load("assets/level.txt");
	printf("ready");
	assert(texture=IMG_Load("assets/texture.bmp"));
	printf("ready");
	#ifdef EMSCRIPTEN
	emscripten_set_main_loop(main_loop,60,1);
	#else
	while(!done)
	{
		main_loop();
	}
	#endif
	SDL_Quit();
	return 0;
}

vec2f read2f(FILE *fp)
{
	vec2f ret;
	fscanf(fp,"%f,%f\n",&ret.x,&ret.y);
	return ret;
}
Uint32 readU32(FILE *fp)
{
	char r;
	r=fgetc(fp);
	r<<4;
	r|=fgetc(fp);
	char g=(fgetc(fp)<<4)|fgetc(fp);
	char b=(fgetc(fp)<<4)|fgetc(fp);
	return SDL_MapRGB(screen->format,r,g,b);
}
void makeEntity(vec2f p,string str);
void load( const char *path )
{
	FILE *fp;
	fopen_s(&fp,path,"r");
	int nWall,nSector;
	fscanf(fp,"%i\n",&nWall);
	for(int i=0;i<nWall;i++)
	{
		Wall *wall=new Wall(read2f(fp),read2f(fp));
		fscanf(fp,"%i,%i\n",&wall->si,&wall->pi);
		int l;
		fscanf(fp,"%i,",&l);
		string str;
		for(int j=0;j<l;j++)
			str.push_back(fgetc(fp));
		fgetc(fp);
		wall->texture=IMG_Load(string("assets/"+str).c_str());
		walls.push_back(wall);
	}
	fscanf(fp,"%i\n",&nSector);
	for(int i=0;i<nSector;i++)
	{
		Sector *s=new Sector();
		fscanf(fp,"%i\n",&nWall);
		for(int j=0;j<nWall;j++)
		{
			int k;
			fscanf(fp,"%i\n",&k);
			s->walls.push_back(walls[k]);
		}
		int k;
		fscanf(fp,"%f,%f\n",&s->bottom,&s->top);
		fgetc(fp);
		s->floorColor=readU32(fp);
		fgetc(fp);
		s->ceilingColor=readU32(fp);
		fgetc(fp);
		fscanf(fp,"%i\n",&k);
		vector<vec2f> pts;
		for(int l=0;l<k;l++)
		{
			pts.push_back(read2f(fp));
		}
		fscanf(fp,"%i\n",&k);
		for(int l=0;l<k;l++)
		{
			int a,b,c;
			fscanf(fp,"%i,%i,%i\n",&a,&b,&c);
			s->tris.push_back(Triangle(pts[a],pts[b],pts[c]));
		}
		s->p=read2f(fp);
		sectors.push_back(s);
	}
	for(int i=0;i<walls.size();i++)
	{
		walls[i]->sector=sectors[walls[i]->si];
		if(walls[i]->pi!=-1)
			walls[i]->portal=sectors[walls[i]->pi];
	}
	int nEntity;
	fscanf(fp,"%i\n",&nEntity);
	for(int i=0;i<nEntity;i++)
	{
		int l;
		fscanf(fp,"%i,",&l);
		string str;
		for(int j=0;j<l;j++)
			str.push_back(fgetc(fp));
		fgetc(fp);
		Entity *e=new Entity();
		e->str=str;
		e->p=read2f(fp);
		makeEntity(e->p,e->str);
		delete(e);
	}
}

void makeEntity( vec2f p,string str )
{
	string type;
	int i=0;
	while(str[i]!='\n' && i<str.size()) type.push_back(str[i++]);
	if(type=="spawn")
	{
		camera=p;
	}
}
