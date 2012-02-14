
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ullX(w) ((w)->data[0])
#define ullY(w) ((w)->data[1])
#define ullWidth(w) ((w)->data[2])
#define ullHeight(w) ((w)->data[3])
#define ullAlign(w) ((w)->data[4])
#define ullText(w) (&(w)->data[6])
#define ullCmd(w) (&(w)->data[(w)->data[5] + 7])

struct UllWidget
{
	char type, data[0];
};

static struct UllWidget* ullFindScreen(struct UllWidget* w, const char* name)
{
	while (w->type == 'S') {
		if (!strcmp(&w->data[1], name))
			return w;

		short i;
		memcpy(&i, &w->data[1 + w->data[0]], 2);
		w = (struct UllWidget*) &w->data[1 + w->data[0] + 2 + i];
	}
	
	return NULL;
}

static struct UllWidget* ullNextWidget(struct UllWidget* w)
{
	int i = 0;

	if (w->type == 'S')
		i = 1 + w->data[0], i += 2;
	else if (w->type == 'L')
		i = 5, i += 1 + w->data[i];
	else if (w->type == 'B')
		i = 5, i += 1 + w->data[i], i += 1 + w->data[i];

	return (struct UllWidget*) &w->data[i];
}

static char* ullFormat(char* buf, char* fmt, const char* arg)
{
	static char* p;
	char *t;
	
	if (arg != NULL)
		fmt = p, strcat(buf, arg);

	if ((t = strchr(fmt, '$')) != NULL) {
		if (strncat(buf, fmt, t - fmt), (p = strchr(t, ' ')) == NULL)
			p = t + strlen(t);

		return t + 1;
	}

	return strcat(buf, fmt), NULL;
}

static int ullCompile(char* s)
{
	char *base, *cmd, *name, *ws = " \t\r\n";
	char *p = s, *t = strtok(s, ws);
	int n, x, y, w, h;
	short i;
	char align[16], text[256];

	for (; t != NULL; t = strtok(NULL, ws)) {
		if (strncmp(t, "scene", 5))
			return fprintf(stderr, "Expected scene definition\n"), -1;

		if ((name = strtok(NULL, ws)) == NULL)
			return fprintf(stderr, "Expected scene name\n"), -1;

		*p++ = 'S';
		*p++ = (char) (n = strlen(name) + 1);
		memmove(p, name, n), p += n;
		base = p, p += 2;

		for (t = strtok(NULL, ws); t != NULL;) {
			if (!strncmp(t, "end", 5))
				break;

			if (!strncmp(t, "button", 5) || !strncmp(t, "label", 5)) {
				cmd = NULL, x = y = 0, *align = 0, *text = 0;

				if ((t = strtok(NULL, "}")) == NULL)
					return fprintf(stderr, "Expected label text\n"), -1;
					
				if (sscanf(t, "%dx%d%d%d %15s {%255[^\r\n]", &x, &y, &w, &h, align, text) != 6)
					return fprintf(stderr, "Failed to parse label position and text\n"), -1;
					
				if ((t = strtok(NULL, ws)) != NULL && strcmp(t, "end"))
					if (!strncmp(t, "->", 2)) {
						if ((cmd = strtok(NULL, ws)) == NULL)
							return fprintf(stderr, "Expected action text\n"), -1;

						t = strtok(NULL, ws);
					}
					
				*p++ = (!!cmd)["LB"];
				*p++ = (char) x, *p++ = (char) y;
				*p++ = (char) w, *p++ = (char) h;
				*p++ = *align;
				*p++ = (char) (n = strlen(text) + 1);
				memmove(p, text, n), p += n;
				
				if (cmd != NULL) {
					*p++ = (char) (n = strlen(cmd) + 1);
					memmove(p, cmd, n), p += n;
				}
			}
			else
				return fprintf(stderr, "Unexpected token \"%s\"\n", t), -1;
		}
		
		*p++ = 'X';
		i = p - (base + 2);
		memcpy(base, &i, 2);
	}
	
	*p++ = 'X';
	return p - s;
}

#if 0
static void draw(char* p, char* name)
{
	struct UllWidget* w = (struct UllWidget*) p;
	
	if ((w = ullFindScreen(w, name)) != NULL) {
		while (w = ullNextWidget(w), w->type != 'X') {
			char *s = NULL, t[256] = {0};
			
			while ((s = ullFormat(t, ullText(w), s)) != NULL) {
				if (strnicmp(s, "title", 5) == 0)
					s = "MY TITLE";
				else if (strnicmp(s, "level", 5) == 0)
					s = "MY LEVEL";
			}
		
			if (w->type == 'B')
				printf("widget:%c is {%s} (%s)\n", w->type, t, ullCmd(w));
			else
				printf("widget:%c is {%s}\n", w->type, t);
		}
	}
}

char* load(const char* path)
{
	FILE* fd;
	size_t n;
	char* p;

	if ((fd = fopen(path, "rb")) == NULL)
		return NULL;

	fseek(fd, 0, SEEK_END);
	n = ftell(fd);
	rewind(fd);

	p = (char*) malloc(n + 1);
	fread(p, n, 1, fd);
	p[n] = 0;
	fclose(fd);

	return p;
}

int main(int argc, char* argv[])
{
	char* data;
	char* cmd = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: sample <file>\n");
		return 1;
	}

	if ((data = load(argv[1])) == NULL) {
		fprintf(stderr, "Failed to load %s\n", argv[1]);
		return 1;
	}

	if (ullCompile(data) < 0)
		fprintf(stderr, "Giving up\n");
	else {
		draw(data, "main-menu");
		draw(data, "in-game");
		draw(data, "end-game");
	}

	free(data);
	return 0;
}
#endif
