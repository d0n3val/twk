
#if _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include "bass/bass.h"
# pragma comment(lib, "bass/bass.lib")
#elif __linux__
# include <dirent.h>
# include <sys/time.h>
# include <time.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#elif __linux__
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

#define TEX_CRATE (0)
#define TEX_PLAYER (1)
#define NUM_TEX (2)
#define NUM_MAPS (20)
#define PATH_NAME_SIZE (64)

char g_maps[NUM_MAPS][PATH_NAME_SIZE];
int g_last_map = -1;
int g_current_map = 0;

#define texUvPlayer(u0,v0,u1,v1,su,sv) \
	do { \
		u0 = floorf(((su) >= .75f ? (su) - .5f : (su)) / .25f) * .2f - .02f; \
		u1 = (u0) + .19f; \
		v0 = floorf((sv) / .25f) * .25f; \
		v1 = (v0) + .25f; \
	} while (0)

const char* g_texnames[] = {"crate", "player"};
unsigned g_tex[NUM_TEX];

#define TILE_VOID ('!')
#define TILE_WALL ('X')
#define TILE_FLOOR ('.')
#define TILE_START ('$')
#define TILE_CRATE ('#')
#define TILE_TARGET ('*')
#define TILE_UNKNOWN ('?')

struct Crate
{
	int x, y;
};

struct World
{
	int ncrates;
	struct Crate crates[64];

	int startx, starty;
	int nx, ny;
	char tiles[4096];
};

struct World g_world;

int worldTile(int x, int y)
{
	return x >= 0 && y >= 0 ? g_world.tiles[y * g_world.nx + x] : TILE_UNKNOWN;
}

struct Crate* worldCrate(int x, int y)
{
	for (int i = 0; i < g_world.ncrates; ++i)
		if (g_world.crates[i].x == x && g_world.crates[i].y == y)
			return &g_world.crates[i];

	return NULL;
}

void gprintf(float x, float y, unsigned c, const char* fmt, ...);
void quad(float x, float y, float s, unsigned c);
void sprite(float x, float y, float s, unsigned texId, float u0, float v0, float u1, float v1);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GAME_START (0)
#define GAME_PLAY (1)
#define GAME_OVER (2)

///////////////////////////////////////////////////////////////////////////////

struct GameStart
{
	int init;
	float mx, my;
	float fx, fy;
	float px, py;
	float cx[3], cy[3], cs[3], crx[3], cry[3];
	float blink;
	unsigned state;
} g_gameStart;

void gameStart(float elapse, unsigned* stage)
{
	struct GameStart* gs = &g_gameStart;

	if (!gs->init || g_reshape)
	{
		gs->init = 1;

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

		g_current_map = 0;
	}

	if (gs->mx != g_mousex)
		gs->fx = gs->mx = g_mousex;

	if (gs->my != g_mousey)
		gs->fy = gs->my = g_mousey;

	float dx = (gs->fx - gs->px) / g_width;
	float dy = (gs->fy - gs->py) / g_height;

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
	texUvPlayer(u0, v0, u1, v1, .25f, .5f);
	sprite(gs->px, gs->py, 128.f, TEX_PLAYER, u0, v0, u1, v1);

	static const unsigned colors[] = {0x0000ff00, 0x00ffffff};
	unsigned color = colors[gs->state];
	char map_name[PATH_NAME_SIZE];

	sscanf(g_maps[g_current_map], "%*[^/]/%64[^.]", map_name);

	gprintf(.5f, .5f, 0x00ff00ff, "Touchy Warehouse Keeper");
	gprintf(.5f, .52f, 0x00ff00ff, "v0.1");
	gprintf(.5f, .56f, 0x00ff00ff, "Map (right click to cicle): %s", map_name);
	gprintf(.5f, .60f, color, "Click to continue");

	if ((gs->blink += elapse) >= 1.f)
		gs->blink -= 1.f, gs->state ^= 1;

	if (buttonDown(2))
	{
		if (g_current_map < g_last_map)
			++g_current_map;
		else
			g_current_map = 0;
	}

	if (buttonUp(0))
		++(*stage);

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
};

struct Move
{
	int x0, y0;
	int x1, y1;
	int dx, dy;
	int ci;
};

struct GamePlay
{
	int init;
	float intro;
	struct Player player;
	struct Move move;
	int npath;
	struct PathNode path[128];
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

			if (worldTile(x, y) == TILE_WALL || worldCrate(x, y))
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

void load_map(const char* path)
{
		FILE* fd;
		long size;
		char* data;
		int n;

		fd = fopen(path, "rb");
		fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		rewind(fd);
		data = (char*) malloc(size);
		n = fread(data, 1, size, fd);
		fclose(fd);

		memset(&g_world, 0, sizeof(g_world));

		struct Crate* crate = g_world.crates;
		char* dst = g_world.tiles;
		char* src = data;

		for (int i = 0, x = 0; i < n; ++i)
		{
			const char c = *src++;

			if (c == TILE_VOID || c == TILE_WALL || c == TILE_FLOOR || c == TILE_TARGET)
				++x, *dst++ = c;
			else if (c == TILE_CRATE)
				crate->x = x++, (crate++)->y = g_world.ny, *dst++ = TILE_FLOOR;
			else if (c == TILE_START)
				g_world.startx = x++, g_world.starty = g_world.ny, *dst++ = TILE_FLOOR;
			else if (c == '\n')
				x = 0, ++g_world.ny;

			g_world.nx = max(g_world.nx, x);
		}

		g_world.ncrates = crate - g_world.crates;
		free(data);
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
		gp->intro = -M_PI;
		load_map(g_maps[g_current_map]);
		p->ix = p->x = g_world.startx;
		p->iy = p->y = g_world.starty;
		p->path = -1;
	}

