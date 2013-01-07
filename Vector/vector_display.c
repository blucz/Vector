//
//  vector_display.c
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#include "vector_display.h"
#include "vector_display_platform.h"
#include "vector_display_utils.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define MAX_STEPS 60
#define DEFAULT_STEPS 5
#define DEFAULT_DECAY 0.8
#define DEFAULT_INITIAL_DECAY 0.04
#define DEFAULT_THICKNESS 8.0f
#define TEXTURE_SIZE 128
#define HALF_TEXTURE_SIZE (TEXTURE_SIZE/2)

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

typedef struct {
    float x, y, z;
    float u, v;
} nocolor_point_t;

typedef struct {
    float x, y, z;
    float r, g, b, a;
    float u, v;
} point_t;

typedef struct {
    double x, y;
} pending_point_t;

struct vector_display {
    GLuint fb_program;       // program for drawing to the fb
    GLuint fb_uniform_modelview;
    GLuint fb_uniform_projection;
    GLuint fb_uniform_alpha;

    GLuint screen_program;       // program for blitting to the screen
    GLuint screen_uniform_modelview;
    GLuint screen_uniform_projection;
    GLuint screen_uniform_alpha;
    GLuint screen_uniform_mult;         

    GLuint fb_scene;                  // framebuffer object
    GLuint fb_scene_texid;

    GLuint blur_program;       // program for gaussian blur
    GLuint blur_uniform_modelview;
    GLuint blur_uniform_projection;
    GLuint blur_uniform_scale;
    GLuint blur_uniform_alpha;
    GLuint blur_uniform_mult;         

    GLuint fb_glow0;            // framebuffer for glow0
    GLuint fb_glow0_texid;      // texture for blur

    GLuint fb_glow1;            // framebuffer for glow1
    GLuint fb_glow1_texid;      // texture for blur

    double width, height;
    double glow_width, glow_height;

    int steps;
    double decay;
    double r, g, b, a;

    int nvectors;

    size_t cpoints;
    size_t npoints;
    point_t *points;

    size_t pending_cpoints;
    size_t pending_npoints;
    pending_point_t *pending_points;

    int step;
    GLuint *buffers;
    GLuint *buffernpoints;

    GLuint screen_vertexbuffer;
    GLuint glow_vertexbuffer;

    GLuint linetexid;

    int did_setup;

    double initial_decay;
    double thickness;
};

#define VERTEX_POS_INDEX       (0)
#define VERTEX_COLOR_INDEX     (1)
#define VERTEX_TEXCOORD_INDEX  (2)

int vector_display_new(vector_display_t **out_self, double width, double height) {
    vector_display_t *self = (vector_display_t*)calloc(sizeof(vector_display_t), 1);
    if (self == NULL) return -1;
    self->steps = DEFAULT_STEPS;
    self->buffers = (GLuint*)calloc(sizeof(GLuint), self->steps);
    self->buffernpoints = (GLuint*)calloc(sizeof(GLuint), self->steps);

    self->cpoints = 60;
    self->points = (point_t*)calloc(sizeof(point_t), self->cpoints);

    self->pending_cpoints = 60;
    self->pending_points = (pending_point_t*)calloc(sizeof(pending_point_t), self->pending_cpoints);

    self->decay = DEFAULT_DECAY;
    self->r = self->g = self->b = self->a = 1.0f;
    self->width = width;
    self->height = height;
    self->glow_width = width   / 3.0; 
    self->glow_height = height / 3.0; 
    self->initial_decay = DEFAULT_INITIAL_DECAY;
    self->thickness = DEFAULT_THICKNESS;
    *out_self = self;
    return 0;
}

int vector_display_set_initial_decay(vector_display_t *self, double initial_decay) {
    if (initial_decay < 0.0f || initial_decay >= 1.0f) return -1;
    self->initial_decay = initial_decay;
    return 0;
}

int vector_display_set_thickness(vector_display_t *self, double thickness) {
    if (thickness <= 0) return -1;
    self->thickness = thickness;
    return 0;
}

int vector_display_clear(vector_display_t *self) {
    self->npoints = 0;
    return 0;
}

int vector_display_set_color(vector_display_t *self, double r, double g, double b) {
    self->r = r;
    self->g = g;
    self->b = b;
    return 0;
}

static void ensure_pending_points(vector_display_t *self, int npoints) {
    if (self->pending_cpoints < npoints) {
        pending_point_t *newpoints = (pending_point_t*)calloc(sizeof(pending_point_t), self->pending_cpoints * 2);
        self->pending_cpoints *= 2;
        memcpy(newpoints, self->points, sizeof(pending_point_t) * self->pending_npoints);
        free(self->pending_points);
        self->pending_points = newpoints;
    }
}

