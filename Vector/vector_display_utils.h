//
//  vector_display_utils.h
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#ifndef Vector_vector_display_utils_h
#define Vector_vector_display_utils_h

#include "vector_display_glinc.h"

#ifdef __cplusplus
extern "C" {
#endif

GLuint vector_display_load_shader(GLenum type, const char *shaderSrc);
void vector_display_check_error(const char *desc);
int vector_display_check_program_link(GLuint program);

void vector_display_debugf(const char *fmt, ...);

typedef unsigned short hfloat;
hfloat float_to_hfloat(float f);
float hfloat_to_float(hfloat hf);

#ifdef __cplusplus
}
#endif

#endif

