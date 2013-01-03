//
//  vector_display.c
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#include "vector_display_platform.h"
#include "vector_display.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define MAX_STEPS 300
#define DEFAULT_STEPS 10
#define DEFAULT_DECAY 0.8
#define DEFAULT_INITIAL_DECAY 0.04
#define DEFAULT_THICKNESS 60.0f
#define TEXTURE_SIZE 128
#define HALF_TEXTURE_SIZE (TEXTURE_SIZE/2)

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

typedef struct {
    float x, y, z;
    float r, g, b, a;
    float u, v;
} point_t;

typedef struct {
    double x, y;
} pending_point_t;

struct vector_display {
    GLuint program;
    GLuint uniform_modelview;
    GLuint uniform_projection;
    GLuint uniform_alpha;
    GLuint uniform_tex;

    double width, height;

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

    GLuint texid;

    int did_setup;

    double initial_decay;
    double thickness;
};

#define VERTEX_POS_INDEX       (0)
#define VERTEX_COLOR_INDEX     (1)
#define VERTEX_TEXCOORD_INDEX  (2)

static GLuint load_shader(GLenum type, const char *shaderSrc);

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

static double normalize(double a) {
    while (a > 2*M_PI + FLT_EPSILON) a -= 2*M_PI;
    while (a < 0 - FLT_EPSILON)      a += 2*M_PI;
    return a;
}

void draw_fan(vector_display_t *self, double cx, double cy, double pa, double a, double fr, double cr) {
    double *angles;
    int     nsteps;
    double  pa2a        = normalize(a - pa);
    double  a2pa        = normalize(pa - a);

    int i;
    if (a2pa < pa2a) {
        nsteps = max(1, round(a2pa / (M_PI / 8)));
        angles = alloca(sizeof(double) * (nsteps + 1));
        for (i = 0; i <= nsteps; i++)
            angles[i] = a + i * a2pa / nsteps;
        //debugf("%fd in %d steps", a2pa, nsteps);
    } else {
        nsteps = max(1, round(pa2a / (M_PI / 8)));
        angles = alloca(sizeof(double) * (nsteps + 1));
        for (i = 0; i <= nsteps; i++)
            angles[i] = pa + i * pa2a / nsteps;
        //debugf("%fd in %d steps", pa2a, nsteps);
    }
    //debugf("---- %f -> %f nsteps=%d", 360*pa/M_PI/2, 360*a/M_PI/2, 360*pa2a/M_PI/2, 360*a2pa/M_PI/2, nsteps);

    for (i = 1; i <= nsteps; i++) {
        append_texpoint(self, cx + fr * sin(angles[i-1]), cy - fr * cos(angles[i-1]), 0.0f, HALF_TEXTURE_SIZE);
        append_texpoint(self, cx, cy,                                                 HALF_TEXTURE_SIZE + (cr / self->thickness * HALF_TEXTURE_SIZE), HALF_TEXTURE_SIZE);
        append_texpoint(self, cx + fr * sin(angles[i]),   cy - fr * cos(angles[i]),   0.0f, HALF_TEXTURE_SIZE);
    }
}

