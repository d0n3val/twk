
#if _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <ctime> 
# pragma comment(lib, "bass/bass.lib")
# pragma comment(lib, "mxml/mxml1.lib")
#elif __linux__ || _MACOSX
# include <dirent.h>
# include <sys/time.h>
# include <time.h>
#endif
#ifdef _MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <GLUT/glut.h>
# include "../bass/bass.h"
#else 
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glut.h>
#endif
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _WIN32
# include "bass/bass.h"
# include "mxml/mxml.h"
#endif

#if _WIN32
# define _foreach_file(path,func) \
	do { \
		HANDLE _find; \
		WIN32_FIND_DATA _data; \
		char _name[256]; \
		if ((_find = FindFirstFile(path "/*", &_data)) != INVALID_HANDLE_VALUE) { \
			do { \
				if (_data.cFileName[0] == '.') continue; \
				snprintf(_name, sizeof(_name), path "/%s", _data.cFileName); \
				func(_name); \
			} while (FindNextFile(_find, &_data)); \
			FindClose(_find); \
		} \
	} while (0)
# define snooze(msec) Sleep(msec)
# define tbfreq(out) do { LARGE_INTEGER _; QueryPerformanceFrequency(&_); out = _.QuadPart; } while (0)
# define tbcount(out) do { LARGE_INTEGER _; QueryPerformanceCounter(&_); out = _.QuadPart; } while (0)
//#elif __linux__
#else
# define _foreach_file(path,func) \
	do { \
		DIR* _dir; \
		struct dirent* _de; \
		char _name[256]; \
		if ((_dir = opendir(path)) != NULL) { \
			while ((_de = readdir(_dir)) != NULL) { \
				if (_de->d_name[0] == '.') continue; \
				snprintf(_name, sizeof(_name), path "/%s", _de->d_name); \
				func(_name); \
			} \
			closedir(_dir); \
		} \
	} while (0)
# define snooze(msec) \
	do { \
		struct timespec _; \
		_.tv_sec = (msec) / 1000; \
		_.tv_nsec = ((msec) - (msec) / 1000 * 1000) * 1000000; \
		while (nanosleep(&_, &_) == -1); \
	} while (0)
# define tbfreq(out) do { out = 1000; } while (0)
# define tbcount(out) \
	do { \
		struct timeval _; \
		gettimeofday(&_, NULL); \
		out = (unsigned long long) (_.tv_sec * 1000. + _.tv_usec / 1000. + .5); \
	} while (0)
#endif

#undef min
#undef max
#undef near
#undef far

#if _WIN32
# define snprintf _snprintf
#endif

#define arrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) < (b) ? (b) : (a))
#define clamp(a,mn,mx) min(max(a, mn), mx)

#define keyDown(k) ((g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f))) && !(g_keys[1][(k) >> 5] & (1 << ((k) & 0x1f))))
#define keyUp(k) (!(g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f))) && (g_keys[1][(k) >> 5] & (1 << ((k) & 0x1f))))
#define keyHeld(k) (g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f)))

#define buttonDown(b) ((g_buttons[0] & (1 << (b))) && !(g_buttons[1] & (1 << (b))))
#define buttonUp(b) (!(g_buttons[0] & (1 << (b))) && (g_buttons[1] & (1 << (b))))
#define buttonHeld(b) (g_buttons[0] & (1 << (b)))

unsigned long long g_clock;
double g_period;
float g_elapse;

int g_window;
int g_reshape;
float g_width;
float g_height;

unsigned g_keys[2][256 >> 5];
unsigned g_buttons[2];
int g_mousex;
int g_mousey;

int g_fontList;
const unsigned g_font[] =
{
	0x00000000, 0x00000000, 0x00000800, 0x08080808, 0x00000000, 0x14141400, 0x3e141400, 0x14143e14,
	0x0a3c0800, 0x081e281c, 0x10260600, 0x30320408, 0x2a241a00, 0x10282810, 0x00000000, 0x02040800,
	0x08040200, 0x02040808, 0x08102000, 0x20100808, 0x1c2a0800, 0x082a1c08, 0x08080000, 0x0008083e,
	0x00080810, 0x00000000, 0x00000000, 0x0000003e, 0x00181800, 0x00000000, 0x10200000, 0x00020408,
	0x32221c00, 0x1c22262a, 0x08083e00, 0x08182808, 0x18203e00, 0x1c220204, 0x02221c00, 0x1c22020c,
	0x3e040400, 0x040c1424, 0x02221c00, 0x3e203c02, 0x22221c00, 0x0c10203c, 0x08080800, 0x3e220408,
	0x22221c00, 0x1c22221c, 0x02041800, 0x1c22221e, 0x00080000, 0x00000800, 0x00080810, 0x00000800,
	0x180c0600, 0x060c1830, 0x3e000000, 0x00003e00, 0x0c183000, 0x30180c06, 0x08000800, 0x1c220204,
	0x2e201c00, 0x1c222e2a, 0x3e222200, 0x08142222, 0x12123c00, 0x3c12123c, 0x20120c00, 0x0c122020,
	0x12143800, 0x38141212, 0x20203e00, 0x3e20203c, 0x20202000, 0x3e20203c, 0x22221c00, 0x1c22202e,
	0x22222200, 0x2222223e, 0x08081c00, 0x1c080808, 0x04241800, 0x0e040404, 0x28242200, 0x22242830,
	0x20203e00, 0x20202020, 0x22222200, 0x22362a2a, 0x26262200, 0x2232322a, 0x22221c00, 0x1c222222,
	0x20202000, 0x3c22223c, 0x2a241a00, 0x1c222222, 0x28242200, 0x3c22223c, 0x02221c00, 0x1c22201c,
	0x08080800, 0x3e080808, 0x22221c00, 0x22222222, 0x14140800, 0x22222222, 0x2a362200, 0x2222222a,
	0x14222200, 0x22221408, 0x08080800, 0x2222221c, 0x10203e00, 0x3e020408, 0x08080e00, 0x0e080808,
	0x04020000, 0x00201008, 0x08083800, 0x38080808, 0x00000000, 0x08142200, 0x00003e00, 0x00000000,
	0x00000000, 0x00000000, 0x1e221e00, 0x00001c02, 0x22322c00, 0x20202c32, 0x20221c00, 0x00001c22,
	0x22261a00, 0x02021a26, 0x3e201c00, 0x00001c22, 0x08080800, 0x040a083e, 0x261a021c, 0x00001a26,
	0x22222200, 0x20202c32, 0x08081c00, 0x08001808, 0x08084830, 0x08001808, 0x30282400, 0x20202428,
	0x08081c00, 0x18080808, 0x2a2a2a00, 0x0000342a, 0x22222200, 0x00002c32, 0x22221c00, 0x00001c22,
	0x322c2020, 0x00002c32, 0x261a0202, 0x00001a26, 0x20202000, 0x00002c32, 0x3c023c00, 0x00001e20,
	0x10120c00, 0x10103c10, 0x24241a00, 0x00002424, 0x22140800, 0x00002222, 0x2a2a1400, 0x0000222a,
	0x08142200, 0x00002214, 0x261a021c, 0x00002222, 0x08103e00, 0x00003e04, 0x10100c00, 0x0c101020,
	0x08080800, 0x08080808, 0x08083000, 0x30080804, 0x00000000, 0x3e000000, 0x00000000, 0x00000000
};

struct Text
{
	float x, y;
	unsigned color, length;
	char string[0];
};

char g_textBuffer[2048];
char* g_textEnd;

///////////////////////////////////////////////////////////////////////////////

enum
{
	TEX_CRATE,
	TEX_PLAYER,
	TEX_WALLFLOOR,
	TEX_WALLVARS,
	TEX_FLOORVARS,
	TEX_FLOOR_G,
	TEX_FLOOR_P,
	TEX_TARGET_G,
	TEX_TARGET_P,
	TEX_CLOUD,
	NUM_TEX
};

#define MAX_TEX 100

