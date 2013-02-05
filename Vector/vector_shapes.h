#ifndef INCLUDED_VECTOR_SHAPES_H
#define INCLUDED_VECTOR_SHAPES_H

#include "vector_display.h"

#ifdef __cplusplus
extern "C" {
#endif

int vector_shape_draw_line   (vector_display_t *display, double x0, double y0, double x1, double y1);
int vector_shape_draw_box    (vector_display_t *display, double x, double y, double w, double h);
int vector_shape_draw_circle (vector_display_t *display, double x, double y, double radius, double steps);
int vector_shape_draw_wheel  (vector_display_t *display, double spokeangle, double x, double y, double radius);

// see shapetool script to generate points parameter from an svg path
int vector_shape_draw_shape  (vector_display_t *display, double *points, double x, double y, double sx, double sy, double angle);

#ifdef __cplusplus
}
#endif

#endif
