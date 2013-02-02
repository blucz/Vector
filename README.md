Vector
======

Vector emulates a vector display, like those found on the Tektronix terminals
and in arcade games such as Asteroids and Tempest. It is implemented in OpenGL
and OpenGL ES2.

For example:
- *Tektronix* http://www.youtube.com/watch?v=tpD1QXvtlcg
- *Arcade games* http://www.youtube.com/watch?v=0lIzugbsCMk

It targets the iPad and OSX for now. For best results, use an iPad 4 or a
Retina MacBookPro. Vector runs poorly on the simulator and I haven't tested
anything slower than an iPad 4.

In its current state, it's a sample program that demonstrates the most basic of
vector display functionality.

The files are organized as follows:

- *Vector/* Implements the reusable code: a platform independent vector display.
- *test/* Makes up the basic test drawing. Used by all test applications.
- *ios/* The iOS demo application boilerplate.
- *osx/* The Mac OS X demo application boilerplate.

The font in the screenshot is the "simplex" font. More info on that: http://paulbourke.net/dataformats/hershey/

Screenshots
-----------

![screenshot](https://raw.github.com/blucz/Vector/master/images/testpattern.png)

