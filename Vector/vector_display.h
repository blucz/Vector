//
//  vector_display.h
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#ifndef Vector_vector_display_h
#define Vector_vector_display_h

#define VECTOR_DISPLAY_MAX_DECAY_STEPS          (60)
#define VECTOR_DISPLAY_DEFAULT_DECAY_STEPS      (5)
#define VECTOR_DISPLAY_DEFAULT_DECAY            (0.8)
#define VECTOR_DISPLAY_DEFAULT_INITIAL_DECAY    (0.04)
#define VECTOR_DISPLAY_DEFAULT_OFFSET_X         (0.0)
#define VECTOR_DISPLAY_DEFAULT_OFFSET_Y         (0.0)
#define VECTOR_DISPLAY_DEFAULT_SCALE            (1.0)
#define VECTOR_DISPLAY_DEFAULT_BRIGHTNESS       (1.0)  

#ifdef __cplusplus
extern "C" {
#endif

//
// The type of vector displays
//
typedef struct vector_display vector_display_t;

//
// Create a new vector display object.
//
int vector_display_new(vector_display_t **out_self, double width, double height);

//
// Delete a vector display object.
//
void vector_display_delete(vector_display_t *self);

//
// Tear down OpenGL state associated with the vector display.
//
// Assumes that the OpenGl context is already set and the screen's
// FBO is bound.
//
int vector_display_update(vector_display_t *self);

//
// Setup OpenGL state associated with the vector display.
//
// Assumes that the OpenGl context is already set.
//
int vector_display_setup(vector_display_t *self);

//
// Resize a vector display. 
//
// This function clears the display.
//
int vector_display_resize(vector_display_t *self, double width, double height);


//
// Tear down OpenGL state associated with the vector display.
//
// Assumes that the OpenGl context is already set.
//
int vector_display_teardown(vector_display_t *self);

//
// Clear the display.
//
int vector_display_clear(vector_display_t *self);

//
// Draw a series of connected line segments.
//
int vector_display_begin_draw(vector_display_t *self, double x, double y);
int vector_display_draw_to(vector_display_t *self, double x, double y);
int vector_display_end_draw(vector_display_t *self);

//
// Set the current drawing color
//
int vector_display_set_color(vector_display_t *self, double r, double g, double b);

//
// Set the number of frames of decay/fade to apply to the scene.
//
int vector_display_set_decay_steps(vector_display_t *self, int steps);

//
// Set the brightness multipler applied on each decay frame after the first.
//
int vector_display_set_decay(vector_display_t *self, double decay);

//
// Set the brightness multipler applied on the first decay frame.
//
int vector_display_set_initial_decay(vector_display_t *self, double initial_decay);

//
// Set a 2d transformation for the display.
//
// This relates logical coordinates, as passed to vector_display_begin_draw, 
// vector_display_draw_to, and vector_display_draw, to coordinates from (0,0) 
// to (width,height) in the destination framebuffer.
//
// The parameters impact coordinates as follows:
//
//      framebuffer_x = x * scale + offset_x
//      framebuffer_y = y * scale + offset_y
//
int vector_display_set_transform(vector_display_t *self, double offset_x, double offset_y, double scale);

//
// Set the line thickness. 
//
// The line thickness is measured in scene coordinates, and includes all pixels lit by 
// the line before any post-processing. The apparent width of the line to the viewer 
// is significantly narrower, since brightness decays exponentially to zero within the 
// bounds of the line.
//
// Thickness, by default, is guessed based on width and height.
//
// This function clears the display.
//
int vector_display_set_thickness(vector_display_t *self, double thickness);
int vector_display_set_default_thickness(vector_display_t *self);

//
// Set the "brightness" of the display
//
// useful values range from [0.5, 1.5]. 0.0 disables all glow effects.
//
// Due to implementation details of the glow effect, glow is related to 
// the pixel density of the framebuffer. It may require adjustment, 
// particularly when moving between devices of very different pixel density.
//
int vector_display_set_brightness(vector_display_t *self, double brightness);

//
// Get the size from a vector display.
//
void vector_display_get_size(vector_display_t *self, double *out_width, double *out_height);

//
// Install a custom logging function for the vector display library.
//
// By default, vector display logs to stderr. On platforms where this isn't suitable,
// reaplce the default logger with a custom log function.
//
// Set it back to null to return to the default.
//
typedef void (*vector_display_log_cb_t)(const char *msg);
void vector_display_set_log_cb(vector_display_log_cb_t cb_log);

#ifdef __cplusplus
}
#endif

#endif
