Vector
======

Vector emulates a vector display, like those found in Asteroids and Tempest 
arcade games. It is implemented in OpenGL ES 2.0.

It targets the iPad. For best results, use an iPad 4. Vector runs poorly on the 
simulator and I haven't tested anything slower than an iPad 4.

In its current state, it's a sample program that demoes the most basic of vector 
display functionality.

The following files:

    vector_display.c
    vector_display.h
    vector_display_platform.h
    vector_display_platform_ios.c

implement a reusable and platform independent vector display. The remaining 
files make up the iOS demo app.

Screenshots:

![screenshot](https://raw.github.com/blucz/Vector/master/images/screenshot.png)
![screenshot](https://raw.github.com/blucz/Vector/master/images/testpattern.png)

