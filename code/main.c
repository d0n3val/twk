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
# include <strings.h>
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
# define uint unsigned int
#endif
#if __linux__ || _WIN32 || __APPLE__
# include "mxml/mxml.h"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#if 1
#include "uilayoutlang.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *stbi_load_from_memory(
	unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);

#ifdef __cplusplus
}
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

#define sgn(a) ((a) > 0 ? 1 : (a) < 0 ? -1 : 0)

#define sqr(a) ((a) * (a))

#define keyDown(k) ((g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f))) && !(g_keys[1][(k) >> 5] & (1 << ((k) & 0x1f))))
#define keyUp(k) (!(g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f))) && (g_keys[1][(k) >> 5] & (1 << ((k) & 0x1f))))
#define keyHeld(k) (g_keys[0][(k) >> 5] & (1 << ((k) & 0x1f)))

#define buttonDown(b) ((g_buttons[0] & (1 << (b))) && !(g_buttons[1] & (1 << (b))))
#define buttonUp(b) (!(g_buttons[0] & (1 << (b))) && (g_buttons[1] & (1 << (b))))
#define buttonHeld(b) (g_buttons[0] & (1 << (b)))

#if __linux__ || _MACOSX
# define stricmp(s,t) strcasecmp((s), (t))
# define strnicmp(s,t,n) strncasecmp((s), (t), (n))
#endif

///////////////////////////////////////////////////////////////////////////////

struct Pref
{
	char name[16];
	char value[64];
};

unsigned g_nprefs;
struct Pref g_prefs[32];

///////////////////////////////////////////////////////////////////////////////

typedef float vec3[3];
typedef float vec4[4];
typedef float mat4[16];

void mat4Identity(float* d)
{
	d[0] = 1.f; d[1] = 0.f; d[2] = 0.f; d[3] = 0.f;
	d[4] = 0.f; d[5] = 1.f; d[6] = 0.f; d[7] = 0.f;
	d[8] = 0.f; d[9] = 0.f; d[10] = 1.f; d[11] = 0.f;
	d[12] = 0.f; d[13] = 0.f; d[14] = 0.f; d[15] = 1.f;
}

void mat4Mul(float* __restrict d, float* __restrict s, float* __restrict t)
{
	d[0] = s[0] * t[0] + s[1] * t[4] + s[2] * t[8] + s[3] * t[12];
	d[1] = s[0] * t[1] + s[1] * t[5] + s[2] * t[9] + s[3] * t[13];
	d[2] = s[0] * t[2] + s[1] * t[6] + s[2] * t[10] + s[3] * t[14];
	d[3] = s[0] * t[3] + s[1] * t[7] + s[2] * t[11] + s[3] * t[15];
	d[4] = s[4] * t[0] + s[5] * t[4] + s[6] * t[8] + s[7] * t[12];
	d[5] = s[4] * t[1] + s[5] * t[5] + s[6] * t[9] + s[7] * t[13];
	d[6] = s[4] * t[2] + s[5] * t[6] + s[6] * t[10] + s[7] * t[14];
	d[7] = s[4] * t[3] + s[5] * t[7] + s[6] * t[11] + s[7] * t[15];
	d[8] = s[8] * t[0] + s[9] * t[4] + s[10] * t[8] + s[11] * t[12];
	d[9] = s[8] * t[1] + s[9] * t[5] + s[10] * t[9] + s[11] * t[13];
	d[10] = s[8] * t[2] + s[9] * t[6] + s[10] * t[10] + s[11] * t[14];
	d[11] = s[8] * t[3] + s[9] * t[7] + s[10] * t[11] + s[11] * t[15];
	d[12] = s[12] * t[0] + s[13] * t[4] + s[14] * t[8] + s[15] * t[12];
	d[13] = s[12] * t[1] + s[13] * t[5] + s[14] * t[9] + s[15] * t[13];
	d[14] = s[12] * t[2] + s[13] * t[6] + s[14] * t[10] + s[15] * t[14];
	d[15] = s[12] * t[3] + s[13] * t[7] + s[14] * t[11] + s[15] * t[15];
}

static void mat4Ortho(float* m, float left, float right, float bottom, float top, float znear, float zfar)
{
	float width = right - left;
	float height = top - bottom;
	float depth = zfar - znear;

	float x = (right + left) / width;
	float y = (top + bottom) / height;
	float z = (zfar + znear) / depth;

	m[0] = 2.f / width;
	m[1] = 0.f;
	m[2] = 0.f;
	m[3] = 0.f;
	m[4] = 0.f;
	m[5] = 2.f / height;
	m[6] = 0.f;
	m[7] = 0.f;
	m[8] = 0.f;
	m[9] = 0.f;
	m[10] = 2.f / depth;
	m[11] = 0.f;
	m[12] = -x;
	m[13] = -y;
	m[14] = -z;
	m[15] = 1.f;
}

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

stbtt_bakedchar g_fontData[96];
unsigned g_fontTex;
float g_fontSpace;

struct Text
{
	float x, y;
	unsigned color, length;
	char string[0];
};

char g_textBuffer[2048];
char* g_textEnd;

#if 1
struct UllWidget* g_uiScreens;
#endif

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

#define NUM_MAP_TEX (16)

const char* g_texnames[] =
{
	"crate",
	"<player>",
	"wallfloor",
	"wall-vars",
	"floor-vars",
	"floor-g",
	"floor-p",
	"target-g",
	"target-p",
	"cloud"
};

unsigned g_tex[NUM_TEX + NUM_MAP_TEX];
unsigned char g_validMapTex[(NUM_MAP_TEX / 8) + 1];

