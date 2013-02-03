#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector_display_glinc.h"
#include "vector_display.h"
#include "vector_font_simplex.h"
#include "vector_shapes.h"

static vector_display_t *display;
static int dwidth;
static int dheight;

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
    vector_font_simplex_draw(display, (50), (180), (3.5), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    vector_font_simplex_draw(display, (50), (360), (3.5), "abcdefghijklmnopqrstuvwxyz");
    vector_font_simplex_draw(display, (50), (540), (3.5), buf);
    vector_font_simplex_draw(display, (50), (720), (3.5), "!@#$%^&*()-=<>/?;:'\"{}[]|\\+=-_");


    //
    // test pattern for lines
    //
    vector_display_set_color(display, 1.0f, 0.7f, 0.7f);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            vector_shape_draw_line(display, (50),  (750 + 100 * i), (400), (750 + 100 * i));     
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j <= i; j++) {
            vector_shape_draw_line(display, (50 + 100 * i), (1200), (50 + 100 * i), (1400));    
        }
    }

    //
    // test pattern for shapes
    //
    vector_display_set_color(display, 0.7f, 0.7f, 1.0f);
    vector_shape_draw_circle(display, (500),  (950), (20),  32);
    vector_shape_draw_circle(display, (600),  (950), (50),  32);
    vector_shape_draw_circle(display, (800),  (950), (100), 32);
    vector_shape_draw_circle(display, (1075), (950), (150), 64);

    vector_display_set_color(display, 0.7f, 1.0f, 0.7f);
    vector_shape_draw_box(display, (500), (1200), (40),  (40));
    vector_shape_draw_box(display, (565), (1200), (100), (100));
    vector_shape_draw_box(display, (700), (1200), (200), (200));
    vector_shape_draw_box(display, (950), (1200), (300), (300));

    vector_display_set_color(display, 1.0f, 0.7f, 1.0f);
    vector_shape_draw_wheel(display, M_PI,         (1425), (950), (150));
    vector_shape_draw_wheel(display, 3 * M_PI / 4, (1700), (950), (100));
    vector_shape_draw_wheel(display, M_PI / 2,     (1900), (950), (50));
    vector_shape_draw_wheel(display, M_PI / 4,     (2000), (950), (20));

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
        double scale = (double)dwidth / 2048.0;
        vector_display_set_transform(display, 0, (dheight - (int)((double)1536 * scale)) / 2, scale);
    } else {
        double scale = (double)dheight / 1536.0;
        vector_display_set_transform(display, (dwidth - (int)((double)2048 * scale)) / 2, 0, scale);
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
