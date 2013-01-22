#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vector_display.h"
#include "vector_font_simplex.h"

vector_display_t *display;

void myInit() {
    int rc;
    rc = vector_display_new(&display, 2048, 1536);
    if (rc != 0) {
        printf("Failed to create vector display: rc=%d", rc);
        exit(1);
    }

    rc = vector_display_setup(display);
    if (rc != 0) {
        printf("Failed to setup vector display: rc=%d", rc);
        exit(1);
    }
}

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

void myDisplay() {
    int i, j;

    vector_display_clear(display);

    //
    // test pattern for simplex font
    //
    //vector_font_simplex_draw(display, 100, 1300, 5.0, "Hello, World!");
    vector_display_set_color(display, 0.7f, 0.7f, 1.0f);
    vector_font_simplex_draw(display, 50, 180, 3.5, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    vector_font_simplex_draw(display, 50, 360, 3.5, "abcdefghijklmnopqrstuvwxyz");
    vector_font_simplex_draw(display, 50, 540, 3.5, "1234567890");
    vector_font_simplex_draw(display, 50, 720, 3.5, "!@#$%^&*()-=<>/?;:'\"{}[]|\\+=-_");

    //
    // test pattern for lines
    //
    vector_display_set_color(display, 1.0f, 0.7f, 0.7f);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            vector_display_draw(display, 50, 750 + 100 * i, 400, 750 + 100 * i);     // draw line
            vector_display_draw(display, 500, 750 + 100 * i, 500, 750 + 100 * i);     // draw dot
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j <= i; j++) {
            vector_display_draw(display, 50 + 100 * i, 1200, 50 + 100 * i, 1400);     // draw line
            vector_display_draw(display, 50 + 100 * i, 1450, 50 + 100 * i, 1450);     // draw dot
        }
    }

    //
    // test pattern for shapes
    //
    vector_display_set_color(display, 0.7f, 0.7f, 1.0f);
    draw_circle(display, 500, 950, 20, 32);
    draw_circle(display, 600, 950, 50, 32);
    draw_circle(display, 800, 950, 100, 32);
    draw_circle(display, 1075, 950, 150, 64);

    vector_display_set_color(display, 0.7f, 1.0f, 0.7f);
    draw_box(display, 500, 1200, 40, 40);
    draw_box(display, 565, 1200, 100, 100);
    draw_box(display, 700, 1200, 200, 200);
    draw_box(display, 950, 1200, 300, 300);

    vector_display_set_color(display, 1.0f, 0.7f, 1.0f);
    draw_wheel(display, M_PI,         1425, 950, 150);
    draw_wheel(display, 3 * M_PI / 4, 1700,  950, 100);
    draw_wheel(display, M_PI / 2,     1900,  950, 50);
    draw_wheel(display, M_PI / 4,     2000,  950, 20);

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

int main (int argc, char **argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(100,100);
    glutCreateWindow("Vector");

    glutDisplayFunc(myDisplay);

    myInit();

    glutMainLoop();

    return 0;
}

