#include <SDL2/SDL.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

/* 
Copyright (c) 2020 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define HOR 32
#define VER 16
#define PAD 8
#define SZ (HOR * VER * 16)

#define VLIMIT 256
#define ELIMIT 512
#define MLIMIT 128
#define PI 3.14159265358979323846

#define VIEWFRONT Pt3d(180, 0, 0)
#define VIEWTOP Pt3d(90, 0, 0)
#define VIEWSIDE Pt3d(90, 90, 0)

typedef struct {
	double x, y;
} Point2d;

typedef struct {
	double x, y, z;
} Point3d;

typedef struct {
	int color;
	int a;
	int b;
} Edge;

typedef struct {
	int verticeslen;
	int edgeslen;
	Point3d position;
	Point3d vertices[VLIMIT];
	Edge edges[ELIMIT];
} Mesh;

typedef struct {
	int len;
	Point3d position, scale, rotation;
	Mesh meshes[MLIMIT];
} Scene;

typedef enum {
	ISOMETRIC,
	PERSPECTIVE
} Projection;

typedef struct {
	double range;
	Point3d origin, rotation, torigin, trotation;
	Projection projection;
} Camera;

int WIDTH = 8 * HOR + PAD * 2;
int HEIGHT = 8 * (VER + 2) + PAD * 2;
int FPS = 30, GUIDES = 1, ZOOM = 2;

Scene scn;
Camera cam;

/* interface */

Uint32 theme[] = {
	0x000000,
	0xFFFFFF,
	0x72DEC2,
	0x666666,
	0x222222};

Uint8 icons[][8] = {
	{0x10, 0x28, 0x44, 0x92, 0x44, 0x28, 0x10, 0x00},
	{0x10, 0x28, 0x44, 0x82, 0x82, 0x82, 0xd6, 0x00},
	{0x1e, 0x22, 0x40, 0x82, 0x40, 0x22, 0x1e, 0x00},
	{0x04, 0x08, 0x50, 0xa4, 0x50, 0x08, 0x04, 0x00},
	{0x1e, 0x20, 0x40, 0xa2, 0x40, 0x20, 0x1e, 0x00},
	{0x00, 0x00, 0x00, 0x82, 0x44, 0x38, 0x00, 0x00}, /* eye open */
	{0x00, 0x38, 0x44, 0x92, 0x28, 0x10, 0x00, 0x00}  /* eye closed */
};

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_Texture *gTexture = NULL;
Uint32 *pixels;

/* helpers */

Point2d
Pt2d(double x, double y)
{
	Point2d p;
	p.x = x;
	p.y = y;
	return p;
}

