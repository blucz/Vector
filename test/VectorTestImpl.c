#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector_display_glinc.h"
#include "vector_display.h"
#include "vector_font_simplex.h"

static vector_display_t *display;
static int dwidth;
static int dheight;

static double scale = 1.0;
static int offsetx;
static int offsety;

static void draw_wheel(vector_display_t *display, double angle, double x, double y, double radius) {
    double spokeradius = radius - 2.0f;
    // draw spokes
    vector_display_draw(display,
                        x + spokeradius * sin(angle),
                        y - spokeradius * cos(angle),
                        x - spokeradius * sin(angle),
                        y + spokeradius * cos(angle));
    vector_display_draw(display,
                        x + spokeradius * sin(angle + M_PI / 4.0f),
                        y - spokeradius * cos(angle + M_PI / 4.0f),
                        x - spokeradius * sin(angle + M_PI / 4.0f),
                        y + spokeradius * cos(angle + M_PI / 4.0f));
    vector_display_draw(display,
                        x + spokeradius * sin(angle + M_PI / 2.0f),
                        y - spokeradius * cos(angle + M_PI / 2.0f),
                        x - spokeradius * sin(angle + M_PI / 2.0f),
                        y + spokeradius * cos(angle + M_PI / 2.0f));
    vector_display_draw(display,
                        x + spokeradius * sin(angle + 3.0f * M_PI / 4.0f),
                        y - spokeradius * cos(angle + 3.0f * M_PI / 4.0f),
                        x - spokeradius * sin(angle + 3.0f * M_PI / 4.0f),
                        y + spokeradius * cos(angle + 3.0f * M_PI / 4.0f));

    double edgeangle = 0.0f;
    double angadjust = 0.0f;

    vector_display_begin_draw(display,
                              x + radius * sin(angle + edgeangle + angadjust),
                              y - radius * cos(angle + edgeangle + angadjust));
    for (edgeangle = 0; edgeangle < 2 * M_PI - 0.001; edgeangle += M_PI/4.0f) {
        vector_display_draw_to(display,
                               x + radius * sin(angle + edgeangle + M_PI / 4.0f - angadjust),
                               y - radius * cos(angle + edgeangle + M_PI / 4.0f - angadjust));
    }
    vector_display_end_draw(display);
}

static void draw_circle(vector_display_t *display, double x, double y, double radius, double steps) {
    double edgeangle = 0.0f;
    double angadjust = 0.0f;

    double step = M_PI * 2 / steps;

    vector_display_begin_draw(display,
                              x + radius * sin(edgeangle + angadjust),
                              y - radius * cos(edgeangle + angadjust));
    for (edgeangle = 0; edgeangle < 2 * M_PI - 0.001; edgeangle += step) {
        vector_display_draw_to(display,
                               x + radius * sin(edgeangle + step - angadjust),
                               y - radius * cos(edgeangle + step - angadjust));
    }
    vector_display_end_draw(display);
}

static void draw_box(vector_display_t *display, double x, double y, double w, double h) {
    vector_display_begin_draw(display, x, y);
    vector_display_draw_to(display, x + w, y);
    vector_display_draw_to(display, x + w, y + h);
    vector_display_draw_to(display, x, y + h);
    vector_display_draw_to(display, x, y);
    vector_display_end_draw(display);
}

static int fixx(int x) {
    return (int)((double)x * scale) + offsetx;
}
static int fixy(int y) {
    return (int)((double)y * scale) + offsety;
}
static double fix(double v) {
    return v * scale;
}

