#ifndef GR3_GLX_H_INCLUDED
#define GR3_GLX_H_INCLUDED

#ifdef NO_GL
#define GR3_USE_SR
#define GLfloat float
#define GLuint unsigned int
#define GLint int
#endif

#ifndef NO_GL
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <unistd.h> /* for getpid() for tempfile names */

#if GL_VERSION_2_1
#define GR3_CAN_USE_VBO
#endif

#if !(GL_ARB_framebuffer_object || GL_EXT_framebuffer_object)
#error "Neither GL_ARB_framebuffer_object nor GL_EXT_framebuffer_object \
are supported!"
#endif

#ifdef GR3_GLX_C
#define GLFUNC
#else
#define GLFUNC extern
#endif

#ifdef GR3_CAN_USE_VBO
GLFUNC PFNGLBUFFERDATAPROC gr3_glBufferData;
GLFUNC PFNGLBINDBUFFERPROC gr3_glBindBuffer;
GLFUNC PFNGLGENBUFFERSPROC gr3_glGenBuffers;
GLFUNC PFNGLGENBUFFERSPROC gr3_glDeleteBuffers;
GLFUNC PFNGLVERTEXATTRIBPOINTERPROC gr3_glVertexAttribPointer;
GLFUNC PFNGLGETATTRIBLOCATIONPROC gr3_glGetAttribLocation;
GLFUNC PFNGLENABLEVERTEXATTRIBARRAYPROC gr3_glEnableVertexAttribArray;
GLFUNC PFNGLUSEPROGRAMPROC gr3_glUseProgram;
GLFUNC PFNGLDELETESHADERPROC gr3_glDeleteShader;
GLFUNC PFNGLLINKPROGRAMPROC gr3_glLinkProgram;
GLFUNC PFNGLATTACHSHADERPROC gr3_glAttachShader;
GLFUNC PFNGLCREATESHADERPROC gr3_glCreateShader;
GLFUNC PFNGLCOMPILESHADERPROC gr3_glCompileShader;
GLFUNC PFNGLCREATEPROGRAMPROC gr3_glCreateProgram;
GLFUNC PFNGLDELETEPROGRAMPROC gr3_glDeleteProgram;
GLFUNC PFNGLUNIFORM1IPROC gr3_glUniform1i;
GLFUNC PFNGLUNIFORM3FPROC gr3_glUniform3f;
GLFUNC PFNGLUNIFORMMATRIX4FVPROC gr3_glUniformMatrix4fv;
GLFUNC PFNGLUNIFORM4FPROC gr3_glUniform4f;
GLFUNC PFNGLGETUNIFORMLOCATIONPROC gr3_glGetUniformLocation;
GLFUNC PFNGLSHADERSOURCEPROC gr3_glShaderSource;
GLFUNC PFNGLGETSHADERIVPROC gr3_glGetShaderiv;
GLFUNC PFNGLGETPROGRAMIVPROC gr3_glGetProgramiv;
GLFUNC PFNGLACTIVETEXTUREPROC gr3_glActiveTexture;
GLFUNC PFNGLTEXIMAGE3DPROC gr3_glTexImage3D;
#endif
GLFUNC PFNGLDRAWBUFFERSPROC gr3_glDrawBuffers;
#ifdef GL_ARB_framebuffer_object
GLFUNC PFNGLBINDRENDERBUFFERPROC gr3_glBindRenderbuffer;
GLFUNC PFNGLCHECKFRAMEBUFFERSTATUSPROC gr3_glCheckFramebufferStatus;
GLFUNC PFNGLFRAMEBUFFERRENDERBUFFERPROC gr3_glFramebufferRenderbuffer;
GLFUNC PFNGLRENDERBUFFERSTORAGEPROC gr3_glRenderbufferStorage;
GLFUNC PFNGLBINDFRAMEBUFFERPROC gr3_glBindFramebuffer;
GLFUNC PFNGLGENFRAMEBUFFERSPROC gr3_glGenFramebuffers;
GLFUNC PFNGLGENRENDERBUFFERSPROC gr3_glGenRenderbuffers;
GLFUNC PFNGLDELETEFRAMEBUFFERSPROC gr3_glDeleteFramebuffers;
GLFUNC PFNGLDELETERENDERBUFFERSPROC gr3_glDeleteRenderbuffers;
GLFUNC PFNGLFRAMEBUFFERTEXTURE2DPROC gr3_glFramebufferTexture2D;
#endif
#ifdef GL_EXT_framebuffer_object
GLFUNC PFNGLBINDRENDERBUFFEREXTPROC gr3_glBindRenderbufferEXT;
GLFUNC PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC gr3_glCheckFramebufferStatusEXT;
GLFUNC PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC gr3_glFramebufferRenderbufferEXT;
GLFUNC PFNGLRENDERBUFFERSTORAGEEXTPROC gr3_glRenderbufferStorageEXT;
GLFUNC PFNGLBINDFRAMEBUFFEREXTPROC gr3_glBindFramebufferEXT;
GLFUNC PFNGLGENFRAMEBUFFERSEXTPROC gr3_glGenFramebuffersEXT;
GLFUNC PFNGLGENRENDERBUFFERSEXTPROC gr3_glGenRenderbuffersEXT;
GLFUNC PFNGLDELETEFRAMEBUFFERSEXTPROC gr3_glDeleteFramebuffersEXT;
GLFUNC PFNGLDELETERENDERBUFFERSEXTPROC gr3_glDeleteRenderbuffersEXT;
GLFUNC PFNGLFRAMEBUFFERTEXTURE2DEXTPROC gr3_glFramebufferTexture2DEXT;
#endif
GLFUNC void (*gr3_glBegin)(GLenum);
GLFUNC void (*gr3_glBlendColor)(GLclampf, GLclampf, GLclampf, GLclampf);
GLFUNC void (*gr3_glBlendFunc)(GLenum, GLenum);
GLFUNC void (*gr3_glCallList)(GLuint);
GLFUNC void (*gr3_glClear)(GLenum);
GLFUNC void (*gr3_glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
GLFUNC void (*gr3_glColor3fv)(const GLfloat *);
GLFUNC void (*gr3_glColorMaterial)(GLenum, GLenum);
GLFUNC void (*gr3_glDeleteLists)(GLuint, GLsizei);
GLFUNC void (*gr3_glDisable)(GLenum);
GLFUNC void (*gr3_glDrawArrays)(GLenum, GLint, GLsizei);
GLFUNC void (*gr3_glDrawElements)(GLenum, GLsizei, GLenum, const GLvoid *);
GLFUNC void (*gr3_glEnable)(GLenum);
GLFUNC void (*gr3_glEnd)(void);
GLFUNC void (*gr3_glEndList)(void);
GLFUNC GLuint (*gr3_glGenLists)(GLsizei);
GLFUNC GLenum (*gr3_glGetError)(void);
GLFUNC void (*gr3_glGetBooleanv)(GLenum, GLboolean *);
GLFUNC void (*gr3_glGetFloatv)(GLenum, GLfloat *);
GLFUNC void (*gr3_glGetIntegerv)(GLenum, GLint *);
GLFUNC const GLubyte *(*gr3_glGetString)(GLenum);
GLFUNC void (*gr3_glLightfv)(GLenum, GLenum, const GLfloat *);
GLFUNC void (*gr3_glLoadIdentity)(void);
GLFUNC void (*gr3_glLoadMatrixf)(const GLfloat *);
GLFUNC void (*gr3_glMaterialfv)(GLenum, GLenum, const GLfloat *);
GLFUNC void (*gr3_glMatrixMode)(GLenum);
GLFUNC void (*gr3_glMultMatrixf)(const GLfloat *);
GLFUNC void (*gr3_glNewList)(GLuint, GLenum);
GLFUNC void (*gr3_glNormal3fv)(const GLfloat *);
GLFUNC void (*gr3_glPixelStorei)(GLenum, GLint);
GLFUNC void (*gr3_glPopMatrix)(void);
GLFUNC void (*gr3_glPushMatrix)(void);
GLFUNC void (*gr3_glReadBuffer)(GLenum);
GLFUNC void (*gr3_glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
GLFUNC void (*gr3_glVertex3fv)(const GLfloat *);
GLFUNC void (*gr3_glViewport)(GLint, GLint, GLsizei, GLsizei);
GLFUNC void (*gr3_glCullFace)(GLenum);
GLFUNC void (*gr3_glGenTextures)(GLsizei, GLuint *);
GLFUNC void (*gr3_glDeleteTextures)(GLsizei, const GLuint *);
GLFUNC void (*gr3_glBindTexture)(GLenum, GLuint);
GLFUNC void (*gr3_glTexParameteri)(GLenum, GLenum, GLint);
GLFUNC void (*gr3_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

#define glBufferData gr3_glBufferData
#define glBindBuffer gr3_glBindBuffer
#define glGenBuffers gr3_glGenBuffers
#define glDeleteBuffers gr3_glDeleteBuffers
#define glVertexAttribPointer gr3_glVertexAttribPointer
#define glGetAttribLocation gr3_glGetAttribLocation
#define glEnableVertexAttribArray gr3_glEnableVertexAttribArray
#define glUseProgram gr3_glUseProgram
#define glDeleteShader gr3_glDeleteShader
#define glLinkProgram gr3_glLinkProgram
#define glAttachShader gr3_glAttachShader
#define glCreateShader gr3_glCreateShader
#define glCompileShader gr3_glCompileShader
#define glCreateProgram gr3_glCreateProgram
#define glDeleteProgram gr3_glDeleteProgram
#define glUniform1i gr3_glUniform1i
#define glUniform3f gr3_glUniform3f
#define glUniformMatrix4fv gr3_glUniformMatrix4fv
#define glUniform4f gr3_glUniform4f
#define glGetUniformLocation gr3_glGetUniformLocation
#define glShaderSource gr3_glShaderSource
#define glGetShaderiv gr3_glGetShaderiv
#define glGetProgramiv gr3_glGetProgramiv
#define glActiveTexture gr3_glActiveTexture
#define glTexImage3D gr3_glTexImage3D
#define glDrawBuffers gr3_glDrawBuffers
#define glBindRenderbuffer gr3_glBindRenderbuffer
#define glCheckFramebufferStatus gr3_glCheckFramebufferStatus
#define glFramebufferRenderbuffer gr3_glFramebufferRenderbuffer
#define glRenderbufferStorage gr3_glRenderbufferStorage
#define glBindFramebuffer gr3_glBindFramebuffer
#define glGenFramebuffers gr3_glGenFramebuffers
#define glGenRenderbuffers gr3_glGenRenderbuffers
#define glDeleteFramebuffers gr3_glDeleteFramebuffers
#define glDeleteRenderbuffers gr3_glDeleteRenderbuffers
#define glFramebufferTexture2D gr3_glFramebufferTexture2D
#define glBindRenderbufferEXT gr3_glBindRenderbufferEXT
#define glCheckFramebufferStatusEXT gr3_glCheckFramebufferStatusEXT
#define glFramebufferRenderbufferEXT gr3_glFramebufferRenderbufferEXT
#define glRenderbufferStorageEXT gr3_glRenderbufferStorageEXT
#define glBindFramebufferEXT gr3_glBindFramebufferEXT
#define glGenFramebuffersEXT gr3_glGenFramebuffersEXT
#define glGenRenderbuffersEXT gr3_glGenRenderbuffersEXT
#define glDeleteFramebuffersEXT gr3_glDeleteFramebuffersEXT
#define glDeleteRenderbuffersEXT gr3_glDeleteRenderbuffersEXT
#define glFramebufferTexture2DEXT gr3_glFramebufferTexture2DEXT

#define glBegin gr3_glBegin
#define glBlendColor gr3_glBlendColor
#define glBlendFunc gr3_glBlendFunc
#define glCallList gr3_glCallList
#define glClear gr3_glClear
#define glClearColor gr3_glClearColor
#define glColor3fv gr3_glColor3fv
#define glColorMaterial gr3_glColorMaterial
#define glDeleteLists gr3_glDeleteLists
#define glDisable gr3_glDisable
#define glDrawArrays gr3_glDrawArrays
#define glDrawElements gr3_glDrawElements
#define glEnable gr3_glEnable
#define glEnd gr3_glEnd
#define glEndList gr3_glEndList
#define glGenLists gr3_glGenLists
#define glGetError gr3_glGetError
#define glGetBooleanv gr3_glGetBooleanv
#define glGetFloatv gr3_glGetFloatv
#define glGetIntegerv gr3_glGetIntegerv
#define glGetString gr3_glGetString
#define glLightfv gr3_glLightfv
#define glLoadIdentity gr3_glLoadIdentity
#define glLoadMatrixf gr3_glLoadMatrixf
#define glMaterialfv gr3_glMaterialfv
#define glMatrixMode gr3_glMatrixMode
#define glMultMatrixf gr3_glMultMatrixf
#define glNewList gr3_glNewList
#define glNormal3fv gr3_glNormal3fv
#define glPixelStorei gr3_glPixelStorei
#define glPopMatrix gr3_glPopMatrix
#define glPushMatrix gr3_glPushMatrix
#define glReadBuffer gr3_glReadBuffer
#define glReadPixels gr3_glReadPixels
#define glVertex3fv gr3_glVertex3fv
#define glViewport gr3_glViewport
#define glCullFace gr3_glCullFace
#define glGenTextures gr3_glGenTextures
#define glDeleteTextures gr3_glDeleteTextures
#define glBindTexture gr3_glBindTexture
#define glTexParameteri gr3_glTexParameteri
#define glTexImage2D gr3_glTexImage2D

#endif

int gr3_initGL_GLX_(void);

#endif
