#include "vector_display_utils.h"
#include "vector_display.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

vector_display_log_cb_t vector_display_log_cb = NULL;

void vector_display_debugf(const char *fmt, ...) {
    vector_display_log_cb_t log_cb = vector_display_log_cb;

    if (log_cb) {
        char buf[8192];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        log_cb(buf);
        va_end(ap);
    } else {
        // log to stderr
        va_list ap;
        fprintf(stderr, "[vector_display] ");
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

// NOTE: returns 0 on failure
GLuint vector_display_load_shader(GLenum type, const char *shaderSrc) {
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
            vector_display_debugf("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void vector_display_check_error(const char *desc) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        vector_display_debugf("opengl error in %s: %d", desc, (int)err);
    }
}

int vector_display_check_program_link(GLuint program) {
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1) {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            vector_display_debugf("Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return -1;
    }
    return 0;
}

// -15 stored using a single precision bias of 127
const unsigned int  HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
// max exponent value in single precision that will be converted
// to Inf or Nan when stored as a half-float
const unsigned int  HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;

// 255 is the max exponent biased value
const unsigned int  FLOAT_MAX_BIASED_EXP = (0xFF << 23);

const unsigned int  HALF_FLOAT_MAX_BIASED_EXP = (0x1F << 10);

hfloat float_to_hfloat(float f) {
    unsigned int    x = *(unsigned int *)&f;
    unsigned int    sign = (unsigned short)(x >> 31);
    unsigned int    mantissa;
    unsigned int    exp;
    hfloat          hf;

    // get mantissa
    mantissa = x & ((1 << 23) - 1);
    // get exponent bits
    exp = x & FLOAT_MAX_BIASED_EXP;
    if (exp >= HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP) {
        // check if the original single precision float number is a NaN
        if (mantissa && (exp == FLOAT_MAX_BIASED_EXP)) {
            // we have a single precision NaN
            mantissa = (1 << 23) - 1;
        } else {
            // 16-bit half-float representation stores number as Inf
            mantissa = 0;
        }
        hf = (((hfloat)sign) << 15) | (hfloat)(HALF_FLOAT_MAX_BIASED_EXP) |
              (hfloat)(mantissa >> 13);
    } else if (exp <= HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) { // check if exponent is <= -15
        // store a denorm half-float value or zero
        exp = (HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
        mantissa >>= (14 + exp);

        hf = (((hfloat)sign) << 15) | (hfloat)(mantissa);
    } else {
        hf = (((hfloat)sign) << 15) |
              (hfloat)((exp - HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 13) |
              (hfloat)(mantissa >> 13);
    }

    return hf;
}

float hfloat_to_float(hfloat hf) {
    unsigned int    sign = (unsigned int)(hf >> 15);
    unsigned int    mantissa = (unsigned int)(hf & ((1 << 10) - 1));
    unsigned int    exp = (unsigned int)(hf & HALF_FLOAT_MAX_BIASED_EXP);
    unsigned int    f;

    if (exp == HALF_FLOAT_MAX_BIASED_EXP) {
        // we have a half-float NaN or Inf
        // half-float NaNs will be converted to a single precision NaN
        // half-float Infs will be converted to a single precision Inf
        exp = FLOAT_MAX_BIASED_EXP;
        if (mantissa)
            mantissa = (1 << 23) - 1;    // set all bits to indicate a NaN
    } else if (exp == 0x0) {
        // convert half-float zero/denorm to single precision value
        if (mantissa) {
           mantissa <<= 1;
           exp = HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
           // check for leading 1 in denorm mantissa
           while ((mantissa & (1 << 10)) == 0) {
               // for every leading 0, decrement single precision exponent by 1
               // and shift half-float mantissa value to the left
               mantissa <<= 1;
               exp -= (1 << 23);
            }
            // clamp the mantissa to 10-bits
            mantissa &= ((1 << 10) - 1);
            // shift left to generate single-precision mantissa of 23-bits
            mantissa <<= 13;
        }
    } else {
        // shift left to generate single-precision mantissa of 23-bits
        mantissa <<= 13;
        // generate single precision biased exponent value
        exp = (exp << 13) + HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
    }

    f = (sign << 31) | exp | mantissa;
    return *((float *)&f);
}