void
VectorTestImpl_Draw()
{
    int i, j;

    vector_display_clear(display);

    char buf[1024];
    sprintf(buf, "1234567890    size: %dx%d ", dwidth, dheight);

    //
    // test pattern for simplex font
    //
    //vector_font_simplex_draw(display, 100, 1300, 5.0, "Hello, World!");
    vector_display_set_color(display, 0.7f, 0.7f, 1.0f);
    vector_font_simplex_draw(display, fixx(50), fixy(180), fix(3.5), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    vector_font_simplex_draw(display, fixx(50), fixy(360), fix(3.5), "abcdefghijklmnopqrstuvwxyz");
    vector_font_simplex_draw(display, fixx(50), fixy(540), fix(3.5), buf);
    vector_font_simplex_draw(display, fixx(50), fixy(720), fix(3.5), "!@#$%^&*()-=<>/?;:'\"{}[]|\\+=-_");


    //
    // test pattern for lines
    //
    vector_display_set_color(display, 1.0f, 0.7f, 0.7f);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            vector_display_draw(display, fixx(50),  fixy(750 + 100 * i), fixx(400), fixy(750 + 100 * i));     // draw line
            vector_display_draw(display, fixx(500), fixy(750 + 100 * i), fixx(500), fixy(750 + 100 * i));     // draw dot
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j <= i; j++) {
            vector_display_draw(display, fixx(50 + 100 * i), fixy(1200), fixx(50 + 100 * i), fixy(1400));     // draw line
            vector_display_draw(display, fixx(50 + 100 * i), fixy(1450), fixx(50 + 100 * i), fixy(1450));     // draw dot
        }
    }

    //
    // test pattern for shapes
    //
    vector_display_set_color(display, 0.7f, 0.7f, 1.0f);
    draw_circle(display, fixx(500),  fixy(950), fix(20),  32);
    draw_circle(display, fixx(600),  fixy(950), fix(50),  32);
    draw_circle(display, fixx(800),  fixy(950), fix(100), 32);
    draw_circle(display, fixx(1075), fixy(950), fix(150), 64);

    vector_display_set_color(display, 0.7f, 1.0f, 0.7f);
    draw_box(display, fixx(500), fixy(1200), fix(40),  fix(40));
    draw_box(display, fixx(565), fixy(1200), fix(100), fix(100));
    draw_box(display, fixx(700), fixy(1200), fix(200), fix(200));
    draw_box(display, fixx(950), fixy(1200), fix(300), fix(300));

    vector_display_set_color(display, 1.0f, 0.7f, 1.0f);
    draw_wheel(display, M_PI,         fixx(1425), fixy(950), fix(150));
    draw_wheel(display, 3 * M_PI / 4, fixx(1700), fixy(950), fix(100));
    draw_wheel(display, M_PI / 2,     fixx(1900), fixy(950), fix(50));
    draw_wheel(display, M_PI / 4,     fixx(2000), fixy(950), fix(20));

    //
    // finish
    //
    int rc;
    rc = vector_display_update(display);
    if (rc != 0) {
        printf("Failed to update vector display: rc=%d", rc);
        exit(1);
    }

    glFlush(); //Write this out to the screen
}

static void resize(int w, int h)
{
    dwidth = w;
    dheight = h;

    if (((double)dwidth / (double)dheight) < (2048.0/1536.0)) {
        scale = (double)dwidth / 2048.0;
        offsetx = 0;
        offsety = (dheight - (int)((double)1536 * scale)) / 2;
    } else {
        scale = (double)dheight / 1536.0;
        offsetx = (dwidth - (int)((double)2048 * scale)) / 2;
        offsety = 0;
    }
}

void
VectorTestImpl_Init(int w, int h)
{
    int rc;
    rc = vector_display_new(&display, w, h);
    if (rc != 0) {
        printf("Failed to create vector display: rc=%d", rc);
        exit(1);
    }

    rc = vector_display_setup(display);
    if (rc != 0) {
        printf("Failed to setup vector display: rc=%d", rc);
        exit(1);
    }

    resize(w, h);
}

void
VectorTestImpl_Resize(int w, int h)
{
    vector_display_resize(display, w, h);
    resize(w,h);
}

void
VectorTestImpl_Destroy()
{
    int rc;
    rc = vector_display_teardown(display);
    if (rc != 0) {
        printf("Failed to tear down vector display: rc=%d", rc);
        exit(1);
    }
    vector_display_delete(display);
}
