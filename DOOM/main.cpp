#include <stdio.h>
#include <vector>
#include "..\..\enginepp\enginepp\vec2.h"
#ifndef _MBCS
#include <emscripten.h>
//#include <SDL/SDL_image.h>
#else
#include <SDL.h>
#define IMG_Load(s) SDL_DisplayFormat(SDL_LoadBMP(s))
#define M_PI 3.1415962
#ifndef GCC
#define freopen freopen_s
#define fscanf fscanf_s

#endif
#endif
#include <assert.h>
#include <string>
using namespace std;
#include <map>
#include "js.h"
#include <math.h>
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
class Texture
{
public:
	Color **pixels;
	float w,h;
	string name;
	Texture( string path )
	{
		name=path;
		/*SDL_Surface *s=IMG_Load(path.c_str());
		SDL_LockSurface(s);
		w=s->w;
		h=s->h;
		pixels=new Color*[s->w];
		for(int i=0;i<s->w;i++)
		{
			pixels[i]=new Color[s->h];
			for(int j=0;j<s->h;j++)
				pixels[i][j]=getpixel(s,j,i);
		}
		SDL_UnlockSurface(s);
		SDL_FreeSurface(s);*/
	}
};
map<string,Texture*> textures;
Texture *getTexture(string path)
{
	auto it=textures.find(path);
	if(it==textures.end())
		return new Texture(path);
	return it->second;
}
class Sector;
class Wall
{
public:
	vec2f a,b;
	Texture *texture;
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
	Color floorColor;
	Color ceilingColor;
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
		s=NULL;
	}
};
vector<Entity*> entities;
class Player;
Player *player;
class Player:public Entity
{
public:
	float angle;
	Player(vec2f _p):Entity()
	{
		p=_p;
		angle=0;
		z=5;
		player=this;
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
float hFOV=90*DEG2RAD;
float vFOV=hFOV*3/4*DEG2RAD;
float vFrustumSlope=tan(vFOV/2);
	bool done=0;
unsigned char keystate[1500];
void main_loop()
{
	/*SDL_Event e;
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
					player->p.x+=cos(player->angle);
					player->p.y+=sin(player->angle);
					break;
				case SDLK_DOWN:
					player->p.x-=cos(player->angle);
					player->p.y-=sin(player->angle);
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
		player->angle+=.03;
	}
	if(keystate[SDLK_RIGHT])
	{
		player->angle-=.03;
	}
	if(keystate[SDLK_RSHIFT])
		player->angle+=.1;
	if(keystate[SDLK_RCTRL])
	{
		player->angle-=.1;
		if(player->z<0)
			player->z=0;
			}*/
player->angle-=.03;
	bindColor(0,0,0);
	rect(0,0,1024,768);
	{
		vec2f c(-sin(player->angle),cos(player->angle));
		for(int x=0;x<getWidth();x++)
		{
			float angle=(float)(getWidth()-x)/(float)getWidth()*hFOV-hFOV/2+player->angle;
			int visibleBottom=getHeight();
			int visibleTop=0;
			Sector *s=getSector(player->p);
			int i;
			for(i=0;i<s->walls.size();i++)
			{
				vec2f a=s->walls[i]->a-player->p;
				vec2f b=s->walls[i]->b-player->p;
				vec2f p;
				float percent;
				if((percent=get_line_intersection(a.x,a.y,b.x,b.y,0,0,cos(angle)*1000,sin(angle)*1000,&p.x,&p.y))==-1)
					continue;
				float dist=LineToPointDistance2D(vec2f(0),c,p);
				float frustumHeight=vFrustumSlope*dist;
				float z1=s->bottom-player->z;
				float z2=s->top-player->z;
				int sz1=(1-(z1/dist*.5+.5))*getHeight();
				int sz2=(1-(z2/dist*.5+.5))*getHeight();
				if(sz1<visibleBottom)
				{
					bindColor(s->floorColor.r,s->floorColor.g,s->floorColor.b);
					rect(x,sz1,1,visibleBottom-sz1);
					visibleBottom=sz1;
				}
				if(sz2>visibleTop)
				{
					bindColor(s->ceilingColor.r,s->ceilingColor.g,s->ceilingColor.b);
					rect(x,visibleTop,1,sz2-visibleTop);
					visibleTop=sz2;
				}
				int u=percent*s->walls[i]->texture->w;
				//Color *c=s->walls[i]->texture->pixels[u];
				bindColor(255,255,255);
				float h=sz1-sz2;
				float v=(visibleTop-sz2)/h;
				float dv=s->walls[i]->texture->h/h;
				for(int y=visibleTop;y<visibleBottom;y++)
				{
					putpixel(x,y/*c[(int)v]*/);
					v+=dv;
				}
			}
			continue;
		}
	}
	flipGFX();
}
void load(const char *path);
int main(int argc, char **argv)
{
	for(int i=0;i<1500;i++)
		keystate[i]=0;
	printf("ready");
	initGFX();
	load("assets/level.txt");
	printf("ready");
	printf("ready");
	#ifdef EMSCRIPTEN
	emscripten_set_main_loop(main_loop,60,1);
	#else
	while(!done)
	{
		main_loop();
	}
	#endif
	return 0;
}

vec2f read2f(FILE *fp)
{
	vec2f ret;
	fscanf(fp,"%f,%f\n",&ret.x,&ret.y);
	return ret;
}
unsigned char readHex(FILE *fp)
{
	char c=fgetc(fp);
	if(c<='9')
		return c-'0';
	return c-'A'+10;
}
Color readU32(FILE *fp)
{
	unsigned char r=(readHex(fp)<<4)|readHex(fp);
	unsigned char g=(readHex(fp)<<4)|readHex(fp);
	unsigned char b=(readHex(fp)<<4)|readHex(fp);
	return Color(r,g,b);
}
void makeEntity(vec2f p,string str);
void load( const char *path )
{
#ifndef _MBCS
	FILE *fp=fopen(path,"r");
#else
	FILE *fp;
	fopen_s(&fp,path,"r");
#endif
	int nWall,nSector;
	fscanf(fp,"%i\n",&nWall);
	for(int i=0;i<nWall;i++)
	{
		Wall *wall=new Wall(read2f(fp),read2f(fp));
		//wall->a*=.1;
		//wall->b*=.1;
		fscanf(fp,"%i,%i\n",&wall->si,&wall->pi);
		int l;
		fscanf(fp,"%i,",&l);
		string str;
		for(int j=0;j<l;j++)
			str.push_back(fgetc(fp));
		fgetc(fp);
		wall->texture=getTexture(string("assets/"+str));
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
		entities.push_back(new Player(p));
	}
}
