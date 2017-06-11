# OpenGL .OBJ model importer
This is a small program i wrote that i will use as a template for the model import features of my GLSL IDE.

Loading a .obj file via command line is possible:
```Usage: GLtest file.obj```

This program is mainly targeted towards Linux distributions, although compilation on Windows is also possible with some modifications.
This program uses GLFW to create a GL context and requires glew.c to be included in the source directory if you wish to compile it on 
Windows.