static void ensure_points(vector_display_t *self, int npoints) {
    if (self->cpoints < npoints) {
        point_t *newpoints = (point_t*)calloc(sizeof(point_t), self->cpoints * 2);
        self->cpoints *= 2;
        memcpy(newpoints, self->points, sizeof(point_t) * self->npoints);
        free(self->points);
        self->points = newpoints;
    }
}

static void append_texpoint(vector_display_t *self, double x, double y, double u, double v) {
    ensure_points(self, self->npoints + 1);

    self->points[self->npoints].x = x;
    self->points[self->npoints].y = y;
    self->points[self->npoints].z = 10000.0;
    self->points[self->npoints].r = self->r;
    self->points[self->npoints].g = self->g;
    self->points[self->npoints].b = self->b;
    self->points[self->npoints].a = self->a;
    self->points[self->npoints].u = u / TEXTURE_SIZE;
    self->points[self->npoints].v = 1.0f - v / TEXTURE_SIZE;
    self->npoints++;
}

int vector_display_begin_draw(vector_display_t *self, double x, double y) {
    if (self->pending_npoints != 0) {
        debugf("assertion failure");
        abort();
    }
    ensure_pending_points(self, self->pending_npoints + 1);
    self->pending_points[self->pending_npoints].x = x;
    self->pending_points[self->pending_npoints].y = y;
    self->pending_npoints++;
    return 0;
}

int vector_display_draw_to(vector_display_t *self, double x, double y) {
    ensure_pending_points(self, self->pending_npoints + 1);
    self->pending_points[self->pending_npoints].x = x;
    self->pending_points[self->pending_npoints].y = y;
    self->pending_npoints++;
    return 0;
}

static float normalizef(float a) {
    while (a > 2*M_PI + FLT_EPSILON) a -= 2*M_PI;
    while (a < 0 - FLT_EPSILON)      a += 2*M_PI;
    return a;
}

static double normalize(double a) {
    while (a > 2*M_PI + DBL_EPSILON) a -= 2*M_PI;
    while (a < 0 - DBL_EPSILON)      a += 2*M_PI;
    return a;
}

typedef struct {
    float x0, y0, x1, y1;                      // nominal points
    float a;                                   // angle
    float sin_a, cos_a;                        // precomputed trig

    float xl0, yl0, xl1, yl1;                  // left side of the box
    float xr0, yr0, xr1, yr1;                  // right side of the box

    int is_first, is_last;
    int has_next, has_prev;                     // booleans indicating whether this line connects to prev/next
    float prev_ad, next_ad;                    // angle differences to prev/next (if has_prev/has_next)

    float xlt0, ylt0, xlt1, ylt1;              // coordinates of endcaps (if !has_prev/!has_next)
    float xrt0, yrt0, xrt1, yrt1;              // coordinates of endcaps (if !has_prev/!has_next)

    float len;
} line_t;

void draw_simple_fan(vector_display_t *self, float cx, float cy, float pa, float a, float t) {
    float *angles;
    int     nsteps;
    float  pa2a        = normalizef(a - pa);
    float  a2pa        = normalizef(pa - a);

    float s = 0;
    float e = 0;

    int i;
    if (a2pa < pa2a) {
        t = -t;
        s = TEXTURE_SIZE;
        e = 0;
        nsteps = max(1, round(a2pa / (M_PI / 8)));
        angles = alloca(sizeof(float) * (nsteps + 1));
        for (i = 0; i <= nsteps; i++)
            angles[i] = a + i * a2pa / nsteps;
        //debugf("%fd in %d steps", a2pa, nsteps);
    } else {
        s = 0;
        e = TEXTURE_SIZE;
        nsteps = max(1, round(pa2a / (M_PI / 8)));
        angles = alloca(sizeof(float) * (nsteps + 1));
        for (i = 0; i <= nsteps; i++)
            angles[i] = pa + i * pa2a / nsteps;
        //debugf("%fd in %d steps", pa2a, nsteps);
    }
    //debugf("---- %f -> %f nsteps=%d", 360*pa/M_PI/2, 360*a/M_PI/2, 360*pa2a/M_PI/2, 360*a2pa/M_PI/2, nsteps);

    for (i = 1; i <= nsteps; i++) {
        append_texpoint(self, cx + t * sin(angles[i-1]), cy - t * cos(angles[i-1]), s, HALF_TEXTURE_SIZE);
        append_texpoint(self, cx, cy,                                               e, HALF_TEXTURE_SIZE);
        append_texpoint(self, cx + t * sin(angles[i]),   cy - t * cos(angles[i]),   s, HALF_TEXTURE_SIZE);
    }
}

