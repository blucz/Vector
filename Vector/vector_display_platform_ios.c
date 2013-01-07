//
//  vector_display_platform_ios.c
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#include "vector_display_platform.h"

#include <stdarg.h>
#include <stdio.h>

void debugf(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "[vector_display] ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void failf(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "[vector_display] ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    exit(1);
}