#define texUvPlayer_up(u0,v0,u1,v1,s) \
	do { \
		u0 = 0.f + floorf((s) / .25f) * .125f; u1 = (u0) + .125f; \
		v0 = 0.f; v1 = .125f; \
	} while (0)

#define texUvPlayer_right(u0,v0,u1,v1,s) \
	do { \
		u0 = .5f + floorf((s) / .25f) * .125f; u1 = (u0) + .125f; \
		v0 = 0.f; v1 = .125f; \
	} while (0)

#define texUvPlayer_down(u0,v0,u1,v1,s) \
	do { \
		u0 = 0.f + floorf((s) / .25f) * .125f; u1 = (u0) + .125f; \
		v0 = .125f; v1 = .25f; \
	} while (0)

#define texUvPlayer_left(u0,v0,u1,v1,s) \
	do { \
		u0 = .5f + floorf((s) / .25f) * .125f; u1 = (u0) + .125f; \
		v0 = .125f; v1 = .25f; \
	} while (0)

const char* g_texnames[] = {
	"crate",
	"player",
	"wallfloor",
	"wall-vars",
	"floor-vars",
	"floor-g",
	"floor-p",
	"target-g",
	"target-p",
	"cloud"
};

unsigned g_tex[NUM_TEX+MAX_TEX];

///////////////////////////////////////////////////////////////////////////////

#define PATH_NAME_SIZE (64)
#define DATA_DIR ("data/")

struct MapInfo
{
	char name[80];
	char txt[256];
	char file[PATH_NAME_SIZE];
	char music[PATH_NAME_SIZE];
	char bg[PATH_NAME_SIZE];
	int seed;
	int par;
};

#define MAX_MAPS 20

struct MapInfo g_map_progression[MAX_MAPS];
int g_current_map = 0;

///////////////////////////////////////////////////////////////////////////////

#define TILE_VOID ('!')
#define TILE_WALL ('X')
#define TILE_FLOOR ('.')
#define TILE_START ('$')
#define TILE_START_TARGET ('@')
#define TILE_CRATE ('#')
#define TILE_CRATE_TARGET ('%')
#define TILE_TARGET ('*')
#define TILE_UNKNOWN ('?')

#define MAX_CRATES 32
#define MAX_LAYERS 10
#define MAX_TEXTURES 20

struct Object
{
	int x, y;
};

struct texData 
{
	int max_id;
	int num_tiles;
};


struct World
{
	int ntextures;
	struct texData Textures[MAX_TEXTURES];

	int crateTex;
	int crateTargetTex;
	int ncrates;
	struct Object crates[MAX_CRATES];

	int ntargets;
	struct Object targets[MAX_CRATES];

	int startx, starty;
	int nx, ny;
	int nlayers;
	int wallLayer;
	unsigned char tiles[MAX_LAYERS][4096];
};

struct World g_world;

int worldTile(int layer, int x, int y)
{
	return x >= 0 && y >= 0 ? g_world.tiles[layer][y * g_world.nx + x] : TILE_UNKNOWN;
}

struct Object* worldCrates(int x, int y)
{
	for (int i = 0; i < g_world.ncrates; ++i)
		if (g_world.crates[i].x == x && g_world.crates[i].y == y)
			return &g_world.crates[i];

	return NULL;
}

struct Object* worldTargets(int x, int y)
{
	for (int i = 0; i < g_world.ntargets; ++i)
		if (g_world.targets[i].x == x && g_world.targets[i].y == y)
			return &g_world.targets[i];

	return NULL;
}

int isWall(int x, int y)
{
	int w = worldTile(g_world.wallLayer, x, y);

	if (g_world.nlayers > 1)
		return w != 0;
	else
		return w == TILE_WALL || w == TILE_VOID;
}

int isWalkable(int x, int y)
{
	int w = worldTile(g_world.wallLayer, x, y);

	if (g_world.nlayers > 1)
		return w == 0;
	else
		return w == TILE_FLOOR || w == TILE_TARGET;
}