int vector_display_end_draw(vector_display_t *self) {
    if (self->pending_npoints < 2) { 
        self->pending_npoints = 0;
        return 0;
    }

    int i;
    double t = self->thickness;

    int   first_last_same = abs(self->pending_points[0].x - self->pending_points[self->pending_npoints-1].x) < 0.1 &&
                            abs(self->pending_points[0].y - self->pending_points[self->pending_npoints-1].y) < 0.1;

    for (i = 1; i < self->pending_npoints; i++) {
        int   is_first = i == 1;
        int   is_last  = i == self->pending_npoints - 1;

        // figure out what connections we have
        int has_prev_connect = !is_first || (is_first && first_last_same);
        int has_next_connect = !is_last  || (is_last  && first_last_same);

        // precomputed info for prev line
        int   pi     = i == 1 ? self->pending_npoints - 2 : i - 2;
        double px0    = self->pending_points[pi].x;       // previous x0
        double py0    = self->pending_points[pi].y;       // previous y0
        double px1    = self->pending_points[i-1].x;      // previous x1
        double py1    = self->pending_points[i-1].y;      // previous y1
        double pa     = atan2(py1 - py0, px1 - px0);      // angle from positive x axis, increasing ccw, [-pi, pi]
        if (pa < 0) pa += 2 * M_PI;

        // precomputed info for current line
        double x0    = self->pending_points[i-1].x;       // my x0
        double y0    = self->pending_points[i-1].y;       // my y0
        double x1    = self->pending_points[i].x;         // my x1
        double y1    = self->pending_points[i].y;         // my y1
        double a     = atan2(y1 - y0, x1 - x0);           // angle from positive x axis, increasing ccw, [-pi, pi]
        double sin_a = sin(a), cos_a = cos(a);

        // precomputed info for next line
        int   ni     = i + 1 != self->pending_npoints ? i + 1 : 1;
        double nx0    = self->pending_points[i].x;        // next x0
        double ny0    = self->pending_points[i].y;        // next y0
        double nx1    = self->pending_points[ni].x;       // next x1
        double ny1    = self->pending_points[ni].y;       // next y1
        double na     = atan2(ny1 - ny0, nx1 - nx0);      // angle from positive x axis, increasing ccw, [-pi, pi]

        // location of the line in render space (note: these values are adjusted below to allow for connectors)
        double xl0 = x0 + t * sin_a, yl0 = y0 - t * cos_a;
        double xr0 = x0 - t * sin_a, yr0 = y0 + t * cos_a;
        double xl1 = x1 + t * sin_a, yl1 = y1 - t * cos_a;
        double xr1 = x1 - t * sin_a, yr1 = y1 + t * cos_a;

        double len  = sqrt(pow(x1-x0, 2) + pow(y1-y0, 2));
        double plen = sqrt(pow(px1-px0, 2) + pow(py1-py0, 2));
        double nlen = sqrt(pow(nx1-nx0, 2) + pow(ny1-ny0, 2));

        double cr   = min(6.0, len  / 2);
        double pcr  = min(cr, plen / 2);
        double ncr  = min(cr, nlen / 2);

        if (has_next_connect) {
            double cr1     = min(cr, ncr);
            double ad      = normalize(na - a);
            double shorten = (cr1 * sin(ad/2)) / cos(ad/2);

            if (ad > M_PI) shorten = -shorten;

            xl1 = xl1 - shorten * cos_a; yl1 = yl1 - shorten * sin_a;
            xr1 = xr1 - shorten * cos_a; yr1 = yr1 - shorten * sin_a;
            x1  = x1  - shorten * cos_a; y1  = y1  - shorten * sin_a;
        }

        if (has_prev_connect) {
            // shorten start of line to compensate for the size of the connector triangles
            double cr0     = min(cr, pcr);
            double ad      = normalize(a - pa);
            double shorten = (cr0 * sin(ad/2)) / cos(ad/2);
            double fr      = t + cr0;    // fan radius

            if (ad > M_PI) shorten = -shorten;

            // shorten line
            xl0 = xl0 + shorten * cos_a; yl0 = yl0 + shorten * sin_a;
            xr0 = xr0 + shorten * cos_a; yr0 = yr0 + shorten * sin_a;
            x0  = x0  + shorten * cos_a; y0  = y0  + shorten * sin_a;

            if (ad > M_PI) {            // concave
                double cxl = x0 + cr0 * sin_a, cyl = y0 - cr0 * cos_a;
                draw_fan(self, cxl, cyl, pa, a, -fr, cr0+1.0); 
            } else {                    // convex
                double cxr = x0 - cr0 * sin_a, cyr = y0 + cr0 * cos_a;
                draw_fan(self, cxr, cyr, pa, a, fr, cr0);
            }
        }

        // left side
        append_texpoint(self, x0,  y0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, x1,  y1,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, xl1, yl1, 0.0f,              HALF_TEXTURE_SIZE);
        append_texpoint(self, x0,  y0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, xl0, yl0, 0.0f,              HALF_TEXTURE_SIZE);
        append_texpoint(self, xl1, yl1, 0.0f,              HALF_TEXTURE_SIZE);

        // right side
        append_texpoint(self, x1,  y1,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, x0,  y0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, xr1, yr1, TEXTURE_SIZE,      HALF_TEXTURE_SIZE);
        append_texpoint(self, xr1, yr1, TEXTURE_SIZE,      HALF_TEXTURE_SIZE);
        append_texpoint(self, x0,  y0,  HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
        append_texpoint(self, xr0, yr0, TEXTURE_SIZE,      HALF_TEXTURE_SIZE);

        if (!has_next_connect) {
            // draw endcap
            double xlt1 = xl1 + t * cos_a;
            double ylt1 = yl1 + t * sin_a;
            double xrt1 = xr1 + t * cos_a;
            double yrt1 = yr1 + t * sin_a;
            append_texpoint(self, xl1, yl1, 0.0f, HALF_TEXTURE_SIZE);
            append_texpoint(self, xlt1, ylt1, 0.0f, 0.0f);    
            append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, xlt1, ylt1, 0.0f, 0.0f);
            append_texpoint(self, xrt1, yrt1, TEXTURE_SIZE, 0.0f);
        }

        if (!has_prev_connect) {
            // draw startcap
            double xlt0 = xl0 - t * cos_a;
            double ylt0 = yl0 - t * sin_a;
            double xrt0 = xr0 - t * cos_a;
            double yrt0 = yr0 - t * sin_a;
            append_texpoint(self, xl0, yl0, 0.0f, HALF_TEXTURE_SIZE);
            append_texpoint(self, xlt0, ylt0, 0.0f, 0.0f);    
            append_texpoint(self, xr0, yr0, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, xr0, yr0, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
            append_texpoint(self, xlt0, ylt0, 0.0f, 0.0f);
            append_texpoint(self, xrt0, yrt0, TEXTURE_SIZE, 0.0f);   
        }
    }

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

static void check_error(const char *desc) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        debugf("opengl error in %s: %d", desc, (int)err);
    }
}