static void draw_lines(vector_display_t *self, line_t *lines, int nlines) {
    int    i;
    float t = self->thickness;

    for (i = 0; i < nlines; i++) {
        line_t *line  = &lines[i], *pline = &lines[(nlines+i-1)%nlines];

        if (line->has_prev) {   // draw fan for connection to previous
            float  pa2a = normalizef(pline->a -  line->a);
            float  a2pa = normalizef( line->a - pline->a);
            if (a2pa < pa2a) {
                draw_simple_fan(self, line->xr0, line->yr0, pline->a, line->a, t*2);
            } else {
                draw_simple_fan(self, line->xl0, line->yl0, pline->a, line->a, t*2);
            }
        }

#if 1
        append_texpoint(self, line->xr0,  line->yr0,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xr1,  line->yr1,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl1,  line->yl1,  0.0f,         HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl0,  line->yl0,  0.0f,         HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xr0,  line->yr0,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl1,  line->yl1,  0.0f,         HALF_TEXTURE_SIZE);
#else
        self->a = 0.8;
        append_texpoint(self, line->xr0,  line->yr0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xr1,  line->yr1,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl1,  line->yl1,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl0,  line->yl0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xr0,  line->yr0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, line->xl1,  line->yl1,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        self->a = 1.0;
#endif

        if (!line->has_prev) { // draw startcap
            append_texpoint(self, line->xl0,  line->yl0,  0.0f,         HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xlt0, line->ylt0, 0.0f,         0.0f);    
            append_texpoint(self, line->xr0,  line->yr0,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xr0,  line->yr0,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xlt0, line->ylt0, 0.0f,         0.0f);
            append_texpoint(self, line->xrt0, line->yrt0, TEXTURE_SIZE, 0.0f);   
        } 

        if (!line->has_next) { // draw endcap
            append_texpoint(self, line->xlt1, line->ylt1, 0.0f,         0.0f);    
            append_texpoint(self, line->xl1,  line->yl1,  0.0f,         HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xr1,  line->yr1,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xlt1, line->ylt1, 0.0f,         0.0f);
            append_texpoint(self, line->xr1,  line->yr1,  TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, line->xrt1, line->yrt1, TEXTURE_SIZE, 0.0f);
        }

    }
}

int vector_display_end_draw(vector_display_t *self) {
    if (self->pending_npoints < 2) { 
        self->pending_npoints = 0;
        return 0;
    }

    float t = self->thickness;
    int i;
    int  first_last_same = abs(self->pending_points[0].x - self->pending_points[self->pending_npoints-1].x) < 0.1 &&
                           abs(self->pending_points[0].y - self->pending_points[self->pending_npoints-1].y) < 0.1;

    // from the list of points, build a list of lines
    int     nlines = self->pending_npoints-1;
    line_t *lines  = (line_t*)alloca((self->pending_npoints-1) * sizeof(line_t));

    for (i = 1; i < self->pending_npoints; i++) {
        line_t *line = &lines[i-1];
        line->is_first = i == 1;
        line->is_last  = i == self->pending_npoints - 1;

        // precomputed info for current line
        line->x0    = self->pending_points[i-1].x;    
        line->y0    = self->pending_points[i-1].y;     
        line->x1    = self->pending_points[i].x;        
        line->y1    = self->pending_points[i].y;         
        line->a     = atan2(line->y1 - line->y0, line->x1 - line->x0); // angle from positive x axis, increasing ccw, [-pi, pi]
        line->sin_a = sin(line->a);
        line->cos_a = cos(line->a);
        line->len  = sqrt(pow(line->x1-line->x0, 2) + pow(line->y1-line->y0, 2));

        // compute initial values for left,right,leftcenter,rightcenter points 
        line->xl0 = line->x0 + t * line->sin_a; line->yl0 = line->y0 - t * line->cos_a;
        line->xr0 = line->x0 - t * line->sin_a; line->yr0 = line->y0 + t * line->cos_a;
        line->xl1 = line->x1 + t * line->sin_a; line->yl1 = line->y1 - t * line->cos_a;
        line->xr1 = line->x1 - t * line->sin_a; line->yr1 = line->y1 + t * line->cos_a;
    }

    for (i = 0; i < nlines; i++) {
        line_t *line  = &lines[i], *pline = &lines[(nlines+i-1)%nlines], *nline = &lines[(i+1)%nlines];

        // figure out what connections we have
        line->has_prev = (!line->is_first || (line->is_first && first_last_same));
        line->has_next = (!line->is_last  || (line->is_last  && first_last_same));

        line->prev_ad = normalizef(line->a - pline->a);
        line->next_ad = normalizef(line->a - nline->a);
        
        if (line->has_prev) { 
            float  pa2a = normalizef(pline->a -  line->a);
            float  a2pa = normalizef( line->a - pline->a);
            if (a2pa <= (M_PI / 2 + FLT_EPSILON) || pa2a <= (M_PI / 2 + FLT_EPSILON)) {
                if (a2pa < pa2a) {
                    float u = t * sin(a2pa/2) / cos(a2pa/2);
                    line->xr0  = line->xr0  + u * line->cos_a; line->yr0  = line->yr0  + u * line->sin_a;
                    line->xl0  = line->xl0  + u * line->cos_a; line->yl0  = line->yl0  + u * line->sin_a;
                } else {
                    float u = t * sin(pa2a/2) / cos(pa2a/2);
                    line->xl0  = line->xl0  + u * line->cos_a; line->yl0  = line->yl0  + u * line->sin_a;
                    line->xr0  = line->xr0  + u * line->cos_a; line->yr0  = line->yr0  + u * line->sin_a;
                }
            } else {
                line->has_prev = 0;
            }
        }

        if (line->has_next) {
            float  na2a = normalizef(nline->a -  line->a);
            float  a2na = normalizef( line->a - nline->a);
            if (a2na <= (M_PI / 2 + FLT_EPSILON) || na2a <= (M_PI / 2 + FLT_EPSILON)) {
                if (a2na < na2a) {
                    float u = t * sin(a2na/2) / cos(a2na/2);
                    line->xl1  = line->xl1  - u * line->cos_a; line->yl1  = line->yl1  - u * line->sin_a;
                    line->xr1  = line->xr1  - u * line->cos_a; line->yr1  = line->yr1  - u * line->sin_a;
                } else {
                    float u = t * sin(na2a/2) / cos(na2a/2);
                    line->xr1  = line->xr1  - u * line->cos_a; line->yr1  = line->yr1  - u * line->sin_a;
                    line->xl1  = line->xl1  - u * line->cos_a; line->yl1  = line->yl1  - u * line->sin_a;
                }
            } else {
                line->has_next = 0;
            }
        }

        if (!line->has_prev) {
            line->xlt0 = line->xl0 - t * line->cos_a; 
            line->ylt0 = line->yl0 - t * line->sin_a;
            line->xrt0 = line->xr0 - t * line->cos_a; 
            line->yrt0 = line->yr0 - t * line->sin_a;
        }

        if (!line->has_next) {
            line->xlt1 = line->xl1 + t * line->cos_a; 
            line->ylt1 = line->yl1 + t * line->sin_a;
            line->xrt1 = line->xr1 + t * line->cos_a;
            line->yrt1 = line->yr1 + t * line->sin_a;
        }
    }

    // draw the lines
    draw_lines(self, lines, nlines);

    self->pending_npoints = 0;

    return 0;
}

int vector_display_draw(vector_display_t *self, double x0, double y0, double x1, double y1) {
    vector_display_begin_draw(self, x0, y0);
    vector_display_draw_to(self, x1, y1);
    vector_display_end_draw(self);
    return 0;
}

int vector_display_set_steps(vector_display_t *self, int steps) {
    if (steps < 0 || steps > MAX_STEPS) return -1;
    if (self->did_setup) {
        glDeleteBuffers(self->steps, self->buffers);
    }
    free(self->buffers);
    free(self->buffernpoints);
    self->step = 0;
    self->steps = steps;
    self->buffers = (GLuint*)calloc(sizeof(GLuint), self->steps);
    self->buffernpoints = (GLuint*)calloc(sizeof(GLuint), self->steps);
    if (self->did_setup) {
        glGenBuffers(self->steps, self->buffers);
    }
    return 0;
}

int vector_display_set_decay(vector_display_t *self, double decay) {
    if (decay < 0.0f || decay >= 1.0f) return -1;
    self->decay = decay;
    return 0;
}

void gen_linetex(vector_display_t *self) {
    // generate the texture
    hfloat texbuf[TEXTURE_SIZE * TEXTURE_SIZE * 4];
    memset(texbuf, 0xff, sizeof(texbuf));
    int x,y;
    for (x = 0; x < TEXTURE_SIZE; x++) {
        for (y = 0; y < TEXTURE_SIZE; y++) {
            double distance = sqrt(pow(x-HALF_TEXTURE_SIZE, 2) + pow(y-HALF_TEXTURE_SIZE, 2)) / (double)HALF_TEXTURE_SIZE;

            if (distance > 1.0) distance = 1.0;

            double line = pow(16, -2 * distance);
            double glow = pow(2,  -4 * distance) / 10.0;          
            glow = 0;
            double val  = max(0, min(1, line + glow));                  // clamp

            //if (distance < 0.1) val = 1.0;

            texbuf[(x + y * TEXTURE_SIZE) * 4 + 0] = float_to_hfloat(1.0); 
            texbuf[(x + y * TEXTURE_SIZE) * 4 + 1] = float_to_hfloat(1.0); 
            texbuf[(x + y * TEXTURE_SIZE) * 4 + 2] = float_to_hfloat(1.0); 
            texbuf[(x + y * TEXTURE_SIZE) * 4 + 3] = float_to_hfloat(val);
        }
    }

    // load and bind the texture
    glGenTextures(1, &self->linetexid);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, self->linetexid);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_HALF_FLOAT_OES, texbuf);
    vector_display_check_error("glTexImage2D");
}

