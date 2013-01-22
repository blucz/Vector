#ifdef __APPLE__
#    include "TargetConditionals.h"
#    ifdef TARGET_OS_IPHONE
#        include <OpenGLES/ES2/gl.h>
#        include <OpenGLES/ES2/glext.h>
#    elif defined TARGET_OS_MAC
#        include <GLUT/glut.h>
#        include <OpenGL/OpenGL.h>
#    endif
#elif defined _WIN32 || defined _WIN64
#    include <GL\glut.h>
#endif