Point3d *
set3d(Point3d *v, double x, double y, double z)
{
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

Point3d
Pt3d(double x, double y, double z)
{
	Point3d p;
	set3d(&p, x, y, z);
	return p;
}

Mesh
Ms3d(double x, double y, double z)
{
	Mesh m;
	set3d(&m.position, x, y, z);
	m.verticeslen = 0;
	m.edgeslen = 0;
	return m;
}

Scene
Sc3d(void)
{
	Scene s;
	s.len = 0;
	set3d(&s.position, 0, 0, 0);
	set3d(&s.scale, 1, 1, 1);
	set3d(&s.rotation, 0, 0, 0);
	return s;
}

Camera
Cm3d(double pitch, double yaw, double roll)
{
	Camera c;
	set3d(&c.rotation, pitch, yaw, roll);
	set3d(&c.trotation, pitch, yaw, roll);
	c.projection = PERSPECTIVE;
	c.range = 50;
	return c;
}

int
colortheme(Uint32 hex)
{
	int i = 0;
	for(i = 0; i < 5; ++i)
		if(theme[i] == hex)
			return i;
	return 0;
}

/* geometry */

double
interpolate(double a, double b, double speed, double range)
{
	if(a < b - range || a > b + range) {
		a += (b - a) / speed;
	} else
		a = b;
	return a;
}

Point2d
rot2d(Point2d a, Point2d b, double deg)
{
	double radian = deg * (PI / 180);
	double angle = atan2(b.y - a.y, b.x - a.x) + radian;
	double r = sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	return Pt2d(a.x + r * cos(angle), a.y + r * sin(angle));
}

int
equpt3d(Point3d p0, Point3d p1)
{
	return p0.x == p1.x && p0.y == p1.y && p0.z == p1.z;
}

Point3d *
addpt3d(Point3d *p, double x, double y, double z)
{
	return set3d(p, p->x + x, p->y + y, p->z + z);
}

Point3d
add3d(Point3d *a, Point3d *b)
{
	return Pt3d(a->x + b->x, a->y + b->y, a->z + b->z);
}

Point3d
mul3d(Point3d *a, Point3d *b)
{
	return Pt3d(a->x * b->x, a->y * b->y, a->z * b->z);
}

/* scene */

Point3d *
translate3d(Point3d *p, Point3d *t)
{
	*p = add3d(p, t);
	return p;
}

Point3d *
scale3d(Point3d *p, Point3d *t)
{
	*p = mul3d(p, t);
	return p;
}

Point3d *
rotate3d(Point3d *p, Point3d *o, Point3d *t)
{
	if(t->x) {
		Point2d r = rot2d(Pt2d(o->y, o->z), Pt2d(p->y, p->z), t->x);
		p->y = r.x;
		p->z = r.y;
	}
	if(t->y) {
		Point2d r = rot2d(Pt2d(o->x, o->z), Pt2d(p->x, p->z), t->y);
		p->x = r.x;
		p->z = r.y;
	}
	if(t->z) {
		Point2d r = rot2d(Pt2d(o->x, o->y), Pt2d(p->x, p->y), t->z);
		p->x = r.x;
		p->y = r.y;
	}
	return p;
}

Scene *
moveto(Scene *s, double x, double y, double z)
{
	set3d(&s->position, x, y, z);
	return s;
}

Scene *
scaleto(Scene *s, double x, double y, double z)
{
	set3d(&s->scale, x, y, z);
	return s;
}

Scene *
rotateto(Scene *s, double x, double y, double z)
{
	set3d(&s->rotation, x, y, z);
	return s;
}

Scene *
reset(Scene *s)
{
	moveto(scaleto(rotateto(s, 0, 0, 0), 1, 1, 1), 0, 0, 0);
	return s;
}

int
addvertex(Mesh *m, double x, double y, double z)
{
	int i;
	Point3d v = Pt3d(x, y, z);
	if(m->verticeslen == VLIMIT) {
		printf("Warning: Reached vertex limit\n");
		return -1;
	}
	translate3d(&v, &scn.position);
	scale3d(&v, &scn.scale);
	rotate3d(&v, &scn.position, &scn.rotation);
	for(i = 0; i < m->verticeslen; ++i)
		if(equpt3d(m->vertices[i], v))
			return i;
			
	// m->verticeslen++;
	set3d(&m->vertices[m->verticeslen], v.x, v.y, v.z);
	return m->verticeslen++;
}

Edge *
addedge(Mesh *m, int a, int b, int color)
{
	if(m->edgeslen == ELIMIT) {
		printf("Warning: Reached edge limit\n");
		return NULL;
	}
	m->edges[m->edgeslen].a = a;
	m->edges[m->edgeslen].b = b;
	m->edges[m->edgeslen].color = color;
	return &m->edges[m->edgeslen++];
}

Mesh *
addmesh(Scene *s)
{
	if(s->len == MLIMIT) {
		printf("Warning: Reached mesh limit\n");
		return NULL;
	}
	return &s->meshes[s->len++];
}

/* transforms */

Mesh *
translate(Mesh *m, double x, double y, double z)
{
	int i;
	Point3d t = Pt3d(x, y, z);
	for(i = 0; i < m->verticeslen; i++)
		translate3d(&m->vertices[i], &t);
	return m;
}

Mesh *
scale(Mesh *m, double x, double y, double z)
{
	int i;
	Point3d t = Pt3d(x, y, z);
	for(i = 0; i < m->verticeslen; i++)
		scale3d(&m->vertices[i], &t);
	return m;
}

Mesh *
rotate(Mesh *m, double pitch, double yaw, double roll)
{
	int i;
	Point3d t = Pt3d(pitch, yaw, roll);
	for(i = 0; i < m->verticeslen; i++)
		rotate3d(&m->vertices[i], &m->position, &t);
	return m;
}

Mesh *
extrude(Mesh *m, Point3d t, int color)
{
	int i, j, vl = m->verticeslen, el = m->edgeslen;
	for(i = 0; i < vl; i++) {
		Point3d *a = &m->vertices[i];
		addedge(m, i, addvertex(m, a->x + t.x, a->y + t.y, a->z + t.z), color);
	}
	for(i = 0; i < el; i++) {
		Edge *e0 = &m->edges[i];
		Edge *e1 = addedge(m, e0->a, e0->b, e0->color);
		for(j = 0; j < vl; j++) {
			if(e0->a == j)
				e1->a = vl + j;
			else if(e0->b == j)
				e1->b = vl + j;
		}
	}
	return m;
}

/* Primitives */

Mesh *
addpoly(Mesh *m, int a, int b, int c, int color)
{
	addedge(m, a, b, color);
	addedge(m, b, c, color);
	addedge(m, c, a, color);
	return m;
}

Mesh *
addline(Mesh *m, Point3d a, Point3d b, int color)
{
	int i1 = addvertex(m, a.x, a.y, a.z);
	int i2 = addvertex(m, b.x, b.y, b.z);
	addedge(m, i1, i2, color);
	return m;
}

Mesh *
addarc(Mesh *m, double radius, double segs, double angle, int color)
{
	int i;
	double arc = 2 * PI * angle / 360 / segs;
	Point3d b;
	for(i = 0; i < segs + 1; i++) {
		Point3d a = Pt3d(radius * cos(i * arc), radius * sin(i * arc), 0);
		if(i > 0)
			addline(m, a, b, color);
		b = a;
	}
	return m;
}

Mesh *
addshape(Mesh *m, double radius, int segs, int color)
{
	return addarc(m, radius, segs, 360, color);
}

/* Primitives */

Mesh *
createfrustum(Scene *s, double radius, int segs, double depth, double cap, int color)
{
	int i;
	Mesh *m = addmesh(s);
	addshape(m, cap, segs, color);
	translate(m, 0, 0, depth);
	addshape(m, radius, segs, color);
	for(i = 0; i < segs + 1; i++)
		addedge(m, i, segs + i + 1, color);
	return m;
}

Mesh *
createpyramid(Scene *s, double radius, int segs, double depth, int color)
{
	int i;
	Mesh *m = addmesh(s);
	int p = addvertex(m, 0, 0, depth);
	addshape(m, radius, segs, color);
	for(i = 0; i < segs + 1; i++)
		addedge(m, i, p, color);
	return m;
}

Mesh *
createprism(Scene *s, double radius, int segs, double depth, int color)
{
	return createfrustum(s, radius, segs, depth, radius, color);
}

Mesh *
createplane(Scene *s, double width, double height, double xsegs, double ysegs, int color)
{
	int ix, iy;
	Mesh *m = addmesh(s);
	for(ix = 0; ix < xsegs + 1; ix++)
		addline(m,
			Pt3d(ix * (width / xsegs) - width / 2, height / 2, 0),
			Pt3d(ix * (width / xsegs) - width / 2, -height / 2, 0),
			color);
	for(iy = 0; iy < ysegs + 1; iy++)
		addline(m,
			Pt3d(width / 2, iy * (height / ysegs) - height / 2, 0),
			Pt3d(-width / 2, iy * (height / ysegs) - height / 2, 0),
			color);
	return m;
}

Mesh *
createbox(Scene *s, double width, double height, double depth, int color)
{
	Mesh *m = createplane(s, width, height, 1, 1, color);
	extrude(m, Pt3d(0, 0, depth), color);
	return m;
}

Mesh *
createicosaedron(Scene *s, double radius, int color)
{
	Mesh *m = addmesh(s);
	double t = (1.0 + sqrt(5.0)) / 2.0;
	addvertex(m, -1, t, 0);
	addvertex(m, 1, t, 0);
	addvertex(m, -1, -t, 0);
	addvertex(m, 1, -t, 0);
	addvertex(m, 0, -1, t);
	addvertex(m, 0, 1, t);
	addvertex(m, 0, -1, -t);
	addvertex(m, 0, 1, -t);
	addvertex(m, t, 0, -1);
	addvertex(m, t, 0, 1);
	addvertex(m, -t, 0, -1);
	addvertex(m, -t, 0, 1);
	addpoly(m, 0, 11, 5, 3); /* - */
	addpoly(m, 0, 5, 1, 3);
	addpoly(m, 0, 1, 7, 3);
	addpoly(m, 0, 7, 10, 3);
	addpoly(m, 0, 10, 11, 3);
	addpoly(m, 1, 5, 9, 3); /* - */
	addpoly(m, 5, 11, 4, 3);
	addpoly(m, 11, 10, 2, 3);
	addpoly(m, 10, 7, 6, 3);
	addpoly(m, 7, 1, 8, 3);
	addpoly(m, 3, 9, 4, 3); /* - */
	addpoly(m, 3, 4, 2, 3);
	addpoly(m, 3, 2, 6, 3);
	addpoly(m, 3, 6, 8, 3);
	addpoly(m, 3, 8, 9, 3);
	addpoly(m, 4, 9, 5, 3); /* - */
	addpoly(m, 2, 4, 11, 3);
	addpoly(m, 6, 2, 10, 3);
	addpoly(m, 8, 6, 7, 3);
	addpoly(m, 9, 8, 1, 3);
	scale(m, radius / 2, radius / 2, radius / 2);
	return m;
}

Mesh *
createumbrella(Scene *s, int color1, int color2)
{
	int i;
	Mesh *umbrella = addmesh(s);
	reset(s);
	for(i = 0; i < 5; ++i) {
		addarc(umbrella, 10, 8, 180, color2);
		rotateto(s, 0, 45 * i, 0);
	}
	rotateto(s, 90, 0, 0);
	addshape(umbrella, 10, 8, color1);
	translate(scale(umbrella, 1, 0.6, 1), 0, 2, 0);
	addline(umbrella, Pt3d(0, 0, -10), Pt3d(0, 0, 8), color1);
	rotateto(moveto(s, 0, -8, 2), 90, 90, 90);
	addarc(umbrella, 2, 8, 180, color2);
	reset(s);
	return umbrella;
}

void
exportmesh(char* meshname, Mesh* m)
{
	printf("Vertices:\n[");
	for (int i = 0; i < m->verticeslen; i++)
	{
		printf("\t(%lf, %lf, %lf)\n", m->vertices[i].x, m->vertices[i].y, m->vertices[i].z);
	}
	printf("]\n");

	printf("Edges: \n[");
	for (int i = 0; i < m->edgeslen; i++)
	{
		printf("\t(%d, %d)\n", m->edges[i].a, m->edges[i].b);
	}
	printf("]\n");
	

}

/* draw */

void
clear(Uint32 *dst)
{
	int i, j;
	for(i = 0; i < HEIGHT; i++)
		for(j = 0; j < WIDTH; j++)
			dst[i * WIDTH + j] = theme[0];
}

int
getpixel(Uint32 *dst, int x, int y)
{
	return dst[(y + PAD) * WIDTH + (x + PAD)];
}

void
putpixel(Uint32 *dst, int x, int y, int color)
{
	if(x >= 0 && x < WIDTH - 8 && y >= 0 && y < HEIGHT - 8)
		dst[(y + PAD) * WIDTH + (x + PAD)] = theme[color];
}

void
line(Uint32 *dst, Point2d p0, Point2d p1, int color)
{
	int p0x = (int)p0.x, p0y = (int)p0.y;
	int p1x = (int)p1.x, p1y = (int)p1.y;
	int dx = abs(p1x - p0x), sx = p0x < p1x ? 1 : -1;
	int dy = -abs(p1y - p0y), sy = p0y < p1y ? 1 : -1;
	int err = dx + dy, e2;
	for(;;) {
		putpixel(dst, p0x, p0y, color);
		if(p0x == p1x && p0y == p1y)
			break;
		e2 = 2 * err;
		if(e2 >= dy) {
			err += dy;
			p0x += sx;
		}
		if(e2 <= dx) {
			err += dx;
			p0y += sy;
		}
	}
}

void
drawicon(Uint32 *dst, int x, int y, Uint8 *icon, int color)
{
	int v, h;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++) {
			int c = (icon[v] >> (8 - h)) & 0x1;
			putpixel(dst, x + h, y + v, c ? color : 0);
		}
}