int isTargetTile(int x, int y)
{
	if (g_world.nlayers > 1)
		return worldTargets(x,y) != NULL;

	struct Object* target = &g_world.targets[g_world.ntargets];
	while(target-- != g_world.targets)
	{
		if (target->x == x && target->y == y)
			return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

#define SAVE_GAME_VERSION (2)
#define SAVE_GAME_PATH ("data/save.bin")

void gprintf(float x, float y, unsigned c, const char* fmt, ...);
void quad(float x, float y, float s, unsigned c);
void sprite(float x, float y, float s, unsigned texId, float u0, float v0, float u1, float v1);

char* loadFile(const char *path, int *readed);
void loadMap(const char* path);
void loadMap_tmx(const char* path);
void saveGame();
void loadGame();
void loadConfig(const char* path);

///////////////////////////////////////////////////////////////////////////////

#if _WIN32 || _MACOSX
HSTREAM g_musicStream = 0;
#endif
unsigned g_mute;

void playMusic(const char* path)
{
	(void) path;

#if _WIN32 || _MACOSX
	char data_path[PATH_NAME_SIZE] = DATA_DIR;
	strcat(data_path, path);

	printf("Loading music [%s], Nick dancing in 3 2 1\n", data_path);
	if (g_musicStream != 0 )
	{
		BASS_ChannelStop(g_musicStream);
		//BASS_StreamFree(g_musicStream);
		g_musicStream = 0;
	}
		
	g_musicStream = BASS_StreamCreateFile(FALSE, data_path, 0, 0, BASS_SAMPLE_LOOP);
	BASS_ChannelPlay(g_musicStream, 0);
#endif
}

void ChangeRes(int w, int h, const char * name)
{
	char title[80] = "Touchy Warehouse Keeper - ";
	const int scrw = glutGet(GLUT_SCREEN_WIDTH) * 0.5f;
	const int scrh = glutGet(GLUT_SCREEN_HEIGHT) * 0.5f;

	strcat(title, name);
	glutSetWindowTitle(title);
	glutReshapeWindow(w, h);
	glutPositionWindow(scrw - w/2, scrh - h/2);
}

///////////////////////////////////////////////////////////////////////////////

struct Spring
{
	float dist;
	float mass;
	float damp;
	float k;
	float x, y;
	float vx, vy;
	float fx, fy;
};

void springUpdate(float attx, float atty, struct Spring* s, float elapse)
{
	s->fx = 0.f;
	s->fy = -9.81f * s->mass;

	float x = s->x - attx;
	float y = s->y - atty;
	float n = sqrtf(x * x + y * y);

	if (n - s->dist >= .001f)
	{
		float i = -s->k * (n - s->dist);

		x /= n;
		y /= n;

		n = sqrtf(x * x + y * y);

		s->fx += x / n * i - .25f * s->vx;
		s->fy += y / n * i - .25f * s->vy;
	}

	s->vx += s->fx / s->mass * elapse;
	s->vy += s->fy / s->mass * elapse;

	s->x += s->vx;
	s->y += s->vy;

	s->vx *= s->damp;
	s->vy *= s->damp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GAME_START (0)
#define GAME_PLAY (1)
#define GAME_OVER (2)

///////////////////////////////////////////////////////////////////////////////

struct GameStart
{
	int init;
	int hasSaveGame;
	int loadGameOnStart;
	float mx, my;
	float fx, fy;
	float px, py;
	float cx[3], cy[3], cs[3], crx[3], cry[3];
	float blink;
	unsigned state;
	float sx[2], sy[2];
	struct Spring spring[2];
} g_gameStart;

void gameStart(float elapse, unsigned* stage)
{
	struct GameStart* gs = &g_gameStart;

	if (!gs->init || g_reshape)
	{
		gs->init = 1;
		playMusic("menu.mp3");
		g_current_map = 0;

		gs->mx = g_mousex;
		gs->my = g_mousey;
		gs->fx = g_width * .5f;
		gs->fy = g_height * .5f;
		gs->px = g_width * .5f - 64.f;
		gs->py = g_height * .5f - 48.f;

		gs->cx[0] = gs->px - 72.f;
		gs->cy[0] = gs->py + 12.f;
		gs->cs[0] = 48.f;
		gs->crx[0] = 60.f;
		gs->cry[0] = 30.f;

		gs->cx[1] = gs->px + 72.f;
		gs->cy[1] = gs->py + 4.f;
		gs->cs[1] = 64.f;
		gs->crx[1] = 40.f;
		gs->cry[1] = 20.f;

		gs->cx[2] = gs->px - 32.f;
		gs->cy[2] = gs->py;
		gs->cs[2] = 96.f;
		gs->crx[2] = 30.f;
		gs->cry[2] = 10.f;

		gs->sx[0] = .3f;
		gs->sy[0] = 2.2f;
		gs->spring[0].mass = .06f;
		gs->spring[0].damp = .99f;
		gs->spring[0].dist = .2f;
		gs->spring[0].k = .5f;
		gs->spring[0].x = .3f;
		gs->spring[0].y = 1.1f;
		gs->spring[0].vx = 0.f;
		gs->spring[0].vy = 0.f;
		gs->spring[0].fx = 0.f;
		gs->spring[0].fy = 0.f;

		gs->sx[1] = .7f;
		gs->sy[1] = 1.8f;
		gs->spring[1].mass = .1f;
		gs->spring[1].damp = .9f;
		gs->spring[1].dist = .2f;
		gs->spring[1].k = 1.1f;
		gs->spring[1].x = .5f;
		gs->spring[1].y = 1.1f;
		gs->spring[1].vx = 0.f;
		gs->spring[1].vy = 0.f;
		gs->spring[1].fx = 0.f;
		gs->spring[1].fy = 0.f;

		FILE* fd;
		int ver;

		if ((fd = fopen(SAVE_GAME_PATH, "rb")) != NULL)
		{
			if (fread(&ver, sizeof(ver), 1, fd) != 1)
				fprintf(stderr, "Failed to read version\n");

			if (ver == SAVE_GAME_VERSION)
				gs->hasSaveGame = 1;

			fclose(fd);
		}
	}

	if (gs->mx != g_mousex)
		gs->fx = gs->mx = g_mousex;

	if (gs->my != g_mousey)
		gs->fy = gs->my = g_mousey;

	float dx = (gs->fx - gs->px) / g_width;
	float dy = (gs->fy - gs->py) / g_height;

	for (unsigned i = 0; i < arrayCount(gs->spring); ++i)
	{
		float hw = g_width;
		float hh = g_height;

		float attx = gs->sx[i] - 50.f / hw + 100.f / hw * dx;
		float atty = gs->sy[i] + 20.f / hh - 40.f / hh * dy;

		// solve spring

		springUpdate(attx, atty, &gs->spring[i], elapse * .2f);

		float sx = gs->spring[i].x, sy = gs->spring[i].y;
		float sl = sqrtf(sx * sx + sy * sy);
		sx /= sl, sy /= sl;

		float ax = attx, ay = atty;
		float al = sqrtf(ax * ax + ay * ay);
		ax /= al, ay /= al;

		float a = acosf(sx * ax + sy * ay);
		if (fabsf(a) >= .0001f)
			a /= M_PI, a *= 180.f;
		else
			a = 0.f;

		float cz = sx * ay - sy * ax;

		// draw it..

		attx = attx * g_width;
		atty = g_height - atty * g_height;

		glColor3f(1.f, 1.f, 1.f);
		glBegin(GL_LINES);
			glVertex2f(attx, atty);
			glVertex2f(gs->spring[i].x * g_width, g_height - gs->spring[i].y * g_height);
		glEnd();

		glPushMatrix();
			glTranslatef(gs->spring[i].x * g_width, g_height - gs->spring[i].y * g_height, 0.f);
			glRotatef(a, 0.f, 0.f, cz);
			sprite(0.f, 0.f, 192.f + i * 128.f, TEX_CLOUD, 0, 0, 1, 1);
		glPopMatrix();
	}

	for (int i = 0; i < 3; ++i)
	{
		float x = gs->cx[i];
		float y = gs->cy[i];
		float s = gs->cs[i];
		float rx = gs->crx[i];
		float ry = gs->cry[i];

		sprite((x - rx * .5f) + (rx * dx), (y - ry * .5f) + (ry * dy), s, TEX_CRATE, 0.f, 0.f, 1.f, 1.f);
	}

	float u0, v0, u1, v1;
	u0 = .5f, u1 = u0 + .125f;
	v0 = .375f, v1 = v0 + .125f;
	sprite(gs->px, gs->py, 128.f, TEX_PLAYER, u0, v0, u1, v1);

	static const unsigned colors[] = {0x0000ff00, 0x00ffffff};
	unsigned color = colors[gs->state];

	gprintf(.5f, .5f, 0x00ff00ff, "Touchy Warehouse Keeper");
	gprintf(.5f, .52f, 0x00ff00ff, "v0.1");
	gprintf(.5f, .56f, 0x00ff00ff, "Map (right click to cycle): %s", g_map_progression[g_current_map].name);
	gprintf(.5f, .60f, color, "Click to continue");

	if (gs->hasSaveGame)
		gprintf(.02f, .95f, 0x00ffffff, "[C]ONTINUE");

	if ((gs->blink += elapse) >= 1.f)
		gs->blink -= 1.f, gs->state ^= 1;

	if (buttonDown(2))
	{
		++g_current_map;
		if(g_map_progression[g_current_map].par < 0)
			g_current_map = 0;
	}

	if (buttonUp(0) || !!(gs->loadGameOnStart = keyDown('c')))
		++(*stage);

	if (keyDown('1'))
		ChangeRes(glutGet(GLUT_SCREEN_WIDTH) / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2, "PC");
	else if (keyDown('2'))
		ChangeRes(1024, 800, "Galaxy Tablet");
	else if (keyDown('3'))
		ChangeRes(1024, 768, "Ipad");
	else if (keyDown('4'))
		ChangeRes(1024, 600, "Playbook");
	else if (keyDown('5'))
		ChangeRes(960, 640, "Iphone 4");
	else if (keyDown('6'))
		ChangeRes(800, 480, "Galaxy Phone");
	else if (keyDown('7'))
		ChangeRes(480, 360, "BlackBerry");
}


///////////////////////////////////////////////////////////////////////////////

struct PathNode
{
	int x : 16;
	int y : 16;
	int h : 8;
	int g : 6;
	int p : 2;
};

struct Player
{
	float time;
	int x, y;
	int ix, iy;
	int path;
	int move;
	float speed;
#if _WIN32 || _MACOSX
	HCHANNEL sound_channel;
#endif
};

struct Move
{
	int x0, y0;
	int x1, y1;
	int dx, dy;
	int ci;
};

struct Timer
{
	float elapse;
	int hours;
	int minutes;
	int seconds;
};

struct UndoStep
{
	struct Object crates[MAX_CRATES];
	int playerx, playery;
};

struct GamePlay
{
	int init;
	int moves;
	int finished;
	float intro;
	struct Timer timer;
	struct Player player;
	struct Move move;
	int npath;
	struct PathNode path[128];
	int stepSaved;
	struct UndoStep steps[64];
} g_gamePlay;

int pathFind(int startx, int starty, int goalx, int goaly)
{
	static struct PathNode nodes[2048];
	static struct PathNode* open[256];
	static struct PathNode* closed[2048];

	struct GamePlay* gp = &g_gamePlay;
	const struct PathNode start = {startx, starty, 0, 0, 0};

	nodes[0] = start;
	open[0] = &nodes[0];

	for (int nnodes = 1, nopen = 1, nclosed = 0; nopen > 0;)
	{
		int i, j, k = 0;

		for (int i = 1; i < nopen; ++i)
			if (open[k]->g + open[k]->h > open[i]->g + open[i]->h)
				k = i;

		struct PathNode* cur = open[k];

		if (cur->x == goalx && cur->y == goaly)
		{
			for (gp->npath = 0; cur->x != startx || cur->y != starty;)
			{
				assert((size_t) gp->npath < arrayCount(gp->path));
				gp->path[gp->npath++] = *cur;

				int x = cur->x + ((1 | cur->p << 31 >> 31) & ~(cur->p << 30 >> 31));
				int y = cur->y + ((1 | cur->p << 31 >> 31) & (cur->p << 30 >> 31));

				for (j = 0; nodes[j].x != x || nodes[j].y != y; ++j)
					assert(j < nnodes);

				cur = &nodes[j];
			}

			return 0;
		}

		assert((size_t) nclosed < arrayCount(closed));
		open[k] = open[--nopen];
		closed[nclosed++] = cur;

		for (i = 0; i < 4; ++i)
		{
			int x = cur->x + ((1 | i << 31 >> 31) & ~(i << 30 >> 31));
			int y = cur->y + ((1 | i << 31 >> 31) & (i << 30 >> 31));

			if (x < 0 || x >= g_world.nx || y < 0 || y >= g_world.ny)
				continue;

			for (j = 0; j < nclosed; ++j)
				if (closed[j]->x == x && closed[j]->y == y)
					break;

			if (j < nclosed)
				continue;

			if (isWall(x, y) || worldCrates(x, y))
				continue;

			for (j = 0; j < nopen; ++j)
				if (open[j]->x == x && open[j]->y == y)
					break;

			if (j == nopen)
			{
				struct PathNode nxt = {x, y, abs(goalx - x) + abs(goaly - y), 1, i ^ 1};

				assert((size_t) nnodes < arrayCount(nodes));
				assert((size_t) nopen < arrayCount(open));

				nodes[nnodes] = nxt;
				open[nopen++] = &nodes[nnodes++];
			}
			else if (cur->g + 1 < open[j]->g)
			{
				open[j]->g = cur->g + 1;
				open[j]->p = i ^ 1;
			}
		}
	}

	gp->npath = 0;
	return -1;
}

float posX(int x, int s) { return g_width * .5f - s * (g_world.nx * .5f) + x * s; }
float posY(int y, int s) { return g_height * .5f - s * (g_world.ny * .5f) + y * s; }
int unposX(float x, int s) { return (x + s *.5f - (g_width * .5f - s * (g_world.nx * .5f))) / s; }
int unposY(float y, int s) { return (y + s *.5f - (g_height * .5f - s * (g_world.ny * .5f))) / s; }

int checkWinConditions()
{
	for (int i = 0; i < g_world.ncrates; ++i)
		if (!isTargetTile(g_world.crates[i].x, g_world.crates[i].y))
			return 0;

	return 1;
}


int getTileSize(int tile)
{
	for(int i=0; i<g_world.ntextures; ++i)
	{
		if(tile < g_world.Textures[i].max_id)
			return g_world.Textures[i].num_tiles;
	}
	assert(0 && "Texture not found");
	return 0;
}

int getTexCoords(int tile, float *vu, float *vv, float *tilesize)
{
	int size = getTileSize(tile);
	*tilesize = 1.0f/size;
	int rel = (tile-1) % (size*size);
	*vv = (rel / size) * (*tilesize);
	*vu = (rel % size) * (*tilesize);
	return NUM_TEX + 1 + (tile/(size*size));
}

void gamePlay(float elapse, unsigned* stage)
{
	struct GamePlay* gp = &g_gamePlay;
	struct Player* p = &gp->player;
	struct Move* m = &gp->move;
	float ss = 32.f;

	if (!gp->init)
	{
		gp->init = 1;
		gp->finished = 0;
		gp->intro = -M_PI;
		gp->stepSaved = 0;
		gp->moves = 0;

		if (!g_gameStart.loadGameOnStart)
		{
			char* ext = strchr(g_map_progression[g_current_map].file, '.');

			if (strcmp(ext, ".tmx") == 0)
				loadMap_tmx(g_map_progression[g_current_map].file);
			else
				loadMap(g_map_progression[g_current_map].file);
			playMusic(g_map_progression[g_current_map].music);
		}

		p->ix = p->x = g_world.startx;
		p->iy = p->y = g_world.starty;
		p->path = -1;
		p->speed = 0.5f;

		if (g_gameStart.loadGameOnStart)
		{
			g_gameStart.loadGameOnStart = 0;
			loadGame();
			playMusic(g_map_progression[g_current_map].music);
		}
	}

	int winConditions = checkWinConditions();

	if (winConditions == 1) 
	{
		if (gp->finished == 0) 
		{
			playMusic("win.mp3");
			gp->finished = 1;
		}
		if (buttonDown(0))
		{
			if (g_map_progression[++g_current_map].par < 0)
				--(*stage);
			else
				*stage = ~0;
		}
		goto ignore_mouse_input;
	}

	if ((gp->timer.elapse += elapse) >= 1.f)
	{
		gp->timer.elapse -= 1.f;

		if (++gp->timer.seconds == 60) {
			gp->timer.seconds = 0;

			if (++gp->timer.minutes == 60) {
				gp->timer.minutes = 0;
				++gp->timer.hours;
			}
		}
	}

	gp->intro = min(gp->intro + 16.f * elapse, M_PI * 4.5f);

	if (keyDown('p'))
	{
		saveGame();
		--(*stage);
	}
	else if (keyDown('r'))
	{
		*stage = ~0;
	}
	else if (keyDown('u') && (gp->moves > 0 || p->path >= 0 || p->move))
	{
		if(p->path < 0 && p->move == 0)
			--gp->moves;
		else
		{
			p->move = 0;
			p->path = -1;
			m->x0 = -1;
			m->y0 = -1;
			m->x1 = -1;
			m->y1 = -1;
			m->dx = 0;
			m->dy = 0;
		}
		p->ix = p->x = gp->steps[gp->moves].playerx;
		p->iy = p->y = gp->steps[gp->moves].playery;
		memcpy(g_world.crates, gp->steps[gp->moves].crates, sizeof(struct Object) * g_world.ncrates);
	}

	if (gp->stepSaved == 0)
	{
		gp->stepSaved = 1;
		gp->steps[gp->moves].playerx = p->x;
		gp->steps[gp->moves].playery = p->y;
		memcpy(gp->steps[gp->moves].crates, g_world.crates, sizeof(struct Object) * g_world.ncrates);
	}

	if (p->path >= 0 || p->move)
		goto ignore_mouse_input;

	const int mx = unposX(g_mousex, ss);
	const int my = unposY(g_mousey, ss);

	if (buttonDown(0))
	{
		struct Object* crate = worldCrates(mx, my);

		m->x0 = -1;
		m->y0 = -1;

		if (crate != NULL)
		{
			m->x0 = mx;
			m->y0 = my;
			m->ci = crate - g_world.crates;
		}
	}

	if (buttonHeld(0))
	{
		m->x1 = -1;
		m->y1 = -1;

		if (mx != m->x0 && my == m->y0)
		{
			int i = min(mx, m->x0 + 1);
			int n = max(mx, m->x0 - 1);

			for (; i < n + 1; ++i)
				if (!isWalkable(i, my))
					break;

			if (i > n)
			{
				m->x1 = mx;
				m->y1 = my;
			}
		}
		else if (mx == m->x0 && my != m->y0)
		{
			int i = min(my, m->y0 + 1);
			int n = max(my, m->y0 - 1);

			for (; i < n + 1; ++i)
				if (!isWalkable(mx, i))
					break;

			if (i > n)
			{
				m->x1 = mx;
				m->y1 = my;
			}
		}
	}

	if (buttonUp(0))
	{
		m->dx = 0;
		m->dy = 0;

		if (m->x0 >= 0)
		{
			if (m->x0 == m->x1)
				m->dy = 1 | (m->y0 - m->y1) >> 31;
			else if (m->y0 == m->y1)
				m->dx = 1 | (m->x0 - m->x1) >> 31;
		}

		if (m->dx || m->dy)
		{
			if (p->x != m->x0 + m->dx || p->y != m->y0 + m->dy)
			{
				pathFind(p->x, p->y, m->x0 + m->dx, m->y0 + m->dy);

				p->time = 0.f;
				p->move = 0;

				if ((p->path = gp->npath - 1) >= 0)
				{
					p->ix = gp->path[p->path].x;
					p->iy = gp->path[p->path].y;

					float min_speed = 0.4f;
					float max_speed = 0.2f;
					float min_dist = 3;
					float max_dist = 18;
					p->speed = max_speed;

					if(p->path <= min_dist)
						p->speed = min_speed;
					else if (p->path < max_dist)
					{
						float r = 1.0f - ((p->path - min_dist) / (max_dist - min_dist));
						p->speed += (min_speed - max_speed) * r;
					}

						// Trigger sounds
#if _WIN32 || _MACOSX
						BASS_ChannelStop(p->sound_channel);
						HSAMPLE sample = BASS_SampleLoad(0, "data/walk.wav", 0, 0, 1, BASS_SAMPLE_LOOP);
						p->sound_channel = BASS_SampleGetChannel(sample, 0);
						BASS_ChannelPlay(p->sound_channel, 1);
#endif
				}
				else
				{
					m->x0 = -1;
					m->y0 = -1;
				}
			}
			else
			{
				p->ix = p->x - m->dx;
				p->iy = p->y - m->dy;
				p->time = 0.f;
				p->move = 1;
#if _WIN32 || _MACOSX
				BASS_ChannelStop(p->sound_channel);
				HSAMPLE sample = BASS_SampleLoad(0, "data/push.wav", 0, 0, 1, BASS_SAMPLE_LOOP);
				p->sound_channel = BASS_SampleGetChannel(sample, 0);
				BASS_ChannelPlay(p->sound_channel, 1);
#endif
			}
		}
	}

ignore_mouse_input:
	;

	if (p->path < 0 && !p->move)
		;
	else if ((p->time += elapse) >= p->speed)
	{
		p->time -= p->speed;

		if (p->path >= 0)
		{
			p->x = gp->path[p->path].x;
			p->y = gp->path[p->path].y;

			if (--p->path >= 0)
			{
				p->ix = gp->path[p->path].x;
				p->iy = gp->path[p->path].y;
			}
			else
			{
				p->ix = p->x - m->dx;
				p->iy = p->y - m->dy;
				p->move = 1;
				p->speed = 0.55f;
#if _WIN32 || _MACOSX
				BASS_ChannelStop(p->sound_channel);
				HSAMPLE sample = BASS_SampleLoad(0, "data/push.wav", 0, 0, 1, BASS_SAMPLE_LOOP);
				p->sound_channel = BASS_SampleGetChannel(sample, 0);
				BASS_ChannelPlay(p->sound_channel, 1);
#endif
			}
		}
		else if (p->move)
		{
			m->x0 -= m->dx;
			m->y0 -= m->dy;

			g_world.crates[m->ci].x -= m->dx;
			g_world.crates[m->ci].y -= m->dy;

			p->ix = (p->x -= m->dx) - m->dx;
			p->iy = (p->y -= m->dy) - m->dy;

			if (m->x0 == m->x1 && m->y0 == m->y1)
			{
				p->ix = p->x;
				p->iy = p->y;
				p->move = 0;
				++gp->moves;
				gp->stepSaved = 0;

				m->x0 = -1;
				m->y0 = -1;
				m->x1 = -1;
				m->y1 = -1;
				m->dx = 0;
				m->dy = 0;
#if _WIN32 || _MACOSX
				BASS_ChannelStop(p->sound_channel);
				if(isTargetTile(g_world.crates[m->ci].x, g_world.crates[m->ci].y))
				{
					HSAMPLE sample = BASS_SampleLoad(0, "data/success.wav", 0, 0, 1, 0);
					p->sound_channel = BASS_SampleGetChannel(sample, 0);
					BASS_ChannelPlay(p->sound_channel, 1);
				}
#endif
			}
		}
	}

	if (keyDown('0'))
	{
		srand(time(0) ^ g_map_progression[g_current_map].seed);
		g_map_progression[g_current_map].seed = (int) (rand() / (float) RAND_MAX * 3333.f);
	}

	srand(g_map_progression[g_current_map].seed);
	gprintf(.9f, .07f, 0x00ff00ff, "SEED %d", g_map_progression[g_current_map].seed);

	const float vx = floorf(rand() / (float) RAND_MAX / .5f) * .5f;
	const float vy = floorf(rand() / (float) RAND_MAX / .5f) * .5f;

	for (int n = 0; n < g_world.nlayers; ++n)
	{
		for (int y = 0; y < g_world.ny; ++y)
		{
			for (int x = 0; x < g_world.nx; ++x)
			{
				float vu,vv;
				const int tile = worldTile(n, x, y);

				if(g_world.nlayers > 1 && tile > 0)
				{
					float tilesize;
					int id = getTexCoords(tile, &vu, &vv, &tilesize);
					sprite(posX(x, ss), posY(y, ss), ss, id, vu, vv, vu + tilesize, vv + tilesize);
					
					//if(n == g_world.wallLayer)
					//{
					//	vu = vx + floorf((.3f + .3f * sinf(x)) * (rand() / (float) RAND_MAX / .5f)) * .25f;
					//	vv = vy + floorf((.5f + .2f * cosf(x)) * (rand() / (float) RAND_MAX / .5f)) * .25f;
					//	sprite(posX(x, ss), posY(y, ss), ss, TEX_WALLVARS, vu, vv, vu + .25f, vv + .25f);
					//}
				}
				else
				{
					vu = vx + floorf((.3f + .3f * sinf(x)) * (rand() / (float) RAND_MAX / .5f)) * .25f;
					vv = vy + floorf((.5f + .2f * cosf(x)) * (rand() / (float) RAND_MAX / .5f)) * .25f;

					if (tile == TILE_FLOOR)
						sprite(posX(x, ss), posY(y, ss), ss, TEX_FLOORVARS, vu, vv, vu + .25f, vv + .25f);
					else if (tile == TILE_WALL)
						sprite(posX(x, ss), posY(y, ss), ss, TEX_WALLVARS, vu, vv, vu + .25f, vv + .25f);
					else if (tile == TILE_TARGET)
						sprite(posX(x, ss), posY(y, ss), ss, TEX_TARGET_G, 0.f, 0.f, 1.f, 1.f);
				}
			}
		}
	}

	if (buttonHeld(0) || p->path >= 0)
	{
		const int x0 = min(m->x0, m->x1), x1 = max(m->x0, m->x1);
		const int y0 = min(m->y0, m->y1), y1 = max(m->y0, m->y1);

		if ((x0 == x1 && y0 != y1) || (x0 != x1 && y0 == y1))
		{
			for (int i = x0; i <= x1; ++i)
				quad(posX(i, ss), posY(y0, ss), ss * .25f, 0);

			for (int i = y0; i <= y1; ++i)
				quad(posX(x0, ss), posY(i, ss), ss * .25f, 0);
		}
	}

	const int dx = p->ix - p->x;
	const int dy = p->iy - p->y;
	const float ix = dx * ss * (p->time / p->speed);
	const float iy = dy * ss * (p->time / p->speed);

	for (int i = 0; i < g_world.ncrates; ++i)
	{
		const int x = g_world.crates[i].x;
		const int y = g_world.crates[i].y;
		float xx = posX(x, ss);
		float yy = posY(y, ss);
		float sm = 1.f;

		if (p->move && p->x - m->dx == x && p->y - m->dy == y)
		{
			xx += ix;
			yy += iy;
		}
		else if (m->x0 == x && m->y0 == y && buttonHeld(0))
			sm = 1.2f;

		const float t = gp->intro + (float) rand() / RAND_MAX * M_PI;
		const float w = min(t / (M_PI * 4.5f), 1.f);
		const float ss2 = ss * .5f + ss * (cosf(t) + 1.f) * .5f;
		const float ss3 = w * ss + (1.f - w) * ss2;

		if(g_world.nlayers > 1 && g_world.crateTex != 0)
		{
			int tex = g_world.crateTex;
			if (isTargetTile(x,y) && g_world.crateTargetTex != 0)
				tex = g_world.crateTargetTex;

			float vv, vu, tilesize;
			int id = getTexCoords(tex, &vu, &vv, &tilesize);
			sprite(xx, yy, ss3 * sm, id, vu, vv, vu + tilesize, vv + tilesize);
		}
		else
			sprite(xx, yy, ss3 * sm, TEX_CRATE, 0.f, 0.f, 1.f, 1.f);
	}

	const float s = dx || dy ? clamp(p->time / p->speed, 0.f, 1.f) : .25f;
	float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;

	if (dx < 0.f)
		texUvPlayer_left(u0, v0, u1, v1, s);
	else if (dx > 0.f)
		texUvPlayer_right(u0, v0, u1, v1, s);
	else if (dy < 0.f)
		texUvPlayer_up(u0, v0, u1, v1, s);
	else if (dy > 0.f)
		texUvPlayer_down(u0, v0, u1, v1, s);
	else
	{
		u0 = .5f, u1 = u0 + .125f;
		v0 = .25f, v1 = v0 + .125f;
	}

	if (p->move)
		v0 += .5f, v1 += .5f;

	sprite(posX(p->x, ss) + ix, posY(p->y, ss) + iy, ss, TEX_PLAYER, u0, v0, u1, v1);

	gprintf(.02f, .05f, 0x00ffffff, "LEVEL: %s", g_map_progression[g_current_map].name);
	gprintf(.02f, .07f, 0x00ffffff, "TIME:  %02d:%02d:%02d", gp->timer.hours, gp->timer.minutes, gp->timer.seconds);
	gprintf(.02f, .09f, 0x00ffffff, "MOVES: %d PAR: %d", gp->moves, g_map_progression[g_current_map].par);
	gprintf(.02f, .95f, 0x00ffffff, "[U]NDO [P]AUSE  [R]ESTART");

	if( g_map_progression[g_current_map].txt[0] != 0) 
	{
		float row = 0.05f;
		char txt[256];
		strncpy(txt, g_map_progression[g_current_map].txt, 256);

		char* tok = strtok(txt, "^");
		while(tok != NULL)
		{
			gprintf(0.5f - (strlen(tok) * 0.005f), row, 0x00ffffff, tok);
			row += 0.02f;
			tok = strtok(NULL, "^");
		}
	}

	if (winConditions == 1) 
	{
		float size = 210.f;
		float x = g_width * .5f;
		float y = g_height * .5f;
		quad(x, y, size, 0x00cf7f7f);

		float rxsize = (size*.37f) / g_width;
		float rysize = (size*.37f) / g_height;
		gprintf(0.5 - rxsize, 0.5 - rysize, 0x00ffffff, "* LEVEL COMPLETED *");
		gprintf(0.5 - rxsize, 0.5 - rysize + 0.05f, 0x00ffffff, " Click to continue");

		float u0, v0, u1, v1;
		u0 = .5f, u1 = u0 + .125f;
		v0 = .375f, v1 = v0 + .125f;
		sprite(x, y + 25.f, 128.f, TEX_PLAYER, u0, v0, u1, v1);
	}
}

///////////////////////////////////////////////////////////////////////////////

struct
{
	float time;
	int count;
} g_gameOver;

void gameOver(float elapse, unsigned* stage)
{
	gprintf(.5f, .5f, 0x00ff00ff, "You died");
	gprintf(.5f, .54f, 0x00ff00ff, "Exit in %d", 3 - g_gameOver.count);

	if ((g_gameOver.time += elapse) >= 1.f)
	{
		g_gameOver.time -= 1.f;

		if (++g_gameOver.count == 4)
			++(*stage);
	}
}

///////////////////////////////////////////////////////////////////////////////

struct
{
	void* data;
	size_t size;
	void (*func)(float, unsigned*);
} g_stages[] =
{
	{&g_gameStart, sizeof(g_gameStart), &gameStart},
	{&g_gamePlay, sizeof(g_gamePlay), &gamePlay},
	{&g_gameOver, sizeof(g_gameOver), &gameOver}
};

void renderGame(float elapse)
{
	static unsigned currentStage;
	unsigned stage = currentStage;

	(*g_stages[stage].func)(elapse, &stage);

	if (currentStage != stage)
	{
		if (stage == ~0u)
			stage = currentStage;

		memset(g_stages[stage].data, 0, g_stages[stage].size);
	}

	currentStage = stage;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void gprintf(float x, float y, unsigned c, const char* fmt, ...)
{
	va_list ap;
	unsigned n;
	char s[256];

	va_start(ap, fmt);
	n = vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);

	if ((size_t) (&g_textBuffer[sizeof(g_textBuffer)] - g_textEnd) >= sizeof(struct Text) + n + 1)
	{
		((struct Text*) g_textEnd)->x = x;
		((struct Text*) g_textEnd)->y = y;
		((struct Text*) g_textEnd)->color = c;
		((struct Text*) g_textEnd)->length = n;
		memcpy(((struct Text*) g_textEnd)->string, s, n + 1);

		g_textEnd += sizeof(struct Text) + n + 1;
	}
}

void quad(float x, float y, float s, unsigned c)
{
	glColor4ubv((unsigned char*) &c);

	glPushMatrix();
	glTranslatef(x, y, 0.f);

	glBegin(GL_QUADS);
		glVertex2f(-(s * .5f), s * .5f);
		glVertex2f(s * .5f, s * .5f);
		glVertex2f(s * .5f, -(s * .5f));
		glVertex2f(-(s * .5f), -(s * .5f));
	glEnd();

	glPopMatrix();
}

void sprite(float x, float y, float s, unsigned texId, float u0, float v0, float u1, float v1)
{
	glColor3f(1.f, 1.f, 1.f);

	glPushMatrix();
	glTranslatef(x, y, 0.f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_tex[texId]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		glTexCoord2f(u0, v1), glVertex2f(-(s * .5f), s * .5f);
		glTexCoord2f(u1, v1), glVertex2f(s * .5f, s * .5f);
		glTexCoord2f(u1, v0), glVertex2f(s * .5f, -(s * .5f));
		glTexCoord2f(u0, v0), glVertex2f(-(s * .5f), -(s * .5f));
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glPopMatrix();
}

void onKeyDown(unsigned char k, int x, int y)
{
	(void) x;
	(void) y;

	if (k == 0x1b)
		exit(0);

	g_keys[0][k >> 5] |= 1 << (k & 0x1f);
}

void onKeyUp(unsigned char k, int x, int y)
{
	(void) x;
	(void) y;

	g_keys[0][k >> 5] &= ~(1 << (k & 0x1f));
}

void onReshape(int w, int h)
{
	g_reshape = 1;
	g_width = w;
	g_height = h;

	glViewport(0, 0, w, h);
}

void onMouse(int button, int state, int x, int y)
{
	(void) x;
	(void) y;

	if (!state)
		g_buttons[0] |= 1 << button;
	else
		g_buttons[0] &= ~(1 << button);
}

void onMotion(int x, int y)
{
	g_mousex = x;
	g_mousey = y;
}

void onDisplay()
{
	// update timer

	unsigned long long clock;
	float elapse;

	tbcount(clock);
	elapse = (float) (((double) clock - (double) g_clock) * g_period);
	g_clock = clock;

	elapse = (1.f / 3.f) * elapse + (1.f - 1.f / 3.f) * g_elapse;
	g_elapse = elapse;

	gprintf(.9f, .05f, 0x00cf7f7f, "FPS %.02f", 1.f / elapse);

	// render game

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.f, g_width, g_height, 0.f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(.1f, .1f, .3f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	renderGame(elapse);

	// render font

	for (
		struct Text* t = (struct Text*) g_textBuffer;
		t != (struct Text*) g_textEnd;
		t = (struct Text*) &t->string[t->length + 1])
	{
		glPushAttrib(GL_LIST_BIT);
		glListBase(g_fontList);

		glColor4ub(0, 0, 0, 0);
		glRasterPos2i((int) (g_width * t->x) + 1, (int) (g_height * t->y) + 1);
		glCallLists(t->length, GL_UNSIGNED_BYTE, t->string);

		glColor4ubv((unsigned char*) &t->color);
		glRasterPos2i((int) (g_width * t->x), (int) (g_height * t->y));
		glCallLists(t->length, GL_UNSIGNED_BYTE, t->string);

		glPopAttrib();
	}

	g_textEnd = g_textBuffer;

	// end frame

	memcpy(g_keys[1], g_keys[0], sizeof(g_keys[0]));
	g_buttons[1] = g_buttons[0];
	g_reshape = 0;

	snooze(15);
	glutSwapBuffers();
	glutPostWindowRedisplay(g_window);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char* loadFile(const char *path, int *readed)
{
	FILE* fd;
	long size;
	char* data;

	printf("Loading [%s]\n", path);
	fd = fopen(path, "rb");
	if(fd == NULL)
	{
		printf(">>> ERROR OPENING FILE [%s] <<< ... blame Heiko", path);
		exit(1);
	}
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	rewind(fd);
	data = (char*) malloc(size);
	*readed = fread(data, 1, size, fd);
	fclose(fd);

	return data;
}

void loadTileProperties(mxml_node_t* tileset, mxml_node_t* tree)
{
	// Tile properties
	mxml_node_t *node, *property;
	mxml_index_t *index = mxmlIndexNew(tileset, "tile", NULL);

	while((node = mxmlIndexEnum(index)) != NULL)
	{
		int id = atoi(mxmlElementGetAttr(node, "id"));
		property = mxmlFindElement(node, tree, "property", NULL, NULL, MXML_DESCEND);
		if(property == NULL)
			continue;

		const char* name = mxmlElementGetAttr(property, "name");
		if(name == NULL)
			continue;

		if(stricmp(name, "crate") == 0)
			g_world.crateTex = id + 1;
		else if(stricmp(name, "cratetarget") == 0)
			g_world.crateTargetTex = id + 1;
		else
			printf("Unknown property for tile %d named [%s], blame Tony\n", id, name);

		mxmlDelete(property);
	}
	mxmlIndexDelete(index);		
}

void loadMap_tmx(const char* path)
{
		int n = 0;
		char data_path[PATH_NAME_SIZE] = DATA_DIR;
		char* data = loadFile(strcat(data_path, path), &n);

		memset(&g_world, 0, sizeof(g_world));

		mxml_node_t *tree = mxmlLoadString(NULL, data, MXML_TEXT_CALLBACK);
		mxml_node_t *map = mxmlFindElement(tree, tree, "map", NULL, NULL, MXML_DESCEND_FIRST);

		g_world.nx = atoi(mxmlElementGetAttr(map, "width"));
		g_world.ny = atoi(mxmlElementGetAttr(map, "height"));
		int tilesize = atoi(mxmlElementGetAttr(map, "tilewidth"));

		printf("Map of %dx%d, tile size: %d, approved by Senta\n", g_world.nx, g_world.ny, tilesize); 

		mxml_index_t *index, *index2;
		mxml_node_t *node, *subnode, *node2;

		// Tileset loading ---
		index = mxmlIndexNew(map, "tileset", NULL);

		while((node = mxmlIndexEnum(index)) != NULL)
		{
			int gid = atoi(mxmlElementGetAttr(node, "firstgid"));
			const char* tw = mxmlElementGetAttr(node, "tilewidth");
			if(tw == NULL)
				continue;
			int tilesize = atoi(tw);
			subnode = mxmlFindElement(node, tree, "image", NULL, NULL, MXML_DESCEND_FIRST);
			const char* src = mxmlElementGetAttr(subnode, "source");
			int w = atoi(mxmlElementGetAttr(subnode, "width"));
			int h = atoi(mxmlElementGetAttr(subnode, "height"));

			sprintf(data_path, "%s%s", DATA_DIR, src);
			char* ext = strrchr (data_path, '.');
			strcpy(ext, ".rgba");
			char* img = loadFile(data_path, &n);
			assert(n == w * h * 4);

			int num_subtextures = (w / tilesize);
			int tex_id = NUM_TEX + 1 + gid/(num_subtextures*num_subtextures);
			glGenTextures(1, &g_tex[tex_id]);
			glBindTexture(GL_TEXTURE_2D, g_tex[tex_id]);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
			free(img);

			g_world.Textures[g_world.ntextures].max_id = gid + (num_subtextures*num_subtextures);
			g_world.Textures[g_world.ntextures].num_tiles = num_subtextures;
			//printf("Texture with max_id %d contains %dx%d squares\n", g_world.Textures[g_world.ntextures].max_id, num_subtextures, num_subtextures);
			g_world.ntextures++;

			loadTileProperties(node, tree);
			mxmlDelete(subnode);
		}
		mxmlIndexDelete(index);		

		// Layer loading ---
		index = mxmlIndexNew(map, "layer", NULL);

		while((node = mxmlIndexEnum(index)) != NULL)
		{
			const char* name = mxmlElementGetAttr(node, "name");
			subnode = mxmlFindElement(node, tree, "data", NULL, NULL, MXML_DESCEND_FIRST);
			index2 = mxmlIndexNew(subnode, "tile", NULL);
			int i = 0;
			while((node2 = mxmlIndexEnum(index2)) != NULL)
			{
				g_world.tiles[g_world.nlayers][i++] = atoi(mxmlElementGetAttr(node2, "gid"));
			}
			if(stricmp("Walls", name) == 0)
			{
				g_world.wallLayer = g_world.nlayers; 
				printf("Found wall in layer %d, Malte stills hates XML\n", g_world.wallLayer);
			}
			g_world.nlayers++;
			mxmlIndexDelete(index2);
			mxmlDelete(subnode);
		}
		mxmlIndexDelete(index);

		// Objects loading ---
		// Only supports one ¨objectgroup¨. Inside it only 3 types of objects are valid:
		// - start: position were the player starts
		// - crate: a crate will be created in that position
		// - target: all targets need to have a crate in order to finish a level

		mxml_node_t *objectsgroup = mxmlFindElement(map, tree, "objectgroup", NULL, NULL, MXML_DESCEND_FIRST);
		index = mxmlIndexNew(objectsgroup, "object", NULL);

		while((node = mxmlIndexEnum(index)) != NULL)
		{
			int x = atoi(mxmlElementGetAttr(node, "x")) / tilesize;
			int y = atoi(mxmlElementGetAttr(node, "y")) / tilesize;
			const char* type = mxmlElementGetAttr(node, "type");
			if(strcmp(type, "start") == 0)
			{
				g_world.startx = x;
				g_world.starty = y;
			}
			if(strcmp(type, "crate") == 0)
			{
				g_world.crates[g_world.ncrates].x = x;
				g_world.crates[g_world.ncrates].y = y;
				g_world.ncrates++;
			}
			if(strcmp(type, "target") == 0)
			{
				g_world.targets[g_world.ntargets].x = x;
				g_world.targets[g_world.ntargets].y = y;
				g_world.ntargets++;
			}
		}
		mxmlIndexDelete(index);

		// ----------------------
		mxmlDelete(tree);
		free(data);
}

void loadMap(const char* path)
{
		int n = 0;
		char data_path[PATH_NAME_SIZE] = DATA_DIR;
		char* data = loadFile(strcat(data_path, path), &n);

		memset(&g_world, 0, sizeof(g_world));

		struct Object* crate = g_world.crates;
		struct Object* target = g_world.targets;
		unsigned char* dst = g_world.tiles[0];
		char* src = data;

		for (int i = 0, x = 0; i < n; ++i)
		{
			const char c = *src++;

			if (c == TILE_CRATE_TARGET || c == TILE_START_TARGET || c == TILE_TARGET)
				target->x = x, (target++)->y = g_world.ny;

			if (c == TILE_VOID || c == TILE_WALL || c == TILE_FLOOR || c == TILE_TARGET)
				++x, *dst++ = c;
			else if (c == TILE_CRATE)
				crate->x = x++, (crate++)->y = g_world.ny, *dst++ = TILE_FLOOR;
			else if (c == TILE_CRATE_TARGET)
				crate->x = x++, (crate++)->y = g_world.ny, *dst++ = TILE_TARGET;
			else if (c == TILE_START)
				g_world.startx = x++, g_world.starty = g_world.ny, *dst++ = TILE_FLOOR;
			else if (c == TILE_START_TARGET)
				g_world.startx = x++, g_world.starty = g_world.ny, *dst++ = TILE_TARGET;
			else if (c == '\n')
				x = 0, ++g_world.ny;

			g_world.nx = max(g_world.nx, x);
		}

		g_world.ncrates = crate - g_world.crates;
		g_world.ntargets = target - g_world.targets;
		g_world.nlayers = 1;
		free(data);
}

void saveGame()
{
	FILE* fd;
	int ver = SAVE_GAME_VERSION;

	if ((fd = fopen(SAVE_GAME_PATH, "wb")) != NULL)
	{
		fwrite(&ver, sizeof(ver), 1, fd);
		fwrite(&g_current_map, sizeof(g_current_map), 1, fd);
		fwrite(&g_world, sizeof(g_world), 1, fd);
		fwrite(&g_gamePlay, sizeof(g_gamePlay), 1, fd);
		fclose(fd);
	}
	else fprintf(stderr, "Failed to save game state\n");
}

void loadGame()
{
	FILE* fd;
	int ver = -1;

	if ((fd = fopen(SAVE_GAME_PATH, "rb")) != NULL)
	{
		if (fread(&ver, sizeof(ver), 1, fd) != 1)
			fprintf(stderr, "Failed to read version\n");

		if (ver == SAVE_GAME_VERSION)
		{
			if (fread(&g_current_map, sizeof(g_current_map), 1, fd) != 1)
				fprintf(stderr, "Failed to read map id\n");

			if (fread(&g_world, sizeof(g_world), 1, fd) != 1)
				fprintf(stderr, "Failed to read world\n");

			if (fread(&g_gamePlay, sizeof(g_gamePlay), 1, fd) != 1)
				fprintf(stderr, "Failed to read gameplay\n");
		}
		else fprintf(stderr, "Version mismatch in game state\n");

		fclose(fd);
	}
	else fprintf(stderr, "Failed to load game state\n");
}

#define CFG_HEADER ("map")
#define CFG_NAME ("name")
#define CFG_MUSIC ("music")
#define CFG_BG ("bg")
#define CFG_FILE ("file")
#define CFG_SEED ("seed")
#define CFG_PAR ("par")
#define CFG_TXT ("txt")

void loadConfig(const char* path)
{
	int n = 0;
	char* data = loadFile(path, &n);
	int map = -1; 

	for (int i = 0; i < n; ++i)
		if (data[i] == '\n' || data[i] == '\r')
			data[i] = 0;

	for (int i = 0; i < n; ++i)
	{
		if (strncmp(&data[i], CFG_HEADER, sizeof(CFG_HEADER) - 1) == 0)
		{
			++map;
			i += sizeof(CFG_HEADER) - 1;
		}
		else if (strncmp(&data[i], CFG_FILE, sizeof(CFG_FILE) - 1) == 0)
		{
			i += sizeof(CFG_FILE);
			strncpy(g_map_progression[map].file, &data[i], PATH_NAME_SIZE);
			i += strlen(g_map_progression[map].file);
		}
		else if (strncmp(&data[i], CFG_TXT, sizeof(CFG_TXT) - 1) == 0)
		{
			i += sizeof(CFG_TXT);
			strncpy(g_map_progression[map].txt, &data[i], 256);
			i += strlen(g_map_progression[map].txt);
		}
		else if (strncmp(&data[i], CFG_NAME, sizeof(CFG_NAME) - 1) == 0)
		{
			i += sizeof(CFG_NAME);
			strncpy(g_map_progression[map].name, &data[i], PATH_NAME_SIZE);
			i += strlen(g_map_progression[map].name);
		}
		else if (strncmp(&data[i], CFG_MUSIC, sizeof(CFG_MUSIC) - 1) == 0)
		{
			i += sizeof(CFG_MUSIC);
			strncpy(g_map_progression[map].music, &data[i], PATH_NAME_SIZE);
			i += strlen(g_map_progression[map].music);
		}				
		else if (strncmp(&data[i], CFG_BG, sizeof(CFG_BG) - 1) == 0)
		{
			i += sizeof(CFG_BG);
			strncpy(g_map_progression[map].bg, &data[i], PATH_NAME_SIZE);
			i += strlen(g_map_progression[map].bg);
		}
		else if (strncmp(&data[i], CFG_SEED, sizeof(CFG_SEED) - 1) == 0)
		{
			i += sizeof(CFG_SEED);
			g_map_progression[map].seed = atoi(&data[i]);
		}
		else if (strncmp(&data[i], CFG_PAR, sizeof(CFG_PAR) - 1) == 0)
		{
			i += sizeof(CFG_PAR);
			g_map_progression[map].par = atoi(&data[i]);
		}
	}

	free(data);

	g_map_progression[++map].par = -1; // watermark
}

void onFile(char* path)
{
	char* ext;
	char* data;
	int n;

	if ((ext = strchr(path, '.')) == NULL)
		return;

	if (strcmp(ext, ".rgba") == 0)
	{
		char name[64];
		int w, h, i;

		if (sscanf(path, "%*[^/]/%64[^_]_%dx%d", name, &w, &h) != 3)
			return;

		for (i = 0; i < NUM_TEX; ++i)
			if (strcmp(g_texnames[i], name) == 0)
				break;

		if (i == NUM_TEX)
			return;

		data = loadFile(path, &n);
		assert(n == w * h * 4);

		glGenTextures(1, &g_tex[i]);
		glBindTexture(GL_TEXTURE_2D, g_tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}
	else if (strcmp(ext, ".cfg") == 0)
	{
		loadConfig(path);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void init(int* argc, char* argv[])
{
	// init timer

	unsigned long long freq;

	tbcount(g_clock);
	tbfreq(freq);
	g_period = 1. / (double) freq;
	g_elapse = 1.f / 60.f;

	// init glut

	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	const int scrw = glutGet(GLUT_SCREEN_WIDTH);
	const int scrh = glutGet(GLUT_SCREEN_HEIGHT);

	glutInitWindowPosition(scrw / 4, scrh / 4);
	glutInitWindowSize(scrw / 2, scrh / 2);

	g_window = glutCreateWindow("Touchy Warehouse Keeper - PC");

	glutKeyboardFunc(&onKeyDown);
	glutKeyboardUpFunc(&onKeyUp);
	glutMouseFunc(&onMouse);
	glutMotionFunc(&onMotion);
	glutPassiveMotionFunc(&onMotion);
	glutReshapeFunc(&onReshape);
	glutDisplayFunc(&onDisplay);

	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// init font

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	g_fontList = glGenLists(256);

	for (int i = 0, j = ' '; j < 256; i += 2, ++j)
	{
		glNewList(g_fontList + j, GL_COMPILE);
		glBitmap(8, 8, 0.f, 0.f, 8.f, 0.f, (unsigned char*) &g_font[i]);
		glEndList();
	}

	g_textEnd = g_textBuffer;

	// init bass

#if _WIN32 || _MACOSX
	BASS_Init(-1, 44100, 0, 0, NULL);
	if (g_mute == 1)
		BASS_SetVolume(0.0f);
#endif

	// load data

	_foreach_file("data", onFile);
}

void end()
{
#if _WIN32 || _MACOSX
	BASS_Free();
#endif
	glutDestroyWindow(g_window);
}

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-mute"))
			g_mute = 1;
	}

	init(&argc, argv);
	atexit(&end);

	glutMainLoop();
	return 0;
}