void createMapTex(unsigned id, const char *data, int size, int width, int height) 
{
	int comp;
	unsigned char* image_data = stbi_load_from_memory(
		(const unsigned char*) data, size, &width, &height, &comp, 0);

	glGenTextures(1, &g_tex[id]);
	glBindTexture(GL_TEXTURE_2D, g_tex[id]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	g_validMapTex[(id - NUM_TEX) / 8] |= 1 << (id - NUM_TEX) % 8;
	free(image_data);
}

void destroyMapTex()
{
	for (unsigned i = 0; i < NUM_MAP_TEX; ++i)
		if (g_validMapTex[i / 8] & (1 << i % 8))
			glDeleteTextures(1, &g_tex[NUM_TEX + i]);

	memset(g_validMapTex, 0, sizeof(g_validMapTex));
}

///////////////////////////////////////////////////////////////////////////////

#define PLAYER_ANIM_TILE_WALKRIGHT (0)
#define PLAYER_ANIM_TILE_WALKLEFT (2)
#define PLAYER_ANIM_TILE_WALKDOWN (4)
#define PLAYER_ANIM_TILE_WALKUP (6)
#define PLAYER_ANIM_TILE_PUSHRIGHT (1)
#define PLAYER_ANIM_TILE_PUSHLEFT (3)
#define PLAYER_ANIM_TILE_PUSHDOWN (5)
#define PLAYER_ANIM_TILE_PUSHUP (7)
#define PLAYER_ANIM_TILE_WALKDOWNRIGHT (8)
#define PLAYER_ANIM_TILE_WALKDOWNLEFT (9)
#define PLAYER_ANIM_TILE_WALKUPRIGHT (10)
#define PLAYER_ANIM_TILE_WALKUPLEFT (11)

const char* g_playerAnimTileNames[] = {
	"walkright", "pushright", "walkleft", "pushleft",
	"walkdown", "pushdown", "walkup", "pushup",
	"walkdownright", "walkdownleft", "walkupright", "walkupleft"
};

struct PlayerAnimInfo {
	char width, height;
	char tileWidth, tileHeight;
	char source[0];
};

#define PLAYER_ANIM_FRAME_COUNT (6)

struct PlayerAnimTile {
	struct {
		float s0, t0;
		float s1, t1;
	} frames[PLAYER_ANIM_FRAME_COUNT];
};

struct PlayerAnimInfo* g_playerAnimInfo;
struct PlayerAnimTile* g_playerAnimTiles;

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
int g_current_map;
int g_isMenuMap;

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

#define TARGET_TARGET (0)
#define TARGET_NEWGAME (1)
#define TARGET_CONTINUEGAME (2)
#define TARGET_HISCORES (3)
#define TARGET_OPTIONS (4)

#define MAX_FLOORS 4
#define MAX_RAMPS 2
#define MAX_CRATES 32
#define MAX_LAYERS 10
#define MAX_TEXTURES 20

struct Crate
{
	int x, y;
};

struct Target
{
	int x, y;
	int type;
};

struct Ramp
{
	int x, y;
	int dir;
};

struct TexData
{
	int max_id;
	int num_tiles;
};

struct Floor
{
	int ncrates;
	struct Crate crates[MAX_CRATES];

	int ntargets;
	struct Target targets[MAX_CRATES];

	int nramps;
	struct Ramp ramps[MAX_RAMPS];

	int nlayers;
	int wallLayer;
	unsigned char tiles[MAX_LAYERS][4096];
};

struct World
{
	int ntextures;
	struct TexData Textures[MAX_TEXTURES];
	int crateTex;
	int crateTargetTex;

	int nx, ny;
	int startx, starty;
	int startFloor;

	struct Floor floors[MAX_FLOORS];
};

struct World g_world;

int worldTile(int fid, int layer, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];

	return x >= 0 && y >= 0 ? f->tiles[layer][y * g_world.nx + x] : TILE_UNKNOWN;
}

struct Crate* worldCrates(int fid, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];

	for (int i = 0; i < f->ncrates; ++i)
		if (f->crates[i].x == x && f->crates[i].y == y)
			return &f->crates[i];

	return NULL;
}

struct Target* worldTargets(int fid, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];

	for (int i = 0; i < f->ntargets; ++i)
		if (f->targets[i].x == x && f->targets[i].y == y)
			return &f->targets[i];

	return NULL;
}

int isWall(int fid, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];
	int w = worldTile(fid, f->wallLayer, x, y);

	if (f->nlayers > 1)
		return w != 0;
	else
		return w == TILE_WALL || w == TILE_VOID;
}

int isWalkable(int fid, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];
	int w = worldTile(fid, f->wallLayer, x, y);

	if (f->nlayers > 1)
		return w == 0;
	else
		return w == TILE_FLOOR || w == TILE_TARGET;
}

