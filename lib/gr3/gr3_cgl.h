#ifndef GR3_CGL_H_INCLUDED
#define GR3_CGL_H_INCLUDED

#include <OpenGL/OpenGL.h>
/* OpenGL.h in Mac OS X 10.7 doesn't include gl.h anymore */
#include <OpenGL/gl.h>

#include <unistd.h> /* for getpid() for tempfile names */

#if GL_VERSION_2_1
#define GR3_CAN_USE_VBO
#endif

#if !(GL_ARB_framebuffer_object || GL_EXT_framebuffer_object)
#error "Neither GL_ARB_framebuffer_object nor GL_EXT_framebuffer_object \
are supported!"
#endif

int gr3_initGL_CGL_(void);
void gr3_terminateGL_CGL_(void);

#endif