void
drawui(Uint32 *dst)
{
	int bottom = VER * 8 + 8;
	drawicon(dst, 0 * 8, bottom, icons[0], equpt3d(cam.rotation, VIEWFRONT) ? 1 : 2);
	drawicon(dst, 1 * 8, bottom, icons[1], equpt3d(cam.rotation, VIEWTOP) ? 1 : 2);
	drawicon(dst, 2 * 8, bottom, icons[2], equpt3d(cam.rotation, VIEWSIDE) ? 1 : 2);
	drawicon(dst, 4 * 8, bottom, icons[cam.projection == ISOMETRIC ? 4 : 3], 1);
	drawicon(dst, 5 * 8, bottom, icons[GUIDES ? 6 : 5], GUIDES ? 1 : 2);
}

Point2d
project(Camera *c, Point3d v)
{
	double r;
	if(c->projection == ISOMETRIC)
		return Pt2d(
			(WIDTH / 2) + v.x * (10 - c->range / 10),
			(HEIGHT / 2) + v.y * (10 - c->range / 10));
	r = 200 / (v.z + c->range);
	return Pt2d(WIDTH / 2 + r * v.x, HEIGHT / 2 + r * v.y);
}

void
redraw(Uint32 *dst)
{
	int i, j;
	clear(dst);
	for(i = 0; i < scn.len; i++) {
		Mesh *m = &scn.meshes[i];
		for(j = 0; j < m->edgeslen; j++) {
			Edge *edge = &m->edges[j];
			Point3d a = add3d(&m->vertices[edge->a], &m->position);
			Point3d b = add3d(&m->vertices[edge->b], &m->position);
			rotate3d(&a, &cam.origin, &cam.rotation);
			rotate3d(&b, &cam.origin, &cam.rotation);
			line(dst,
				project(&cam, add3d(&cam.origin, &a)),
				project(&cam, add3d(&cam.origin, &b)),
				edge->color);
		}
	}
	drawui(dst);
	SDL_UpdateTexture(gTexture, NULL, dst, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
}

/* options */

int
error(char *msg, const char *err)
{
	printf("Error %s: %s\n", msg, err);
	return 0;
}

void
update(Camera *c, double speed)
{
	if(!equpt3d(c->rotation, c->trotation) || !equpt3d(c->origin, c->torigin)) {
		set3d(&c->rotation,
			interpolate(c->rotation.x, c->trotation.x, speed, 1),
			interpolate(c->rotation.y, c->trotation.y, speed, 1),
			interpolate(c->rotation.z, c->trotation.z, speed, 1));
		set3d(&c->origin,
			interpolate(c->origin.x, c->torigin.x, speed, 1),
			interpolate(c->origin.y, c->torigin.y, speed, 1),
			interpolate(c->origin.z, c->torigin.z, speed, 1));
		redraw(pixels);
	}
}

void
modzoom(int mod)
{
	if((mod > 0 && ZOOM < 4) || (mod < 0 && ZOOM > 1))
		ZOOM += mod;
	SDL_SetWindowSize(gWindow, WIDTH * ZOOM, HEIGHT * ZOOM);
	redraw(pixels);
}

void
toggleprojection(Camera *c)
{
	c->projection = c->projection == ISOMETRIC ? PERSPECTIVE : ISOMETRIC;
	redraw(pixels);
}

void
setguides(int v)
{
	GUIDES = v;
	redraw(pixels);
}

void
modrange(int mod)
{
	int res = cam.range + mod;
	if(res > 0 && res < 90)
		cam.range = res;
	redraw(pixels);
}

void
selectoption(int option)
{
	switch(option) {
	case 0: set3d(&cam.trotation, VIEWFRONT.x, VIEWFRONT.y, VIEWFRONT.z); break;
	case 1: set3d(&cam.trotation, VIEWTOP.x, VIEWTOP.y, VIEWTOP.z); break;
	case 2: set3d(&cam.trotation, VIEWSIDE.x, VIEWSIDE.y, VIEWSIDE.z); break;
	case 4: toggleprojection(&cam); break;
	case 5: setguides(!GUIDES); break;
	}
}

void
putchr(Uint8 *chrbuf, int row, int col, int color)
{
	if(row < 0 || row > SZ - 8)
		return;
	if(color == 0 || color == 2)
		chrbuf[row] &= ~(1UL << (7 - col));
	else
		chrbuf[row] |= 1UL << (7 - col);
	if(color == 0 || color == 1)
		chrbuf[row + 8] &= ~(1UL << (7 - col));
	else
		chrbuf[row + 8] |= 1UL << (7 - col);
}

void
exportchr(void)
{
	int h, v, x, y;
	Uint8 chrbuf[SZ];
	FILE *f = fopen("moogle-export.chr", "wb");
	setguides(0);
	for(v = 0; v < VER; ++v)
		for(h = 0; h < HOR; ++h)
			for(y = 0; y < 8; ++y)
				for(x = 0; x < 8; ++x) {
					int row = y + (h + v * HOR) * 16;
					int col = x;
					int clr = pixels[(v * 8 + y + PAD) * WIDTH + (h * 8 + x + PAD)];
					putchr(chrbuf, row, col, colortheme(clr));
				}
	if(!fwrite(chrbuf, sizeof(chrbuf), 1, f))
		error("Save", "Invalid output file");
	fclose(f);
	puts("Exported moogle-export.chr");
}

void
renderbmp(void)
{
	SDL_Surface *surface = SDL_GetWindowSurface(gWindow);
	setguides(0);
	SDL_RenderReadPixels(gRenderer,
		NULL,
		SDL_PIXELFORMAT_ARGB8888,
		surface->pixels,
		surface->pitch);
	if(SDL_SaveBMP(surface, "moogle-render.bmp"))
		printf("SDL_SaveBMP failed: %s\n", SDL_GetError());
	else
		puts("Rendered moogle-render.bmp");
	SDL_FreeSurface(surface);
}

void
quit(void)
{
	free(pixels);
	SDL_DestroyTexture(gTexture);
	gTexture = NULL;
	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_Quit();
	exit(0);
}

void
domouse(SDL_Event *event)
{
	switch(event->type) {
	case SDL_MOUSEBUTTONUP:
		break;
	case SDL_MOUSEBUTTONDOWN:
		if(event->motion.y / ZOOM / 8 == VER + 2)
			selectoption(event->motion.x / ZOOM / 8 - 1);
		break;
	case SDL_MOUSEMOTION:
		break;
	}
}

void
dokey(SDL_Event *event)
{
	int shift = SDL_GetModState() & KMOD_LSHIFT || SDL_GetModState() & KMOD_RSHIFT;
	switch(event->key.keysym.sym) {
	case SDLK_EQUALS:
	case SDLK_PLUS: modzoom(1); break;
	case SDLK_UNDERSCORE:
	case SDLK_MINUS: modzoom(-1); break;
	case SDLK_TAB: toggleprojection(&cam); break;
	case SDLK_1: set3d(&cam.trotation, VIEWFRONT.x, VIEWFRONT.y, VIEWFRONT.z); break;
	case SDLK_2: set3d(&cam.trotation, VIEWTOP.x, VIEWTOP.y, VIEWTOP.z); break;
	case SDLK_3: set3d(&cam.trotation, VIEWSIDE.x, VIEWSIDE.y, VIEWSIDE.z); break;
	case SDLK_e: exportchr(); break;
	case SDLK_r: renderbmp(); break;
	case SDLK_h: setguides(!GUIDES); break;
	case SDLK_UP:
	case SDLK_w:
		if(shift)
			addpt3d(&cam.torigin, 0, 2.5, 0);
		else
			addpt3d(&cam.trotation, 5, 0, 0);
		break;
	case SDLK_LEFT:
	case SDLK_a:
		if(shift)
			addpt3d(&cam.torigin, 2.5, 0, 0);
		else
			addpt3d(&cam.trotation, 0, -5, 0);
		break;
	case SDLK_DOWN:
	case SDLK_s:
		if(shift)
			addpt3d(&cam.torigin, 0, -2.5, 0);
		else
			addpt3d(&cam.trotation, -5, 0, 0);
		break;
	case SDLK_RIGHT:
	case SDLK_d:
		if(shift)
			addpt3d(&cam.torigin, -2.5, 0, 0);
		else
			addpt3d(&cam.trotation, 0, 3.0, 0);
		break;
	case SDLK_f:
		set3d(&cam.torigin, 0, 0, 0);
		set3d(&cam.trotation, VIEWFRONT.x, VIEWFRONT.y, VIEWFRONT.z);
		break;
	case SDLK_z:
		if(shift)
			addpt3d(&cam.torigin, 0, 0, 2.5);
		else
			modrange(3.0);
		break;
	case SDLK_x:
		if(shift)
			addpt3d(&cam.torigin, 0, 0, -2.5);
		else
			modrange(-3.0);
		break;
	}
	redraw(pixels);
}

int
init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return error("Init", SDL_GetError());
	gWindow = SDL_CreateWindow("Moogle",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH * ZOOM,
		HEIGHT * ZOOM,
		SDL_WINDOW_SHOWN);
	if(gWindow == NULL)
		return error("Window", SDL_GetError());
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	if(gRenderer == NULL)
		return error("Renderer", SDL_GetError());
	gTexture = SDL_CreateTexture(gRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC,
		WIDTH,
		HEIGHT);
	if(gTexture == NULL)
		return error("Texture", SDL_GetError());
	pixels = (Uint32 *)malloc(WIDTH * HEIGHT * sizeof(Uint32));
	if(pixels == NULL)
		return error("Pixels", "Failed to allocate memory");
	clear(pixels);
	return 1;
}

