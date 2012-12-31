//
//  vector_display.h
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#ifndef Vector_vector_display_h
#define Vector_vector_display_h

typedef struct vector_display vector_display_t;

// create a new vector display
int vector_display_new(vector_display_t **out_self, float width, float height);

// delete a vector display
void vector_display_delete(vector_display_t *self);

// setup opengl, assumes context is already set
int vector_display_update(vector_display_t *self);

// setup opengl, assumes context is already set
int vector_display_setup(vector_display_t *self);

// tear down opengl, assumes context is already set
int vector_display_teardown(vector_display_t *self);

// clear display
int vector_display_clear(vector_display_t *self);

// add a vector to the display list; x/y are from [0,1)
int vector_display_draw(vector_display_t *self, float x0, float y0, float x1, float y1);

// draw connected lines
int vector_display_begin_draw(vector_display_t *self, float x, float y);
int vector_display_draw_to(vector_display_t *self, float x, float y);
int vector_display_end_draw(vector_display_t *self);

// set the drawing color
int vector_display_set_color(vector_display_t *self, float r, float g, float b);

// set number of fade steps
int vector_display_set_steps(vector_display_t *self, int steps);

// set decay per step
int vector_display_set_decay(vector_display_t *self, float decay);

// set decay on first step
int vector_display_set_initial_decay(vector_display_t *self, float initial_decay);

// set thickness
int vector_display_set_thickness(vector_display_t *self, float thickness);

#endif
