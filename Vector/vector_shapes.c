#include <math.h>

#include "vector_shapes.h"
#include "vector_display.h"

int
vector_shape_draw_line(vector_display_t *display, double x0, double y0, double x1, double y1)
{
    vector_display_begin_draw(display, x0, y0);
    vector_display_draw_to(display, x1, y1);
    vector_display_end_draw(display);
    return 0;
}

int
vector_shape_draw_wheel(vector_display_t *display, double angle, double x, double y, double radius)
{
    double spokeradius = radius - 2.0f;
    // draw spokes
    vector_shape_draw_line(display,
              x + spokeradius * sin(angle),
              y - spokeradius * cos(angle),
              x - spokeradius * sin(angle),
              y + spokeradius * cos(angle));
    vector_shape_draw_line(display,
              x + spokeradius * sin(angle + M_PI / 4.0f),
              y - spokeradius * cos(angle + M_PI / 4.0f),
              x - spokeradius * sin(angle + M_PI / 4.0f),
              y + spokeradius * cos(angle + M_PI / 4.0f));
    vector_shape_draw_line(display,
              x + spokeradius * sin(angle + M_PI / 2.0f),
              y - spokeradius * cos(angle + M_PI / 2.0f),
              x - spokeradius * sin(angle + M_PI / 2.0f),
              y + spokeradius * cos(angle + M_PI / 2.0f));
    vector_shape_draw_line(display,
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
    return 0;
}

int
vector_shape_draw_circle(vector_display_t *display, double x, double y, double radius, double steps)
{
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
    return 0;
}

int
vector_shape_draw_box(vector_display_t *display, double x, double y, double w, double h)
{
    vector_display_begin_draw(display, x, y);
    vector_display_draw_to(display, x + w, y);
    vector_display_draw_to(display, x + w, y + h);
    vector_display_draw_to(display, x, y + h);
    vector_display_draw_to(display, x, y);
    vector_display_end_draw(display);
    return 0;
}

int
vector_shape_draw_shape(vector_display_t *display, double *points, double x, double y, double sx, double sy, double angle)
{
    double cs = cos(angle);
    double sn = sin(angle);

    int i = 0;
    int total = (int)points[i++];
    while (total > 0) {
        int vcnt = (int)points[i++];

        double xx = points[i] * sx;
        double yy = points[i+1] * sy;
        double rx = xx * cs - yy * sn;
        double ry = xx * sn + yy * cs;
        vector_display_begin_draw(display, rx + x, ry + y);
        i += 2;
        int j = 1;
        while (j < vcnt) {
            xx = points[i] * sx;
            yy = points[i+1] * sy;
            rx = xx * cs - yy * sn;
            ry = xx * sn + yy * cs;
            vector_display_draw_to(display, rx + x, ry + y);
            j++;
            i += 2;
        }
        vector_display_end_draw(display);
        total -= vcnt;
    }
    return 0;
}
