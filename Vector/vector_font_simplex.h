#ifndef INCLUDED_VECTOR_FONT_SIMPLEX_H
#define INCLUDED_VECTOR_FONT_SIMPLEX_H

#include "vector_display.h"

int vector_font_simplex_measure(double scale, const char *string, double *out_width, double *out_height);
int vector_font_simplex_draw(vector_display_t *display, double x, double y, double scale, const char *s);

#endif
