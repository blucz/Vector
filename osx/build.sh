gcc -o foo \
-I../Vector -Wall -g \
osx_main.c \
../Vector/vector_display.c \
../Vector/vector_display_platform_ios.c \
../Vector/vector_display_utils.c \
../Vector/vector_font_simplex.c \
-framework Carbon -framework OpenGL -framework GLUT -lm
