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
#include <math.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define MAX_STEPS 300
#define DEFAULT_STEPS 4 
#define DEFAULT_DECAY 0.5f
#define DEFAULT_INITIAL_DECAY 0.2f
#define DEFAULT_THICKNESS 100.0f
#define TEXTURE_SIZE 128
#define HALF_TEXTURE_SIZE (TEXTURE_SIZE/2)

typedef struct {
    float x, y, z;
    float r, g, b, a;
    float u, v;
} point_t;

struct vector_display {
    GLuint program;
    GLuint uniform_modelview;
    GLuint uniform_projection;
    GLuint uniform_alpha;
    GLuint uniform_tex;

    float width, height;

    int steps;
    float decay;
    float r, g, b;

    int nvectors;

    size_t cpoints;
    size_t npoints;
    point_t *points;

    int step;
    GLuint *buffers;
    GLuint *buffernpoints;

    GLuint texid;

    int did_setup;

    float initial_decay;
    float thickness;
};

#define VERTEX_POS_INDEX       (0)
#define VERTEX_COLOR_INDEX     (1)
#define VERTEX_TEXCOORD_INDEX  (2)

static GLuint load_shader(GLenum type, const char *shaderSrc);

int vector_display_new(vector_display_t **out_self, float width, float height) {
    vector_display_t *self = (vector_display_t*)calloc(sizeof(vector_display_t), 1);
    if (self == NULL) return -1;
    self->steps = DEFAULT_STEPS;
    self->buffers = (GLuint*)calloc(sizeof(GLuint), self->steps);
    self->buffernpoints = (GLuint*)calloc(sizeof(GLuint), self->steps);
    self->cpoints = 60;
    self->points = (point_t*)calloc(sizeof(point_t), self->cpoints);
    self->decay = DEFAULT_DECAY;
    self->r = self->g = self->b = 1.0f;
    self->width = width;
    self->height = height;
    self->initial_decay = DEFAULT_INITIAL_DECAY;
    self->thickness = DEFAULT_THICKNESS;
    *out_self = self;
    return 0;
}

int vector_display_set_initial_decay(vector_display_t *self, float initial_decay) {
    if (initial_decay < 0.0f || initial_decay >= 1.0f) return -1;
    self->initial_decay = initial_decay;
    return 0;
}

int vector_display_set_thickness(vector_display_t *self, float thickness) {
    if (thickness <= 0) return -1;
    self->thickness = thickness;
}

int vector_display_clear(vector_display_t *self) {
    self->npoints = 0;
    return 0;
}