int
main(int argc, char *argv[])
{
	int ticknext = 0;
	scn = Sc3d();
	cam = Cm3d(180, 20, 0);
	if(!init())
		return error("Init", "Failure");
	Mesh* umbrella = createumbrella(&scn, 1, 2);
	// createbox(&scn, 5, 5, 5, 1);

	exportmesh("umbrella", umbrella);

	// createicosaedron(&scn, 5, 4);
	// createpyramid(&scn, 6, 5, 10, 1);
	redraw(pixels);
	while(1) {
		int tick = SDL_GetTicks();
		SDL_Event event;
		if(tick < ticknext)
			SDL_Delay(ticknext - tick);
		ticknext = tick + (1000 / FPS);
		update(&cam, 5);
		while(SDL_PollEvent(&event) != 0) {
			if(event.type == SDL_QUIT)
				quit();
			else if(event.type == SDL_MOUSEBUTTONUP ||
					event.type == SDL_MOUSEBUTTONDOWN ||
					event.type == SDL_MOUSEMOTION)
				domouse(&event);
			else if(event.type == SDL_KEYDOWN)
				dokey(&event);
			else if(event.type == SDL_WINDOWEVENT)
				if(event.window.event == SDL_WINDOWEVENT_EXPOSED)
					redraw(pixels);
		}
	}
	quit();
	return 0;
}