int isTargetTile(int fid, int x, int y)
{
	struct Floor* f = &g_world.floors[fid];

	if (f->nlayers > 1)
		return worldTargets(fid, x, y) != NULL;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

#define SAVE_GAME_VERSION (2)
#define SAVE_GAME_PATH ("data/save.bin")

void gprintf(float x, float y, unsigned c, const char* fmt, ...);
void quad(float x, float y, float s, unsigned c);
void rect(float x, float y, float w, float h, unsigned c);
void sprite(float x, float y, float s, unsigned texId, float u0, float v0, float u1, float v1);

void drawUi(const char* name);

char* loadFile(const char *path, int *readed);
void loadMap(const char* path);
void loadMap_tmx(const char* path);
void saveGame();
void loadGame();
void loadConfig(const char* path);

///////////////////////////////////////////////////////////////////////////////

#if _WIN32 || _MACOSX
void* g_musicData;
HSTREAM g_musicStream;
HSAMPLE g_walkSample;
HSAMPLE g_pushSample;
HSAMPLE g_successSample;
#endif
unsigned g_mute;

void playMusic(const char* path)
{
	(void) path;

#if _WIN32 || _MACOSX
	char data_path[PATH_NAME_SIZE] = DATA_DIR;
	strcat(data_path, path);

	printf("Loading: %s\n", data_path);

	if (g_musicStream != 0)
	{
		BASS_ChannelStop(g_musicStream);
		BASS_StreamFree(g_musicStream);
		g_musicStream = 0;
		free(g_musicData);
	}

	if (!g_mute)
	{
		int n;
		g_musicData = (void*) loadFile(data_path, &n);
		g_musicStream = BASS_StreamCreateFile(TRUE, g_musicData, 0, n, BASS_SAMPLE_LOOP);
		BASS_ChannelPlay(g_musicStream, 0);
	}
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

#define GAME_PLAY (0)

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
		playMusic("music/menu.mp3");
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
	const int f = (int) (.60f / .15f);
	const int t = PLAYER_ANIM_TILE_WALKDOWN;

	u0 = g_playerAnimTiles[t].frames[f].s0;
	v0 = g_playerAnimTiles[t].frames[f].t0;
	u1 = g_playerAnimTiles[t].frames[f].s1;
	v1 = g_playerAnimTiles[t].frames[f].t1;

	sprite(gs->px, gs->py, 128.f, TEX_PLAYER, u0, v0, u1, v1);

	static const unsigned colors[] = {0x0000ff00, 0x00ffffff};
	unsigned color = colors[gs->state];

#if 1
	// draw ui

	struct UllWidget* w;

	if ((w = g_uiScreens) != NULL && (w = ullFindScreen(w, "main-menu")) != NULL)
		while (w = ullNextWidget(w), w->type != 'X')
		{
			char *s = NULL, t[256];
			*t = 0;

			while ((s = ullFormat(t, ullText(w), s)) != NULL)
					if (strnicmp(s, "level", 5) == 0)
						s = g_map_progression[g_current_map].name;

			gprintf(ullX(w) / 100.f, ullY(w) / 100.f, ~0, t);
		}
#endif

	if (gs->hasSaveGame)
		gprintf(.02f, .95f, 0x00ffffff, "[C]ONTINUE");

	if ((gs->blink += elapse) >= 1.f)
		gs->blink -= 1.f, gs->state ^= 1;

	if (buttonDown(2))
	{
		++g_current_map;
		if (g_map_progression[g_current_map].par < 0)
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
	int fx, fy;
	int path;
	int turn;
	float turntime;
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
	struct Crate crates[MAX_CRATES];
	int floorId;
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
	int floorId;
	int targetFloor;
	int floorChange;
	float floorFade;
	struct Move move;
	int npath;
	struct PathNode path[128];
	int stepSaved;
	struct UndoStep steps[64];
	int cmd;
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

			if (isWall(gp->floorId, x, y) || worldCrates(gp->floorId, x, y))
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

int checkMenuOptions()
{
	struct GamePlay* gp = &g_gamePlay;
	int fid = gp->floorId;
	struct Floor* f = &g_world.floors[fid];
	struct Target* t;

	for (int i = 0; i < f->ncrates; ++i)
		if ((t = worldTargets(fid, f->crates[i].x, f->crates[i].y)) != NULL)
			return t->type;

	return -1;
}

int checkWinConditions()
{
	struct GamePlay* gp = &g_gamePlay;
	int fid = gp->floorId;
	struct Floor* f = &g_world.floors[fid];

	for (int i = 0; i < f->ncrates; ++i)
		if (!isTargetTile(fid, f->crates[i].x, f->crates[i].y))
			return 0;

	return 1;
}

int getTileSize(int tile)
{
	for (int i = 0; i < g_world.ntextures; ++i)
		if (tile < g_world.Textures[i].max_id)
			return g_world.Textures[i].num_tiles;

	assert(0 && "Texture not found");
	return 0;
}

int getTexCoords(int tile, float *vu, float *vv, float *tilesize)
{
	int size = getTileSize(tile);
	*tilesize = 1.f / size;

	int rel = (tile - 1) % (size * size);
	*vv = (rel / size) * (1.f / size);
	*vu = (rel % size) * (1.f / size);

	return NUM_TEX + tile / sqr(size);
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
			destroyMapTex();

			char* ext = strchr(g_map_progression[g_current_map].file, '.');

			if (stricmp(ext, ".tmx") == 0)
				loadMap_tmx(g_map_progression[g_current_map].file);
			else
				loadMap(g_map_progression[g_current_map].file);

			playMusic(g_map_progression[g_current_map].music);
		}

		gp->floorId = g_world.startFloor;
		gp->targetFloor = g_world.startFloor;
		gp->floorFade = 1.f;

		p->ix = p->x = g_world.startx;
		p->iy = p->y = g_world.starty;
		p->fx = 0;
		p->fy = 1;
		p->path = -1;
		p->speed = 0.5f;

		if (g_gameStart.loadGameOnStart)
		{
			g_gameStart.loadGameOnStart = 0;

			loadGame();
			playMusic(g_map_progression[g_current_map].music);
		}

		g_isMenuMap = !strcmp(g_map_progression[g_current_map].name, "Main Menu");
	}

	const int mx = unposX(g_mousex, ss);
	const int my = unposY(g_mousey, ss);
	int winConditions = 0;

	if (g_isMenuMap) {
		switch (checkMenuOptions()) {
		case TARGET_NEWGAME:
			g_current_map = 1;
			memset(&g_gamePlay, 0, sizeof g_gamePlay);
			break;

		case TARGET_CONTINUEGAME:
			g_current_map = 1;
			g_gameStart.loadGameOnStart = 1;
			memset(&g_gamePlay, 0, sizeof g_gamePlay);
			break;
		}
	}
	else {
		if (gp->floorId == gp->targetFloor)
			winConditions = checkWinConditions();

		if (winConditions == 1) {
			if (gp->finished == 0) {
				playMusic("music/win.mp3");
				gp->finished = 1;
			}

			if (buttonDown(0)) {
				if (g_map_progression[++g_current_map].par < 0)
					--(*stage);
				else
					*stage = ~0;
			}

			goto ignore_mouse_input;
		}
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

	if (gp->cmd == 'p') {
		gp->cmd = 0;
		saveGame();
		--(*stage);
	} else if (gp->cmd == 'r') {
		gp->cmd = 0;
		*stage = ~0;
	} else if (gp->cmd == 'u' && (gp->moves > 0 || p->path >= 0 || p->move)) {
		gp->cmd = 0;

		if (p->path < 0 && p->move == 0)
			--gp->moves;
		else {
			p->move = 0;
			p->path = -1;
			m->x0 = -1;
			m->y0 = -1;
			m->x1 = -1;
			m->y1 = -1;
			m->dx = 0;
			m->dy = 0;
		}

		struct Floor* f = &g_world.floors[gp->floorId];

		p->ix = p->x = gp->steps[gp->moves].playerx;
		p->iy = p->y = gp->steps[gp->moves].playery;
		memcpy(f->crates, gp->steps[gp->moves].crates, sizeof(struct Crate) * f->ncrates);
	}

	if (gp->stepSaved == 0)
	{
		struct Floor* f = &g_world.floors[gp->floorId];

		gp->stepSaved = 1;
		gp->steps[gp->moves].floorId = gp->floorId;
		gp->steps[gp->moves].playerx = p->x;
		gp->steps[gp->moves].playery = p->y;
		memcpy(gp->steps[gp->moves].crates, f->crates, sizeof(struct Crate) * f->ncrates);
	}

	if (p->path >= 0 || p->move)
		goto ignore_mouse_input;

	if (buttonDown(0))
	{
		struct Crate* crate = worldCrates(gp->floorId, mx, my);
		struct Floor* f = &g_world.floors[gp->floorId];

		m->x0 = -1;
		m->y0 = -1;

		if (crate != NULL)
		{
			m->x0 = mx;
			m->y0 = my;
			m->ci = crate - f->crates;
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
				if (!isWalkable(gp->floorId, i, my))
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
				if (!isWalkable(gp->floorId, mx, i))
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

					float min_speed = 0.33f;
					float max_speed = 0.25f;
					float min_dist = 3;
					float max_dist = 18;

					p->speed = max_speed;

					if (p->path <= min_dist)
						p->speed = min_speed;
					else if (p->path < max_dist)
					{
						float r = 1.0f - ((p->path - min_dist) / (max_dist - min_dist));
						p->speed += (min_speed - max_speed) * r;
					}

					// Trigger sounds
#if _WIN32 || _MACOSX
					if (!g_mute)
					{
						BASS_ChannelStop(p->sound_channel);
						p->sound_channel = BASS_SampleGetChannel(g_walkSample, 0);
						BASS_ChannelPlay(p->sound_channel, 1);
					}
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
				if (!g_mute)
				{
					BASS_ChannelStop(p->sound_channel);
					p->sound_channel = BASS_SampleGetChannel(g_pushSample, 0);
					BASS_ChannelPlay(p->sound_channel, 1);
				}
#endif
			}
		}
	}

ignore_mouse_input:
	;

	if ((p->path < 0 && !p->move) || p->turn)
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
				p->turn = 1;
				p->turntime = .04f;
			}
			else
			{
				p->ix = p->x - m->dx;
				p->iy = p->y - m->dy;
				p->move = 1;
				p->speed = 0.5f;

#if _WIN32 || _MACOSX
				if (!g_mute)
				{
					BASS_ChannelStop(p->sound_channel);
					p->sound_channel = BASS_SampleGetChannel(g_pushSample, 0);
					BASS_ChannelPlay(p->sound_channel, 1);
				}
#endif
			}
		}
		else if (p->move)
		{
			struct Floor* f = &g_world.floors[gp->floorId];

			m->x0 -= m->dx;
			m->y0 -= m->dy;

			f->crates[m->ci].x -= m->dx;
			f->crates[m->ci].y -= m->dy;

			p->ix = (p->x -= m->dx) - m->dx;
			p->iy = (p->y -= m->dy) - m->dy;

			if (m->x0 == m->x1 && m->y0 == m->y1)
			{
				p->ix = p->x;
				p->iy = p->y;
				p->move = 0;
				++gp->moves;
				gp->stepSaved = 0;

				for (int i = 0; i < f->nramps; ++i)
					if (m->x1 == f->ramps[i].x && m->y1 == f->ramps[i].y)
					{
						int fid = gp->floorId + f->ramps[i].dir;
						struct Floor* f2 = &g_world.floors[fid];

						memcpy(
							&f2->crates[f2->ncrates++],
							&f->crates[m->ci],
							sizeof(struct Crate));

						if (--f->ncrates > 0)
							memcpy(
								&f->crates[m->ci],
								&f->crates[f->ncrates],
								sizeof(struct Crate));

						gp->targetFloor = fid;
						gp->floorChange = f->ramps[i].dir;
					}

				m->x0 = -1;
				m->y0 = -1;
				m->x1 = -1;
				m->y1 = -1;
				m->dx = 0;
				m->dy = 0;

#if _WIN32 || _MACOSX
				if (!g_mute)
				{
					BASS_ChannelStop(p->sound_channel);

					if (isTargetTile(gp->floorId, f->crates[m->ci].x, f->crates[m->ci].y))
					{
						p->sound_channel = BASS_SampleGetChannel(g_successSample, 0);
						BASS_ChannelPlay(p->sound_channel, 1);
					}
				}
#endif
			}
		}
	}

	if (gp->targetFloor != gp->floorId && gp->floorFade == 0.f)
		gp->floorFade = 1.f;

	if (gp->floorFade != 0.f)
		if ((gp->floorFade -= elapse * .75f) <= 0.f)
		{
			gp->floorId = gp->targetFloor;
			gp->floorFade = 0.f;
		}

	if (keyDown('0'))
	{
		srand(time(0) ^ g_map_progression[g_current_map].seed);
		g_map_progression[g_current_map].seed = (int) (rand() / (float) RAND_MAX * 3333.f);
	}

	gprintf(.9f, .05f + g_fontSpace, 0x00ff00ff, "SEED %d", g_map_progression[g_current_map].seed);

	int myFloorId = gp->floorId;