int vector_display_set_color(vector_display_t *self, float r, float g, float b) {
    self->r = r;
    self->g = g;
    self->b = b;
    return 0;
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

static void append_point(vector_display_t *self, float x, float y, float a) {
    ensure_points(self, self->npoints + 1);
    self->points[self->npoints].x = x;
    self->points[self->npoints].y = y;
    self->points[self->npoints].z = 10000.0f;
    self->points[self->npoints].r = self->r;
    self->points[self->npoints].g = self->g;
    self->points[self->npoints].a = a;
    self->points[self->npoints].b = self->b;
    self->points[self->npoints].u = 0.5;
    self->points[self->npoints].v = 0.5;
    self->npoints++;
}

static void append_texpoint(vector_display_t *self, float x, float y, float u, float v) {
    ensure_points(self, self->npoints + 1);
    self->points[self->npoints].x = x;
    self->points[self->npoints].y = y;
    self->points[self->npoints].z = 10000.0f;
    self->points[self->npoints].r = self->r;
    self->points[self->npoints].g = self->g;
    self->points[self->npoints].b = self->b;
    self->points[self->npoints].a = 1.0f;

    self->points[self->npoints].u = u / TEXTURE_SIZE;
    self->points[self->npoints].v = 1.0f - (v / TEXTURE_SIZE);
    
    //self->points[self->npoints].u = u;
    //self->points[self->npoints].v = v;

    //self->points[self->npoints].u = 1.0f;
    //self->points[self->npoints].v = 1.0f;
    
    self->npoints++;
}

int vector_display_draw(vector_display_t *self, float x0, float y0, float x1, float y1) {
    float t = self->thickness;
    float a = atan2(y1 - y0, x1 - x0);

    float xl0 = x0 + t * sin(a);
    float yl0 = y0 - t * cos(a);

    float xr0 = x0 - t * sin(a);
    float yr0 = y0 + t * cos(a);

    float xlt0 = xl0 - t * cos(a);
    float ylt0 = yl0 - t * sin(a);

    float xrt0 = xr0 - t * cos(a);
    float yrt0 = yr0 - t * sin(a);

    float xl1 = x1 + t * sin(a);
    float yl1 = y1 - t * cos(a);

    float xr1 = x1 - t * sin(a);
    float yr1 = y1 + t * cos(a);

    float xlt1 = xl1 + t * cos(a);
    float ylt1 = yl1 + t * sin(a);

    float xrt1 = xr1 + t * cos(a);
    float yrt1 = yr1 + t * sin(a);

    // left side
    append_texpoint(self, x1, y1, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, x0, y0, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xl1, yl1, 0.0f, HALF_TEXTURE_SIZE);

    append_texpoint(self, xl0, yl0, 0.0f, HALF_TEXTURE_SIZE);
    append_texpoint(self, x0, y0, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xl1, yl1, 0.0f, HALF_TEXTURE_SIZE);

    // right side
    append_texpoint(self, x0, y0, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, x1, y1, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);

    append_texpoint(self, x0, y0, HALF_TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xr0, yr0, TEXTURE_SIZE, HALF_TEXTURE_SIZE);

    // 0 endcap
    append_texpoint(self, xlt0, ylt0, 0.0f, 0.0f);    
    append_texpoint(self, xl0, yl0, 0.0f, HALF_TEXTURE_SIZE);
    append_texpoint(self, xr0, yr0, TEXTURE_SIZE, HALF_TEXTURE_SIZE);

    append_texpoint(self, xlt0, ylt0, 0.0f, 0.0f);
    append_texpoint(self, xr0, yr0, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xrt0, yrt0, TEXTURE_SIZE, 0.0f);   
    
    
    // 1 endcap
    append_texpoint(self, xlt1, ylt1, 0.0f, 0.0f);    
    append_texpoint(self, xl1, yl1, 0.0f, HALF_TEXTURE_SIZE);
    append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);

    append_texpoint(self, xlt1, ylt1, 0.0f, 0.0f);
    append_texpoint(self, xr1, yr1, TEXTURE_SIZE, HALF_TEXTURE_SIZE);
    append_texpoint(self, xrt1, yrt1, TEXTURE_SIZE, 0.0f);   

    /*
    // tip 0
    append_texpoint(self,xt0,yt0,0.0f);
    append_texpoint(self,xr0,yr0,0.0f);
    append_texpoint(self,x0,y0,1.0f);

    append_texpoint(self,xl0,yl0,0.0f);
    append_texpoint(self,xt0,yt0,0.0f);
    append_texpoint(self,x0,y0,1.0f);
    
    // tip 1
    append_texpoint(self,xt1,yt1,0.0f);
    append_texpoint(self,xr1,yr1,0.0f);
    append_texpoint(self,x1,y1,1.0f);

    append_texpoint(self,xt1,yt1,0.0f);
    append_texpoint(self,x1,y1,1.0f);
    append_texpoint(self,xl1,yl1,0.0f);
    */

#if 0
    float t = self->thickness;
    float a = atan2(y1 - y0, x1 - x0);

    float xl0 = x0 + t * sin(a);
    float yl0 = y0 - t * cos(a);

    float xr0 = x0 - t * sin(a);
    float yr0 = y0 + t * cos(a);

    float xt0 = x0 - t * cos(a);
    float yt0 = y0 - t * sin(a);

    float xl1 = x1 + t * sin(a);
    float yl1 = y1 - t * cos(a);

    float xr1 = x1 - t * sin(a);
    float yr1 = y1 + t * cos(a);

    float xt1 = x1 + t * cos(a);
    float yt1 = y1 + t * sin(a);

    // left side
    append_point(self,x1,y1,1.0f);
    append_point(self,x0,y0,1.0f);
    append_point(self,xl1,yl1,0.0f);

    append_point(self,xl0,yl0,0.0f);
    append_point(self,x0,y0,1.0f);
    append_point(self,xl1,yl1,0.0f);

    // right side
    append_point(self,x0,y0,1.0f);
    append_point(self,x1,y1,1.0f);
    append_point(self,xr1,yr1,0.0f);

    append_point(self,x0,y0,1.0f);
    append_point(self,xr1,yr1,0.0f);
    append_point(self,xr0,yr0,0.0f);

    // tip 0
    append_point(self,xt0,yt0,0.0f);
    append_point(self,xr0,yr0,0.0f);
    append_point(self,x0,y0,1.0f);

    append_point(self,xl0,yl0,0.0f);
    append_point(self,xt0,yt0,0.0f);
    append_point(self,x0,y0,1.0f);
    
    // tip 1
    append_point(self,xt1,yt1,0.0f);
    append_point(self,xr1,yr1,0.0f);
    append_point(self,x1,y1,1.0f);

    append_point(self,xt1,yt1,0.0f);
    append_point(self,x1,y1,1.0f);
    append_point(self,xl1,yl1,0.0f);
#endif

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

int vector_display_set_decay(vector_display_t *self, float decay) {
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

#if 0
            double shelf = 4.0/256;
            double mult  = pow(10, -10 * distance) + shelf;

            if (distance > 0.4 && shelf > 0) {
                mult = cos((x - 0.4) * 2 * M_PI) * shelf/2 + shelf/2;
            }
#else
            double mult  = pow(6, -12 * distance);
#endif
 
            int val = (int)round(mult * 256);

            if (distance < 0.01) val = 0xff;
            //if (distance > 0.5 && distance < 1.0) val = 0xff;

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
    glDisable(GL_DITHER);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

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
            glVertexAttribPointer(VERTEX_COLOR_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(point_t), 3 * sizeof(float));
            glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_TRUE, sizeof(point_t), 7 * sizeof(float));
            glEnableVertexAttribArray(VERTEX_POS_INDEX);
            glEnableVertexAttribArray(VERTEX_COLOR_INDEX);
            glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);
            glDrawArrays(GL_TRIANGLES, 0, self->buffernpoints[i]);
        }
    }

#if 0
        /* DRAW A TEST TRIANGLE */
        // vertex/color data
        GLfloat vVertices[] = { 
            100.0f, 100.0f, 10000.0f,
            100.0f, 200.0f, 10000.0f,
            200.0f, 200.0f, 10000.0f,
        };
        GLfloat vColors[] = { 1.0f, 0.0f, 0.0f,
                              0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 1.0f };
            
        glVertexAttribPointer(VERTEX_POS_INDEX,   3, GL_FLOAT, GL_FALSE, 0, vVertices);
        glVertexAttribPointer(VERTEX_COLOR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, vColors);

        glEnableVertexAttribArray(VERTEX_POS_INDEX);
        glEnableVertexAttribArray(VERTEX_COLOR_INDEX);

        // draw
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
    
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0)
        return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        
        if(infoLen > 1)
        {
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