int vector_display_setup(vector_display_t *self) {    
    const char *nocolor_vertex_shader_text =
    "    uniform mat4 inProjectionMatrix;       \n"
    "    uniform mat4 inModelViewMatrix;        \n"
    "                                           \n"
    "    attribute vec2 inTexCoord;             \n"
    "    attribute vec4 inPosition;             \n"
    "                                           \n"
    "    varying vec2 TexCoord;                 \n"
    "                                           \n"
    "    void main() {                          \n"
    "        gl_Position = inProjectionMatrix * inModelViewMatrix * inPosition;\n"
    "        TexCoord    = inTexCoord;          \n"
    "    }\n";

    const char *blit_fragment_shader_text =
    "    precision mediump float;               \n"
    "    uniform sampler2D tex1;                \n"
    "    varying vec2 TexCoord;                 \n"
    "    uniform float alpha;                   \n"
    "    uniform float mult;                    \n"
    "                                           \n"
    "    void main() {                          \n"
    "        gl_FragColor = texture2D(tex1, TexCoord.st) * vec4(mult, mult, mult, alpha*mult);\n"
    "    }                                      \n";
    
    const char *fb_vertex_shader_text =
    "    uniform mat4 inProjectionMatrix;       \n"
    "    uniform mat4 inModelViewMatrix;        \n"
    "                                           \n"
    "    attribute vec2 inTexCoord;             \n"
    "    attribute vec4 inPosition;             \n"
    "    attribute vec4 inColor;                \n"
    "                                           \n"
    "    varying vec4 Color;                    \n"
    "    varying vec2 TexCoord;                 \n"
    "                                           \n"
    "    void main() {                          \n"
    "        gl_Position = inProjectionMatrix * inModelViewMatrix * inPosition;\n"
    "        Color       = inColor;             \n"
    "        TexCoord    = inTexCoord;          \n"
    "    }\n";

    const char *fb_fragment_shader_text =
    "    precision mediump float;               \n"
    "                                           \n"
    "    uniform sampler2D tex1;                \n"
    "    uniform float alpha;                   \n"
    "                                           \n"
    "    varying vec4 Color;                    \n"
    "    varying vec2 TexCoord;                 \n"
    "                                           \n"
    "    void main() {                          \n"
    "        vec4 texColor = texture2D(tex1, TexCoord.st);\n"
    "        gl_FragColor = Color * texColor * vec4(1.0, 1.0, 1.0, alpha);\n"
    "    }                                      \n";

    const char *blur_fragment_shader_text =
    "    precision mediump float;               \n"
    "    uniform sampler2D tex1;                \n"
    "    uniform vec2      scale;               \n"
    "    varying vec2      TexCoord;            \n"
    "    uniform float     alpha;               \n"
    "    uniform float     mult;                \n"
    "                                           \n"
    "    void main() {                          \n"
    "       vec4 color = vec4(0,0,0,0);         \n"
    "       color += texture2D(tex1, vec2(TexCoord.x-4.0*scale.x, TexCoord.y-4.0*scale.y))*0.05;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x-3.0*scale.x, TexCoord.y-3.0*scale.y))*0.09;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x-2.0*scale.x, TexCoord.y-2.0*scale.y))*0.12;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x-1.0*scale.x, TexCoord.y-1.0*scale.y))*0.15;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x+0.0*scale.x, TexCoord.y+0.0*scale.y))*0.16;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x+1.0*scale.x, TexCoord.y+1.0*scale.y))*0.15;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x+2.0*scale.x, TexCoord.y+2.0*scale.y))*0.12;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x+3.0*scale.x, TexCoord.y+3.0*scale.y))*0.09;\n"
    "       color += texture2D(tex1, vec2(TexCoord.x+4.0*scale.x, TexCoord.y+4.0*scale.y))*0.05;\n"
    "       gl_FragColor = color * vec4(mult,mult,mult,alpha*mult);\n"
    "    }                                      \n";

    int rc;
    GLuint vertex_shader;
    GLuint fragment_shader;

    self->did_setup = 1;
    
    // Set up the program for the framebuffer
    vertex_shader   = vector_display_load_shader(GL_VERTEX_SHADER,   fb_vertex_shader_text);
    if (vertex_shader   == 0) return -1;
    fragment_shader = vector_display_load_shader(GL_FRAGMENT_SHADER, fb_fragment_shader_text);
    if (fragment_shader == 0) return -1;
    self->fb_program = glCreateProgram();
    if(self->fb_program == 0) return -1;
    glAttachShader(self->fb_program, vertex_shader);
    glAttachShader(self->fb_program, fragment_shader);
    glBindAttribLocation(self->fb_program, VERTEX_COLOR_INDEX, "inColor");
    glBindAttribLocation(self->fb_program, VERTEX_POS_INDEX,   "inPosition");
    glBindAttribLocation(self->fb_program, VERTEX_TEXCOORD_INDEX, "inTexCoord");
    glLinkProgram(self->fb_program);
    rc = vector_display_check_program_link(self->fb_program);
    if (rc < 0) return rc;
    self->fb_uniform_modelview  = glGetUniformLocation(self->fb_program, "inModelViewMatrix");
    self->fb_uniform_projection = glGetUniformLocation(self->fb_program, "inProjectionMatrix");
    self->fb_uniform_alpha      = glGetUniformLocation(self->fb_program, "alpha");

    // Set up the program for the screen
    vertex_shader   = vector_display_load_shader(GL_VERTEX_SHADER,   nocolor_vertex_shader_text);
    if (vertex_shader   == 0) return -1;
    fragment_shader = vector_display_load_shader(GL_FRAGMENT_SHADER, blit_fragment_shader_text);
    if (fragment_shader == 0) return -1;
    self->screen_program = glCreateProgram();
    if(self->screen_program == 0) return -1;
    glAttachShader(self->screen_program, vertex_shader);
    glAttachShader(self->screen_program, fragment_shader);
    glBindAttribLocation(self->screen_program, VERTEX_POS_INDEX,   "inPosition");
    glBindAttribLocation(self->screen_program, VERTEX_TEXCOORD_INDEX, "inTexCoord");
    glLinkProgram(self->screen_program);
    rc = vector_display_check_program_link(self->screen_program);
    if (rc < 0) return rc;
    self->screen_uniform_modelview  = glGetUniformLocation(self->screen_program, "inModelViewMatrix");
    self->screen_uniform_projection = glGetUniformLocation(self->screen_program, "inProjectionMatrix");
    self->screen_uniform_alpha      = glGetUniformLocation(self->screen_program, "alpha");
    self->screen_uniform_mult       = glGetUniformLocation(self->screen_program, "mult");

    // Set up the program for blur
    vertex_shader   = vector_display_load_shader(GL_VERTEX_SHADER,   nocolor_vertex_shader_text);
    if (vertex_shader   == 0) return -1;
    fragment_shader = vector_display_load_shader(GL_FRAGMENT_SHADER, blur_fragment_shader_text);
    if (fragment_shader == 0) return -1;
    self->blur_program = glCreateProgram();
    if(self->blur_program == 0) return -1;
    glAttachShader(self->blur_program, vertex_shader);
    glAttachShader(self->blur_program, fragment_shader);
    glBindAttribLocation(self->blur_program, VERTEX_POS_INDEX,   "inPosition");
    glBindAttribLocation(self->blur_program, VERTEX_TEXCOORD_INDEX, "inTexCoord");
    glLinkProgram(self->blur_program);
    rc = vector_display_check_program_link(self->blur_program);
    if (rc < 0) return rc;
    self->blur_uniform_modelview  = glGetUniformLocation(self->blur_program, "inModelViewMatrix");
    self->blur_uniform_projection = glGetUniformLocation(self->blur_program, "inProjectionMatrix");
    self->blur_uniform_scale      = glGetUniformLocation(self->blur_program, "scale");                         
    self->blur_uniform_alpha      = glGetUniformLocation(self->blur_program, "alpha");                         
    self->blur_uniform_mult       = glGetUniformLocation(self->blur_program, "mult");

    // generate the line texture
    gen_linetex(self);

    // set up the framebuffer for the scene
    glGenFramebuffers(1, &self->fb_scene);                                                                      vector_display_check_error("glGenFramebuffers");
    glGenTextures(1, &self->fb_scene_texid);                                                                    vector_display_check_error("glGenTextures");
    glBindFramebuffer(GL_FRAMEBUFFER, self->fb_scene);                                                          vector_display_check_error("glBindFramebuffer");
    glBindTexture(GL_TEXTURE_2D, self->fb_scene_texid);                                                         vector_display_check_error("glBindTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->width, self->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);     vector_display_check_error("glTexImage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                           vector_display_check_error("glTexParameteri GL_TEXTURE_MIN_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                           vector_display_check_error("glTexParameteri GL_TEXTURE_MAG_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                        vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                        vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_T");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->fb_scene_texid, 0);       vector_display_check_error("glFramebufferTexture2D");
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return -1;

    // set up the glow0 framebuffer
    glGenFramebuffers(1, &self->fb_glow0);                                                                            vector_display_check_error("glGenFramebuffers");
    glGenTextures(1, &self->fb_glow0_texid);                                                                          vector_display_check_error("glGenTextures");
    glBindFramebuffer(GL_FRAMEBUFFER, self->fb_glow0);                                                                vector_display_check_error("glBindFramebuffer");
    glBindTexture(GL_TEXTURE_2D, self->fb_glow0_texid);                                                               vector_display_check_error("glBindTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->glow_width, self->glow_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); vector_display_check_error("glTexImage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                                 vector_display_check_error("glTexParameteri GL_TEXTURE_MIN_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                                 vector_display_check_error("glTexParameteri GL_TEXTURE_MAG_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                              vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                              vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_T");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->fb_glow0_texid, 0);             vector_display_check_error("glFramebufferTexture2D");
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return -1;

    // set up the glow1 framebuffer
    glGenFramebuffers(1, &self->fb_glow1);                                                                            vector_display_check_error("glGenFramebuffers");
    glGenTextures(1, &self->fb_glow1_texid);                                                                          vector_display_check_error("glGenTextures");
    glBindFramebuffer(GL_FRAMEBUFFER, self->fb_glow1);                                                                vector_display_check_error("glBindFramebuffer");
    glBindTexture(GL_TEXTURE_2D, self->fb_glow1_texid);                                                               vector_display_check_error("glBindTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->glow_width, self->glow_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); vector_display_check_error("glTexImage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                                 vector_display_check_error("glTexParameteri GL_TEXTURE_MIN_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                                 vector_display_check_error("glTexParameteri GL_TEXTURE_MAG_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                              vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                              vector_display_check_error("glTexParameteri GL_TEXTURE_WRAP_T");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->fb_glow1_texid, 0);             vector_display_check_error("glFramebufferTexture2D");
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return -1;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glGenBuffers(self->steps, self->buffers);

    // set up vertex buffer for painting the screen
    glGenBuffers(1, &self->screen_vertexbuffer);
    nocolor_point_t screen_points[] = {
    //    x            y             z           u, v
    //   ------------------------------------------------------------
        { 0,           0,            10000,      0, 1 },        // upper left triangle
        { self->width+1, self->height+1, 10000,      1, 0 },
        { self->width+1, 0,            10000,      1, 1 },

        { 0,           0,            10000,      0, 1 },        // lower right triangle
        { 0,           self->height+1, 10000,      0, 0 },
        { self->width+1, self->height+1, 10000,      1, 0 },
    };

    // load up vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, self->screen_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_points), screen_points, GL_STATIC_DRAW);

    // set up vertex buffer for painting the screen
    glGenBuffers(1, &self->glow_vertexbuffer);
    nocolor_point_t glow_points[] = {
    //    x                 y                  z           u, v
    //   ------------------------------------------------------------
        { 0,                0,                 10000,      0, 1 },        // upper left triangle
        { self->glow_width, self->glow_height, 10000,      1, 0 },
        { self->glow_width, 0,                 10000,      1, 1 },

        { 0,                0,                 10000,      0, 1 },        // lower right triangle
        { 0,                self->glow_height, 10000,      0, 0 },
        { self->glow_width, self->glow_height, 10000,      1, 0 },
    };

    // load up vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, self->glow_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glow_points), glow_points, GL_STATIC_DRAW);

    return 0;
}

int vector_display_update(vector_display_t *self) {
    GLfloat glow_projmat[] = {
        2.0f/self->glow_width, 0, 0, 0,
        0, -2.0f/self->glow_height, 0, 0,
        0, 0, -2.0f/70001.0f, 0,
        -1.0f,1.0f,-1.0f,1.0f
    };

    GLfloat projmat[] = {
        2.0f/self->width, 0, 0, 0,
        0, -2.0f/self->height, 0, 0,
        0, 0, -2.0f/70001.0f, 0,
        -1.0f,1.0f,-1.0f,1.0f
    };

    GLfloat mvmat[] = {
        1.0f,0,0,0,  
        0,1.0f,0,0,  
        0,0,1.0f,0,  
        0,0,-70000.0f,1.0f 
    };

    // save the framebuffer for the final draw
    GLuint drawbuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&drawbuffer); 

    // bind the framebuffer used for rendering the scene
    glBindFramebuffer(GL_FRAMEBUFFER, self->fb_scene);                                                                
    glViewport(0, 0, self->width, self->height);

    // set up opengl options
    glEnable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX_EXT);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    // clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // setup shaders
    glUseProgram(self->fb_program);
    glUniformMatrix4fv(self->fb_uniform_projection, 1, GL_FALSE, projmat);
    glUniformMatrix4fv(self->fb_uniform_modelview, 1, GL_FALSE, mvmat);
    glUniform1f(self->fb_uniform_alpha, 1.0f);

    // bind the line texture
    glBindTexture(GL_TEXTURE_2D, self->linetexid);

    // advance step
    self->step = (self->step + 1) % self->steps;

    // populate vertex buffer for the current step from the vector data
    glBindBuffer(GL_ARRAY_BUFFER, self->buffers[self->step]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point_t) * self->npoints, self->points, GL_STATIC_DRAW);
    self->buffernpoints[self->step] = self->npoints;

    // draw
    int loopvar;
    for (loopvar = 0; loopvar < self->steps; loopvar++) {
        int stepi = self->steps - loopvar - 1;
        //int stepi = loopvar;
        int i = (self->step + self->steps - stepi) % self->steps;
        //debugf("render buffer %d/%d i = %d", stepi, self->steps, i);

        if (self->buffernpoints[i] == 0) {
            //debugf("skip buffer %d", stepi);
        } else {
            float alpha;
            if (stepi == 0) {
                alpha = 1.0f;
            } else if (stepi == 1) {
                alpha = self->initial_decay;
            } else {
                alpha = pow(self->decay, stepi-1) * self->initial_decay;
            }

            glUniform1f(self->fb_uniform_alpha, alpha); 
            glBindBuffer(GL_ARRAY_BUFFER, self->buffers[i]);
            glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_TRUE,  sizeof(point_t), 0);
            glVertexAttribPointer(VERTEX_COLOR_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(point_t), (void*)(3 * sizeof(float)));
            glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_TRUE, sizeof(point_t), (void*)(7 * sizeof(float)));
            glEnableVertexAttribArray(VERTEX_POS_INDEX);
            glEnableVertexAttribArray(VERTEX_COLOR_INDEX);
            glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);
            glDrawArrays(GL_TRIANGLES, 0, self->buffernpoints[i]);
        }
    }

    // 
    // setup for glow post-processing
    //
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(self->blur_program);
    glUniformMatrix4fv(self->blur_uniform_projection, 1, GL_FALSE, glow_projmat);
    glUniformMatrix4fv(self->blur_uniform_modelview, 1, GL_FALSE, mvmat);

    glBindBuffer(GL_ARRAY_BUFFER, self->glow_vertexbuffer);
    glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_TRUE,  sizeof(nocolor_point_t), 0);
    glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_TRUE, sizeof(nocolor_point_t), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(VERTEX_POS_INDEX);
    glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);
    glUniform1f(self->blur_uniform_alpha, 1.0); 
    glUniform1f(self->blur_uniform_mult, 1.05); 

    glViewport(0, 0, self->glow_width, self->glow_height);

    glBindTexture(GL_TEXTURE_2D, self->fb_scene_texid);

    for (int j = 0; j < 3; j++) {
        // render the glow1 texture to the glow0 buffer with horizontal blur
        glBindFramebuffer(GL_FRAMEBUFFER, self->fb_glow0);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2f(self->blur_uniform_scale, 1.0/self->glow_width, 0.0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, self->fb_glow0_texid);
        
        // render the glow0 texture to the glow1 buffer with vertical blur
        glBindFramebuffer(GL_FRAMEBUFFER, self->fb_glow1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2f(self->blur_uniform_scale, 0, 1.0/self->glow_height);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, self->fb_glow1_texid);
    }

    // set up the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, self->glow_vertexbuffer);
    glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_TRUE,  sizeof(nocolor_point_t), 0);
    glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_TRUE, sizeof(nocolor_point_t), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(VERTEX_POS_INDEX);
    glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);

    //
    // render scene + glow1 to the screen
    //
    glBindFramebuffer(GL_FRAMEBUFFER, drawbuffer);
    glViewport(0, 0, self->width, self->height);

    // clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // setup shaders
    glUseProgram(self->screen_program);
    glUniformMatrix4fv(self->screen_uniform_projection, 1, GL_FALSE, projmat);
    glUniformMatrix4fv(self->screen_uniform_modelview, 1, GL_FALSE, mvmat);

    // set up the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, self->screen_vertexbuffer);
    glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_TRUE,  sizeof(nocolor_point_t), 0);
    glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_TRUE, sizeof(nocolor_point_t), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(VERTEX_POS_INDEX);
    glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);

    // set up blending for the final compositing
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    // paint the scene
    glUniform1f(self->screen_uniform_alpha, 1.0); 
    glUniform1f(self->screen_uniform_mult, 1.0); 
    glBindTexture(GL_TEXTURE_2D, self->fb_scene_texid);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // blend in the glow
    glUniform1f(self->screen_uniform_mult, 1.5); 
    glBindTexture(GL_TEXTURE_2D, self->fb_glow1_texid);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    return 0;
}

int vector_display_teardown(vector_display_t *self) {
    glDeleteProgram(self->fb_program);
    glDeleteProgram(self->screen_program);
    glDeleteFramebuffers(1, &self->fb_scene);
    return -1;
}

void vector_display_delete(vector_display_t *self) {
    free(self);
}