render_floor:

	srand(g_map_progression[g_current_map].seed);
	const float vx = floorf(rand() / (float) RAND_MAX / .5f) * .5f;
	const float vy = floorf(rand() / (float) RAND_MAX / .5f) * .5f;

	glPushMatrix();

	if (myFloorId != gp->targetFloor)
	{
		glTranslatef(0.f, (float) gp->floorChange * -g_height * .25f * (1.f - gp->floorFade), 0.f);
		glColor4f(1.f, 1.f, 1.f, gp->floorFade);
	}
	else if (gp->floorFade != 0.f)
	{
		glTranslatef(0.f, (float) gp->floorChange * g_height * .25f * gp->floorFade, 0.f);
		glColor4f(1.f, 1.f, 1.f, 1.f - gp->floorFade);
	}
	else
		glColor4f(1.f, 1.f, 1.f, 1.f);

	struct Floor* f = &g_world.floors[myFloorId];

	for (int n = 0; n < f->nlayers; ++n)
	{
		for (int y = 0; y < g_world.ny; ++y)
		{
			for (int x = 0; x < g_world.nx; ++x)
			{
				float vu,vv;
				const int tile = worldTile(myFloorId, n, x, y);

				if (f->nlayers > 1 && tile > 0)
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

	if (myFloorId != gp->targetFloor)
		glColor4f(1.f, 1.f, 1.f, gp->floorFade);
	else
		glColor4f(1.f, 1.f, 1.f, 1.f - gp->floorFade);

	const int dx = p->ix - p->x;
	const int dy = p->iy - p->y;
	const float ix = dx * ss * (p->time / p->speed);
	const float iy = dy * ss * (p->time / p->speed);

	for (int i = 0; i < f->ncrates; ++i)
	{
		const int x = f->crates[i].x;
		const int y = f->crates[i].y;
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

		if (f->nlayers > 1 && g_world.crateTex != 0)
		{
			int tex = g_world.crateTex;
			if (isTargetTile(gp->floorId, x, y) && g_world.crateTargetTex != 0)
				tex = g_world.crateTargetTex;

			float vv, vu, tilesize;
			int id = getTexCoords(tex, &vu, &vv, &tilesize);
			sprite(xx, yy, ss3 * sm, id, vu, vv, vu + tilesize, vv + tilesize);
		}
		else
			sprite(xx, yy, ss3 * sm, TEX_CRATE, 0.f, 0.f, 1.f, 1.f);
	}

	glPopMatrix();

	if (gp->floorId != gp->targetFloor && myFloorId == gp->floorId)
	{
		myFloorId = gp->targetFloor;
		goto render_floor;
	}

	float u0, v0, u1, v1;

	if (dx || dy)
	{
		const float s = (dx || dy) && !p->turn ? clamp(p->time / p->speed, 0.f, 1.f) : 0.f;
		const int f = clamp((int) (s / .15f), 0, PLAYER_ANIM_FRAME_COUNT - 1);
		int t;

		if (p->turn && (p->fx != dx || p->fy != dy)) {
			if (p->fx > 0) {
				t = dx < 0.f ? PLAYER_ANIM_TILE_WALKDOWN :
					dy < 0.f ? PLAYER_ANIM_TILE_WALKUPRIGHT :
					dy > 0.f ? PLAYER_ANIM_TILE_WALKDOWNRIGHT :
					PLAYER_ANIM_TILE_PUSHDOWN;
			}
			else if (p->fx < 0) {
				t = dx > 0.f ? PLAYER_ANIM_TILE_WALKUP :
					dy < 0.f ? PLAYER_ANIM_TILE_WALKUPLEFT :
					dy > 0.f ? PLAYER_ANIM_TILE_WALKDOWNLEFT :
					PLAYER_ANIM_TILE_PUSHDOWN;
			}
			else if (p->fy > 0) {
				t = dy < 0.f ? PLAYER_ANIM_TILE_WALKLEFT :
					dx < 0.f ? PLAYER_ANIM_TILE_WALKDOWNLEFT :
					dx > 0.f ? PLAYER_ANIM_TILE_WALKDOWNRIGHT :
					PLAYER_ANIM_TILE_PUSHDOWN;
			}
			else if (p->fy < 0) {
				t = dy > 0.f ? PLAYER_ANIM_TILE_WALKRIGHT :
					dx < 0.f ? PLAYER_ANIM_TILE_WALKUPLEFT :
					dx > 0.f ? PLAYER_ANIM_TILE_WALKUPRIGHT :
					PLAYER_ANIM_TILE_PUSHDOWN;
			}
			else
				t = PLAYER_ANIM_TILE_PUSHDOWN;

			if ((p->turntime -= elapse) <= 0.f) {
				p->turn = 0;
				p->fx = dx;
				p->fy = dy;
			}
		}
		else {
			t = dx < 0.f ? PLAYER_ANIM_TILE_WALKLEFT + p->move :
				dx > 0.f ? PLAYER_ANIM_TILE_WALKRIGHT + p->move :
				dy < 0.f ? PLAYER_ANIM_TILE_WALKUP + p->move :
				dy > 0.f ? PLAYER_ANIM_TILE_WALKDOWN + p->move : -1;

			p->turn = 0;
			p->turntime = 0.f;
			p->fx = dx;
			p->fy = dy;
		}

		u0 = g_playerAnimTiles[t].frames[f].s0;
		v0 = g_playerAnimTiles[t].frames[f].t0;
		u1 = g_playerAnimTiles[t].frames[f].s1;
		v1 = g_playerAnimTiles[t].frames[f].t1;
	}
	else
	{
		const int f = (int) (.60f / .15f);
		const int t = PLAYER_ANIM_TILE_WALKDOWN;

		u0 = g_playerAnimTiles[t].frames[f].s0;
		v0 = g_playerAnimTiles[t].frames[f].t0;
		u1 = g_playerAnimTiles[t].frames[f].s1;
		v1 = g_playerAnimTiles[t].frames[f].t1;

		p->turn = 0;
		p->turntime = 0.f;
		p->fx = 0;
		p->fy = 1;
	}

	sprite(posX(p->x, ss) + ix, posY(p->y, ss) + iy, ss, TEX_PLAYER, u0, v0, u1, v1);


#if 1
	// draw ui

	const char* uiScreenName;
	struct UllWidget* w;

	uiScreenName = g_isMenuMap ? "main-menu" : "in-game";

	if ((w = g_uiScreens) != NULL && (w = ullFindScreen(w, uiScreenName)) != NULL)
		while (w = ullNextWidget(w), w->type != 'X') {
			char *s = NULL, t[256], t2[64];
			*t = 0;

			while ((s = ullFormat(t, ullText(w), s)) != NULL)
				if (strnicmp(s, "level", 5) == 0)
					s = g_map_progression[g_current_map].name;
				else if (strnicmp(s, "time", 5) == 0)
					sprintf(s = t2, "%02d:%02d:%02d", gp->timer.hours, gp->timer.minutes, gp->timer.seconds);
				else if (strnicmp(s, "moves", 5) == 0)
					sprintf(s = t2, "%d", gp->moves);
				else if (strnicmp(s, "par", 5) == 0)
					sprintf(s = t2, "%d", g_map_progression[g_current_map].par);

			if (w->type == 'B') {
				float x = g_width * (ullX(w) / 100.f);
				float y = g_height * (ullY(w) / 100.f);
				float ww = g_width * (ullWidth(w) / 100.f) - 1.f;
				float hh = g_height * (ullHeight(w) / 100.f) - 1.f;
				rect(x, y, ww, hh, 0x007f7f7f);

				if (buttonDown(0) &&
						g_mousex >= x && g_mousex < (x + ww) &&
						g_mousey >= y && g_mousey < (y + hh)) {
					if (!strcmp(ullCmd(w), "undo"))
						gp->cmd = 'u';
					else if (!strcmp(ullCmd(w), "pause"))
						gp->cmd = 'p';
					else if (!strcmp(ullCmd(w), "restart"))
						gp->cmd = 'r';
				}
			}

			gprintf(ullX(w) / 100.f + 2.f / g_width, ullY(w) / 100.f + 20.f / g_height, ~0, t);
		}
#else
	gprintf(.02f, .05f, 0x00ffffff, "LEVEL: %s", g_map_progression[g_current_map].name);
	gprintf(.02f, .05f + g_fontSpace, 0x00ffffff, "TIME:  %02d:%02d:%02d", gp->timer.hours, gp->timer.minutes, gp->timer.seconds);
	gprintf(.02f, .05f + g_fontSpace * 2.f, 0x00ffffff, "MOVES: %d PAR: %d", gp->moves, g_map_progression[g_current_map].par);
	gprintf(.02f, .95f, 0x00ffffff, "[U]NDO   [P]AUSE   [R]ESTART");
#endif

	if (g_map_progression[g_current_map].txt[0] != 0) 
	{
		float row = 0.05f;
		char txt[256];
		strncpy(txt, g_map_progression[g_current_map].txt, 256);

		char* tok = strtok(txt, "^");
		while(tok != NULL)
		{
			gprintf(0.5f - (strlen(tok) * 0.005f), row, 0x00ffffff, tok);
			row += g_fontSpace;
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

		glColor3f(1.f, 1.f, 1.f);

		float u0, v0, u1, v1;
		u0 = .5f, u1 = u0 + .125f;
		v0 = .375f, v1 = v0 + .125f;
		sprite(x, y + 25.f, 128.f, TEX_PLAYER, u0, v0, u1, v1);
	}
}

///////////////////////////////////////////////////////////////////////////////

struct GameOver
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

struct Stage
{
	void* data;
	size_t size;
	void (*func)(float, unsigned*);
} g_stages[] =
{
	{&g_gamePlay, sizeof(g_gamePlay), &gamePlay},
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

void rect(float x, float y, float w, float h, unsigned c)
{
	glColor4ubv((unsigned char*) &c);

	glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x, y + h);
		glVertex2f(x + w, y + h);
		glVertex2f(x + w, y);
	glEnd();

	c = ~0;
	glColor4ubv((unsigned char*) &c);
}

void sprite(float x, float y, float s, unsigned texId, float u0, float v0, float u1, float v1)
{
//	glColor3f(1.f, 1.f, 1.f);

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

	mat4 identity;
	mat4Identity(identity);

	mat4 matrix;
	mat4Ortho(matrix, 0.f, g_width, g_height, 0.f, -1.f, 1.f);

	mat4 proj;
	mat4Mul(proj, identity, matrix);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(proj);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(.1f, .1f, .3f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor4f(1.f, 1.f, 1.f, 1.f);
	renderGame(elapse);

	// render font

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_fontTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);

	for (
		struct Text* t = (struct Text*) g_textBuffer;
		t != (struct Text*) g_textEnd;
		t = (struct Text*) &t->string[t->length + 1])
	{
		float x = g_width * t->x;
		float y = g_height * t->y;

		for (unsigned i = 0, n = t->length; i < n; ++i)
		{
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(g_fontData, 512, 512, t->string[i] - 32, &x, &y, &q, 1);

			glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
			glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
			glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
			glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
		}
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

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

	printf("Loading: %s\n", path);
	fd = fopen(path, "rb");

	if(fd == NULL)
	{
		printf("Failed\n");
		return NULL;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	rewind(fd);
	data = (char*) malloc(size + 1);
	*readed = fread(data, 1, size, fd);
	data[size] = 0;
	fclose(fd);

	return data;
}

struct read_progress
{
	const char* img;
	unsigned int readed;
};

void loadTileProperties(mxml_node_t* tileset, mxml_node_t* tree)
{
	// Tile properties
	mxml_node_t *node, *property;
	mxml_index_t *index = mxmlIndexNew(tileset, "tile", NULL);

	while((node = mxmlIndexEnum(index)) != NULL)
	{
		int id = 1 + atoi(mxmlElementGetAttr(node, "id"));
		property = mxmlFindElement(node, tree, "property", NULL, NULL, MXML_DESCEND);
		if(property == NULL)
			continue;

		const char* name = mxmlElementGetAttr(property, "name");
		if(name == NULL)
			continue;

		if (stricmp(name, "crate") == 0)
			g_world.crateTex = id;
		else if (stricmp(name, "cratetarget") == 0)
			g_world.crateTargetTex = id;
		else
			printf("Unknown property for tile:%d name: %s\n", id, name);

		mxmlDelete(property);
		mxmlDelete(node);
	}

	mxmlIndexDelete(index);		
}

void loadMap_tmx(const char* path)
{
	int n = 0;
	char data_path[PATH_NAME_SIZE] = DATA_DIR;
	char* data = loadFile(strcat(data_path, path), &n);

	if (!data)
		exit(1);

	memset(&g_world, 0, sizeof(g_world));

	mxml_node_t *tree = mxmlLoadString(NULL, data, MXML_TEXT_CALLBACK);
	mxml_node_t *map = mxmlFindElement(tree, tree, "map", NULL, NULL, MXML_DESCEND_FIRST);

	g_world.nx = atoi(mxmlElementGetAttr(map, "width"));
	g_world.ny = atoi(mxmlElementGetAttr(map, "height"));
	int tilesize = atoi(mxmlElementGetAttr(map, "tilewidth"));

	mxml_index_t *index, *index2, *index3;
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

		int num_subtextures = (w / tilesize);
		int tex_id = NUM_TEX + gid / sqr(num_subtextures);

		sprintf(data_path, "%s%s", DATA_DIR, src);

		char* img = loadFile(data_path, &n);

		if (img) {
			createMapTex(tex_id, img, n, w, h);
			free(img);
		}

		g_world.Textures[g_world.ntextures].max_id = gid + sqr(num_subtextures);
		g_world.Textures[g_world.ntextures].num_tiles = num_subtextures;
		g_world.ntextures++;

		loadTileProperties(node, tree);
		mxmlDelete(subnode);
		mxmlDelete(node);
	}

	mxmlIndexDelete(index);

	// Layer loading ---

	index = mxmlIndexNew(map, "layer", NULL);

	int nfloors = 0;

	while((node = mxmlIndexEnum(index)) != NULL)
	{
		const char* name = mxmlElementGetAttr(node, "name");
		subnode = mxmlFindElement(node, tree, "data", NULL, NULL, MXML_DESCEND_FIRST);
		index2 = mxmlIndexNew(subnode, "tile", NULL);

		char bare[32] = {0};
		int fid = 1;
		sscanf(name, "%31[A-Za-z]-%d", bare, &fid);
		nfloors = max(nfloors, fid);
		fid -= 1;

		struct Floor* f = &g_world.floors[fid];

		for (unsigned i = 0; (node2 = mxmlIndexEnum(index2)) != NULL; ++i)
			f->tiles[f->nlayers][i] = atoi(mxmlElementGetAttr(node2, "gid"));

		if (stricmp("walls", bare) == 0)
			f->wallLayer = f->nlayers; 

		f->nlayers++;
		mxmlIndexDelete(index2);
		mxmlDelete(subnode);
		mxmlDelete(node);
	}

	mxmlIndexDelete(index);

	// Objects loading ---
	// Only supports one 'objectgroup'. Inside it only 3 types of objects are valid:
	// - start: position were the player starts
	// - crate: a crate will be created in that position
	// - target: all targets need to have a crate in order to finish a level

	index2 = mxmlIndexNew(map, "objectgroup", NULL);

	while((node2 = mxmlIndexEnum(index2)) != NULL) {
		const char* name = mxmlElementGetAttr(node2, "name");

		char bare[32] = {0};
		int fid = 1;
		sscanf(name, "%31[A-Za-z]-%d", bare, &fid);
		fid -= 1;
		assert(fid >= 0 && fid < nfloors);

		struct Floor* f = &g_world.floors[fid];

		index3 = mxmlIndexNew(node2, "object", NULL);

		while((node = mxmlIndexEnum(index3)) != NULL) {
			int x = atoi(mxmlElementGetAttr(node, "x")) / tilesize;
			int y = atoi(mxmlElementGetAttr(node, "y")) / tilesize;
			const char* type = mxmlElementGetAttr(node, "type");

			if (stricmp(type, "start") == 0) {
				g_world.startx = x;
				g_world.starty = y;
				g_world.startFloor = fid;
			}
			else if (stricmp(type, "crate") == 0) {
				f->crates[f->ncrates].x = x;
				f->crates[f->ncrates].y = y;
				f->ncrates++;
			}
			else if (stricmp(type, "target") == 0) {
				f->targets[f->ntargets].x = x;
				f->targets[f->ntargets].y = y;
				f->targets[f->ntargets].type = TARGET_TARGET;
				f->ntargets++;
			}
			else if (stricmp(type, "ramp-up") == 0) {
				f->ramps[f->nramps].x = x;
				f->ramps[f->nramps].y = y;
				f->ramps[f->nramps].dir = 1;
				f->nramps++;
			}
			else if (stricmp(type, "ramp-down") == 0) {
				f->ramps[f->nramps].x = x;
				f->ramps[f->nramps].y = y;
				f->ramps[f->nramps].dir = -1;
				f->nramps++;
			}
			else if (stricmp(type, "newgame") == 0) {
				f->targets[f->ntargets].x = x;
				f->targets[f->ntargets].y = y;
				f->targets[f->ntargets].type = TARGET_NEWGAME;
				f->ntargets++;
			}
			else if (stricmp(type, "continuegame") == 0) {
				f->targets[f->ntargets].x = x;
				f->targets[f->ntargets].y = y;
				f->targets[f->ntargets].type = TARGET_CONTINUEGAME;
				f->ntargets++;
			}
			else if (stricmp(type, "hiscores") == 0) {
				f->targets[f->ntargets].x = x;
				f->targets[f->ntargets].y = y;
				f->targets[f->ntargets].type = TARGET_HISCORES;
				f->ntargets++;
			}
			else if (stricmp(type, "options") == 0) {
				f->targets[f->ntargets].x = x;
				f->targets[f->ntargets].y = y;
				f->targets[f->ntargets].type = TARGET_OPTIONS;
				f->ntargets++;
			}

			mxmlDelete(node);
		}

		mxmlIndexDelete(index3);
		mxmlDelete(node2);
	}

	mxmlIndexDelete(index2);

	// ----------------------

	mxmlDelete(map);
	mxmlDelete(tree);
	free(data);
}

void loadMap(const char* path)
{
		int n = 0;
		char data_path[PATH_NAME_SIZE] = DATA_DIR;
		char* data = loadFile(strcat(data_path, path), &n);

		if (!data)
			exit(1);

		memset(&g_world, 0, sizeof(g_world));

		struct Floor* f = &g_world.floors[0];
		struct Crate* crate = f->crates;
		struct Target* target = f->targets;
		unsigned char* dst = f->tiles[0];
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

		f->ncrates = crate - f->crates;
		f->ntargets = target - f->targets;
		f->nlayers = 1;
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

	if (!data)
		return;

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

void loadFont(const char* name, unsigned size)
{
	char path[256];
	snprintf(path, 256, "data/fonts/%s", name);

	int n;
	unsigned char* data = (unsigned char*) loadFile(path, &n);

	if (!data)
		exit(1);

	unsigned char* tmp = (unsigned char*) malloc(512 * 512);

	stbtt_BakeFontBitmap(data, 0, (float) size, tmp, 512, 512, 32, 96, g_fontData);

	glGenTextures(1, &g_fontTex);
	glBindTexture(GL_TEXTURE_2D, g_fontTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tmp);

	free(data);
	free(tmp);
}

void onFile(char* path)
{
	char* ext;
	char* data;
	int n;

	if ((ext = strchr(path, '.')) == NULL)
		return;

	if (stricmp(ext, ".png") == 0)
	{
		char name[64];
		int w, h, i;

		if (sscanf(path, "%*[^/]/%64[^_]_%dx%d", name, &w, &h) != 3)
			return;

		for (i = 0; i < NUM_TEX; ++i)
			if (stricmp(g_texnames[i], name) == 0)
				break;

		if (i == NUM_TEX)
			return;

		data = loadFile(path, &n);
		if (!data)
			exit(1);

//		assert(n == w * h * 4);

		glGenTextures(1, &g_tex[i]);
		glBindTexture(GL_TEXTURE_2D, g_tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}
	else if (stricmp(ext, ".cfg") == 0)
	{
		loadConfig(path);
	}
	else if (stricmp(ext, ".txt") == 0)
	{
		FILE* fd = fopen(path, "rb");

		for (; g_nprefs < arrayCount(g_prefs); ++g_nprefs)
			if (fscanf(fd, "%15s = %63s", g_prefs[g_nprefs].name, g_prefs[g_nprefs].value) == EOF)
				break;

		fclose(fd);
	}
	else if (!stricmp(ext, ".tmx")) {
		char name[64];
		char fullpath[256];

		if (sscanf(path, "%*[^/]/%64[^.]", name) != 1)
			return;

		if (!strcmp(name, "playeranims")) {
			data = loadFile(path, &n);

			if (!data)
				exit(1);

			// parse tmx data

			char* p = data;
			char* x = p;
			int sw = 0, sh = 0;

			while ((p = strpbrk(p, "<")) != NULL) {
				n = strcspn(++p, " ") + 1;

				if (!strncmp(p, "map ", n))
					p = strstr(p, "width=") + 7, *x++ = (char) atoi(p),
					p = strstr(p, "height=") + 8, *x++ = (char) atoi(p),
					p = strstr(p, "tilewidth=") + 11, *x++ = (char) atoi(p),
					p = strstr(p, "tileheight=") + 12, *x++ = (char) atoi(p);
				else if (!strncmp(p, "image ", n))
					p = strstr(p, "source=") + 8, n = strcspn(p, "\""),
					strncpy(x, p, n), x[n++] = 0, x[n++] = 1, x += n,
					p = strstr(p, "width=") + 7, sw = atoi(p),
					p = strstr(p, "height=") + 8, sh = atoi(p);
				else if (!strncmp(p, "tile ", n))
					p = strstr(p, "id=") + 4, *x++ = (char) atoi(p);
				else if (!strncmp(p, "property ", n))
					p = strstr(p, "name=") + 6, n = strcspn(p, "\""),
					strncpy(x, p, n), x[n++] = 0, x += n,
					p = strstr(p, "value=") + 7, *x++ = (char) atoi(p);
			}

			*x++ = 0;

			// compile texcoords for animation frames

			struct PlayerAnimInfo* pai = (struct PlayerAnimInfo*) data;
			struct PlayerAnimTile* pat = (struct PlayerAnimTile*) x;
			char* s = pai->source + strlen(pai->source) + 1;
			int m = pai->width * pai->height * sizeof(struct PlayerAnimTile);
			int nx = sw / pai->tileWidth;
			int ny = sh / pai->tileHeight;

			while (*s != 1)
				++s;

			while (*s) {
				// let j = tile type, k = tile index, f = frame index

				int j, k = *s++;
				for (j = 0; j < arrayCount(g_playerAnimTileNames) &&
					strcmp(s, g_playerAnimTileNames[j]); ++j);
				s += strlen(s) + 1;
				int f = *s++ - 1;

				if (j < arrayCount(g_playerAnimTileNames))
					pat[j].frames[f].s0 = (k % nx) * (1.f / nx),
					pat[j].frames[f].t0 = (k / ny) * (1.f / ny),
					pat[j].frames[f].s1 = (k % nx) * (1.f / nx) + (1.f / nx),
					pat[j].frames[f].t1 = (k / ny) * (1.f / ny) + (1.f / ny);
			}

			g_playerAnimInfo = pai;
			g_playerAnimTiles = pat;

			// load anim texture

			snprintf(fullpath, sizeof fullpath, "data/%s", pai->source);
			data = loadFile(fullpath, &n);

			if (!data)
				exit(1);

			unsigned char* image_data = stbi_load_from_memory(
				(const unsigned char*) data, n, &sw, &sh, &m, 0);

			glGenTextures(1, &g_tex[TEX_PLAYER]);
			glBindTexture(GL_TEXTURE_2D, g_tex[TEX_PLAYER]);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
			free(image_data);
			free(data);
		}
	}
#if 1
	else if (stricmp(ext, ".ull") == 0)
	{
		if (data = loadFile(path, &n), ullCompile(data) >= 0)
			g_uiScreens = (struct UllWidget*) data;
	}
#endif
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

	// init bass

#if _WIN32 || _MACOSX
	BASS_Init(-1, 44100, 0, 0, NULL);
	g_pushSample = BASS_SampleLoad(0, "data/soundfx/push.wav", 0, 0, 1, BASS_SAMPLE_LOOP);
	g_walkSample = BASS_SampleLoad(0, "data/soundfx/walk.wav", 0, 0, 1, BASS_SAMPLE_LOOP);
	g_successSample = BASS_SampleLoad(0, "data/soundfx/success.wav", 0, 0, 1, 0);
#endif

	// load data

	_foreach_file("data", onFile);

	// init font

	char* fontName = NULL;
	unsigned fontSize = 16;

	for (unsigned i = 0, n = g_nprefs; i < n; ++i)
		if (stricmp(g_prefs[i].name, "font-name") == 0)
			fontName = g_prefs[i].value;
		else if (stricmp(g_prefs[i].name, "font-size") == 0)
			fontSize = atoi(g_prefs[i].value);
		else if (stricmp(g_prefs[i].name, "font-space") == 0)
			g_fontSpace = atof(g_prefs[i].value);

	if (fontName != NULL)
		loadFont(fontName, fontSize);

	g_textEnd = g_textBuffer;

	// Change to iphone reso by default
	ChangeRes(960, 640, "Iphone 4");
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
		if (stricmp(argv[i], "-mute") == 0)
			g_mute = 1;
	}

	init(&argc, argv);
	atexit(&end);

	glutMainLoop();
	return 0;
}