int vector_display_setup(vector_display_t *self) {    
    const char *vShaderStr =
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
    "    void main()                            \n"
    "    {                                      \n"
    "        gl_Position = inProjectionMatrix * inModelViewMatrix * inPosition;\n"
    "        Color       = inColor;             \n"
    "        TexCoord    = inTexCoord;          \n"
    "    }\n";

    const char *fShaderStr =
    "    precision mediump float;               \n"
    "                                           \n"
    "    uniform sampler2D tex1;                \n"
    "    uniform float alpha;                   \n"
    "                                           \n"
    "    varying vec4 Color;                    \n"
    "    varying vec2 TexCoord;                 \n"
    "                                           \n"
    "    void main() {                          \n"
    "        gl_FragColor = Color * texture2D(tex1, TexCoord.st) * vec4(1.0, 1.0, 1.0, alpha);\n"
    "    }                                      \n";
    
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;
    GLint linked;

    self->did_setup = 1;
    
    // Load the vertex/fragment shaders
    vertexShader   = load_shader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = load_shader(GL_FRAGMENT_SHADER, fShaderStr);
    
    if (vertexShader == 0) return -1;
    if (fragmentShader == 0) return -1;
    
    // Create the program object
    program = glCreateProgram();
    if(program == 0)
        return -1;
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    
    glBindAttribLocation(program, VERTEX_COLOR_INDEX, "inColor");
    glBindAttribLocation(program, VERTEX_POS_INDEX,   "inPosition");
    glBindAttribLocation(program, VERTEX_TEXCOORD_INDEX, "inTexCoord");
    
    // Link the program
    glLinkProgram(program);
    // Check the link status
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        
        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            debugf("Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return -1;
    }

    // Store the program object
    self->program = program;

    // generate the texture
    unsigned char texbuf[TEXTURE_SIZE * TEXTURE_SIZE * 4];
    memset(texbuf, 0xff, sizeof(texbuf));
    int x,y;
    for (x = 0; x < TEXTURE_SIZE; x++) {
        for (y = 0; y < TEXTURE_SIZE; y++) {
            double distance = sqrt(pow(x-HALF_TEXTURE_SIZE, 2) + pow(y-HALF_TEXTURE_SIZE, 2)) / (double)HALF_TEXTURE_SIZE;

            if (distance > 1.0) distance = 1.0;

            double line = pow(12, -20 * distance) * 246.0/256.0;
            double glow = pow(2,   -4 * distance) *  6.0/256.0;
            double mult = line + glow;

            int val = (int)round(mult * 256);

            if (distance < 0.01) val = 0xff;

            texbuf[(x + y * TEXTURE_SIZE) * 4 + 3] = (unsigned char)val;
        }
    }

    // load and bind the texture

    glGenTextures(1, &self->texid);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, self->texid);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texbuf);
    check_error("glTexImage2D");

    // get uniform locations
    self->uniform_modelview  = glGetUniformLocation(self->program, "inModelViewMatrix");
    self->uniform_projection = glGetUniformLocation(self->program, "inProjectionMatrix");
    self->uniform_alpha      = glGetUniformLocation(self->program, "alpha");
    self->uniform_tex        = glGetUniformLocation(self->program, "tex");
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glGenBuffers(self->steps, self->buffers);
    
    return 0;
}

int vector_display_update(vector_display_t *self) {
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

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_DITHER);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // setup shaders
    glUseProgram(self->program);
    glUniformMatrix4fv(self->uniform_projection, 1, GL_FALSE, projmat);
    glUniformMatrix4fv(self->uniform_modelview, 1, GL_FALSE, mvmat);
    glUniform1f(self->uniform_alpha, 1.0f);

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

            glUniform1f(self->uniform_alpha, alpha); 
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

#if 0
        /* DRAW A TEST TRIANGLE */
        GLfloat vVertices[] = { 100.0f, 100.0f, 10000.0f, 100.0f, 200.0f, 10000.0f, 200.0f, 200.0f, 10000.0f };
        GLfloat vColors[] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_FALSE, 0, vVertices);
        glVertexAttribPointer(VERTEX_COLOR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, vColors);
        glEnableVertexAttribArray(VERTEX_POS_INDEX);
        glEnableVertexAttribArray(VERTEX_COLOR_INDEX);
        glDrawArrays(GL_TRIANGLES, 0, 3);
#endif

    return 0;
}

int vector_display_teardown(vector_display_t *self) {
    glDeleteProgram(self->program);
    return -1;
}

void vector_display_delete(vector_display_t *self) {
    free(self);
}

// NOTE: returns 0 on failure
static GLuint load_shader(GLenum type, const char *shaderSrc) {
    GLuint shader;
    GLint compiled;
    
    shader = glCreateShader(type);
    if(shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if(!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1) {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            debugf("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