	gp->intro = min(gp->intro + 16.f * elapse, M_PI * 4.5f);

	if (keyDown('r'))
		--(*stage);

	if (p->path >= 0 || p->move)
		goto ignore_mouse_input;

	const int mx = unposX(g_mousex, ss);
	const int my = unposY(g_mousey, ss);

	if (buttonDown(0))
	{
		struct Crate* crate = worldCrate(mx, my);

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
				if ((worldTile(i, my) != TILE_FLOOR && worldTile(i, my) != TILE_TARGET) || worldCrate(i, my))
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
				if ((worldTile(mx, i) != TILE_FLOOR && worldTile(mx, i) != TILE_TARGET) || worldCrate(mx, i))
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
			}
		}
	}

ignore_mouse_input:
	;

	const float tt = .5f;

	if (p->path < 0 && !p->move)
		;
	else if ((p->time += elapse) >= tt)
	{
		p->time -= tt;

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

				m->x0 = -1;
				m->y0 = -1;
				m->x1 = -1;
				m->y1 = -1;
				m->dx = 0;
				m->dy = 0;
			}
		}
	}

	srand(15);

	for (int y = 0; y < g_world.ny; ++y)
	{
		for (int x = 0; x < g_world.nx; ++x)
		{
			const int tile = worldTile(x, y);
			unsigned cc = (unsigned) (0x20 * ((float) rand() / RAND_MAX));
			unsigned kk = 0x008f6f6f;

			if (tile == TILE_WALL)
				kk = 0x002f3f4f;
			else if (tile == TILE_TARGET)
				kk = 0x007fcf7f;
			else if (tile == TILE_VOID)
				continue;

			quad(posX(x, ss), posY(y, ss), ss, kk + cc);
		}
	}

	const int dx = p->ix - p->x;
	const int dy = p->iy - p->y;
	const float ix = dx * ss * (p->time / tt);
	const float iy = dy * ss * (p->time / tt);

	for (int i = 0; i < g_world.ncrates; ++i)
	{
		const int x = g_world.crates[i].x;
		const int y = g_world.crates[i].y;
		float xx = posX(x, ss);
		float yy = posY(y, ss);

		if (p->move && p->x - m->dx == x && p->y - m->dy == y)
		{
			xx += ix;
			yy += iy;
		}

		const float t = gp->intro + (float) rand() / RAND_MAX * M_PI;
		const float w = t / (M_PI * 4.5f);
		const float ss2 = ss * .5f + ss * (cosf(t) + 1.f) * .5f;
		const float ss3 = w * ss + (1.f - w) * ss2;
		sprite(xx, yy, ss3, TEX_CRATE, 0.f, 0.f, 1.f, 1.f);
	}

	if (buttonHeld(0))
	{
		const int x0 = min(m->x0 + 1, m->x1), x1 = max(m->x0 - 1, m->x1);
		const int y0 = min(m->y0 + 1, m->y1), y1 = max(m->y0 - 1, m->y1);

		if ((x0 == x1 && y0 != y1) || (x0 != y1 && y0 == y1))
		{
			for (int i = x0; i <= x1 && worldTile(x0, y0) == TILE_FLOOR; ++i)
				quad(posX(i, ss), posY(y0, ss), ss, 0x007fcf7f);

			for (int i = y0; i <= y1 && worldTile(x0, y0) == TILE_FLOOR; ++i)
				quad(posX(x0, ss), posY(i, ss), ss, 0x007fcf7f);
		}
	}

	const float su = dx || dy ? clamp(p->time / tt, 0.f, 1.f) : .25f;
	const float sv =
		dx < 0 ? .75f : dx > 0 ? .25f :
		dy < 0 ? .0f : dy > 0 ? .5f : .5f;

	float u0, v0, u1, v1;
	texUvPlayer(u0, v0, u1, v1, su, sv);
	sprite(posX(p->x, ss) + ix, posY(p->y, ss) + iy, ss, TEX_PLAYER, u0, v0, u1, v1);
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

	if (stage >= sizeof(g_stages) / sizeof(g_stages[0]))
		exit(0);

	if (currentStage != stage)
		memset(g_stages[stage].data, 0, g_stages[stage].size);

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

	gprintf(.02f, .05f, 0x00cf7f7f, "%dx%d", g_mousex, g_mousey);

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

void onFile(char* path)
{
	FILE* fd;
	long size;
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

		fd = fopen(path, "rb");
		fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		rewind(fd);
		data = (char*) malloc(size);
		n = fread(data, 1, size, fd);
		fclose(fd);

		glGenTextures(1, &g_tex[i]);
		glBindTexture(GL_TEXTURE_2D, g_tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}
	else if (strcmp(ext, ".map") == 0)
	{
		g_last_map++;
		strncpy(g_maps[g_last_map], path, PATH_NAME_SIZE);
	}
	else if (strcmp(ext, ".mp3") == 0)
	{
#if _WIN32
		HSTREAM stream = BASS_StreamCreateFile(FALSE, path, 0, 0, BASS_SAMPLE_LOOP);
		BASS_ChannelPlay(stream, 0);
#endif
	}
}


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

	g_window = glutCreateWindow("twk");

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

#if _WIN32
	BASS_Init(-1, 44100, 0, 0, NULL);
#endif

	// load data

	_foreach_file("data", onFile);
}

void end()
{
#if _WIN32
	BASS_Free();
#endif
	glutDestroyWindow(g_window);
}

int main(int argc, char* argv[])
{
	init(&argc, argv);
	atexit(&end);

	glutMainLoop();
	return 0;
}
