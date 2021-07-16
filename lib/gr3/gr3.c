/*!\file gr3.c
 * ToDo:
 * -
 * Bugs:
 * - glXCreatePbuffer -> "failed to create drawable" probably caused by being
 *      run in virtual box, other applications show the same error message
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gr3.h"
#include "gr3_internals.h"
#include "gr3_sr.h"

#ifndef is_nan
#define is_nan(a) ((a) != (a))
#endif

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

/*!
 * This function pointer holds the function used by gr3_log_(). It can be set
 * with gr3_setlogcallback().
 */
static void (*gr3_log_func_)(const char *log_message) = NULL;

/*!
 * This char array is returned by gr3_getrenderpathstring() if it is called
 * without calling gr3_init() successfully first.
 */
static char not_initialized_[] = "Not initialized";

/*!
 * The id of the framebuffer object used for rendering.
 */
static GLuint framebuffer = 0;
static GLuint user_framebuffer = 0;

static int current_object_id = 0;

int gr3_error_ = GR3_ERROR_NONE;
int gr3_error_line_ = 0;
const char *gr3_error_file_ = "";

/*!
 * The default values for the instances of GR3_InitStruct_t_:\n
 * GR3_InitStruct_t_::framebuffer_width = 512;\n
 * GR3_InitStruct_t_::framebuffer_height = 512;
 * GR3_InitStruct_t_::num_threads = 0;
 */
#define GR3_InitStruct_INITIALIZER \
  {                                \
    512, 512, 0                    \
  }

/*!
 * The only instance of ::GR3_ContextStruct_t_. For documentation, see
 * ::_GR3_ContextStruct_t_.
 */
#ifndef NO_THREADS
#define GR3_ContextStruct_INITIALIZER                                                                                 \
  {                                                                                                                   \
    GR3_InitStruct_INITIALIZER, 0, 0, 0, NULL, 0, NULL, not_initialized_, NULL, NULL, 0, 0, {{0}}, 0, 0, 0, NAN, NAN, \
        NAN, NAN, {0, 0, 0, 0}, 0, 0, 0, 0, 0, {0, 0, 0, 1}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0, -1, 0,   \
        {0}, {0}, {0}, {0}, 0, 0, 0                                                                                   \
  }
#else
#define GR3_ContextStruct_INITIALIZER                                                                                 \
  {                                                                                                                   \
    GR3_InitStruct_INITIALIZER, 0, 0, 0, NULL, 0, NULL, not_initialized_, NULL, NULL, 0, 0, {{0}}, 0, 0, 0, NAN, NAN, \
        NAN, NAN, {0, 0, 0, 0}, 0, 0, 0, 0, 0, {0, 0, 0, 1}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0, -1, 0,   \
        {0}, {0}, {0}, 0, 0                                                                                           \
  }
#endif
GR3_ContextStruct_t_ context_struct_ = GR3_ContextStruct_INITIALIZER;

/* For documentation, see the definition. */
static int gr3_extensionsupported_(const char *extension_name);
static void gr3_meshaddreference_(int mesh);
static void gr3_meshremovereference_(int mesh);
static void gr3_dodrawmesh_(int mesh, int n, const float *positions, const float *directions, const float *ups,
                            const float *colors, const float *scales);

static int gr3_getpixmap_(char *bitmap, int width, int height, int use_alpha, int ssaa_factor);
static int gr3_drawimage_opengl_(float xmin, float xmax, float ymin, float ymax, int width, int height);

static int gr3_allocate_meshdata_(int num_vertices, float **vertices, float **normals, float **colors, int num_indices,
                                  int **indices);

#if GL_EXT_framebuffer_object
static int gr3_initFBO_EXT_(void);
static void gr3_terminateFBO_EXT_(void);
#endif
#if GL_ARB_framebuffer_object
static int gr3_initFBO_ARB_(void);
static void gr3_terminateFBO_ARB_(void);
#endif

static void gr3_projectionmatrix_(float left, float right, float bottom, float top, float znear, float zfar,
                                  GLfloat *matrix);

/*!
 * This method initializes the gr3 context.
 *
 * \param [in] attrib_list  This ::GR3_IA_END_OF_LIST-terminated list can
 *                          specify details about context creation. The
 *                          attributes which use unsigned integer values are
 *                          followed by these values.
 * \return
 * - ::GR3_ERROR_NONE              on success
 * - ::GR3_ERROR_INVALID_VALUE     if one of the attributes' values is out of the
 *                                 allowed range
 * - ::GR3_ERROR_INVALID_ATTRIBUTE if the list contained an unkown attribute or two
 *                                 mutually exclusive attributes were both used
 * - ::GR3_ERROR_OPENGL_ERR        if an OpenGL error occured
 * - ::GR3_ERROR_INIT_FAILED       if an error occured during initialization of the
 *                                 "window toolkit" (CGL, GLX, WIN...)
 * \sa gr3_terminate()
 * \sa context_struct_
 * \sa int
 */
GR3API int gr3_init(int *attrib_list)
{
  int i, error;
  char *renderpath_string = "gr3";
  char *software_renderer;
  software_renderer = getenv("GR3_USE_SR");
#ifdef NO_GL
  software_renderer = "True";
#endif
  GR3_InitStruct_t_ init_struct = GR3_InitStruct_INITIALIZER;
  if (attrib_list)
    {
      for (i = 0; attrib_list[i] != GR3_IA_END_OF_LIST; i++)
        {
          switch (attrib_list[i])
            {
            case GR3_IA_FRAMEBUFFER_WIDTH:
              init_struct.framebuffer_width = attrib_list[++i];
              if (attrib_list[i] <= 0) RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
              break;
            case GR3_IA_FRAMEBUFFER_HEIGHT:
              init_struct.framebuffer_height = attrib_list[++i];
              if (attrib_list[i] <= 0) RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
              break;
            case GR3_IA_NUM_THREADS:
              init_struct.num_threads = attrib_list[++i];
              if (attrib_list[i] <= 0) RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
              break;
            default:
              RETURN_ERROR(GR3_ERROR_INVALID_ATTRIBUTE);
            }
        }
    }
  context_struct_.init_struct = init_struct;

  context_struct_.renderpath_string = malloc(strlen(renderpath_string) + 1);
  strcpy(context_struct_.renderpath_string, renderpath_string);
  error = GR3_ERROR_INIT_FAILED;
  do
    {
      if (software_renderer == NULL)
        {
#if defined(GR3_USE_CGL)
          error = gr3_initGL_CGL_();
          if (error == GR3_ERROR_NONE)
            {
              break;
            }
#endif
#if defined(GR3_USE_GLX)
          error = gr3_initGL_GLX_();
          if (error == GR3_ERROR_NONE)
            {
              break;
            }
#endif
#if defined(GR3_USE_WIN)
          error = gr3_initGL_WIN_();
          if (error == GR3_ERROR_NONE)
            {
              break;
            }
#endif
        }
      else
        {
          error = gr3_initSR_();
          if (error == GR3_ERROR_NONE)
            {
              break;
            }
        }
    }
  while (0);
#ifndef NO_GL
  if (!context_struct_.use_software_renderer &&
      (error != GR3_ERROR_NONE || strncmp((const char *)glGetString(GL_VERSION), "2.1", 3) < 0))
    {
      if (context_struct_.gl_is_initialized)
        {
          context_struct_.terminateGL();
        }
      error = gr3_initSR_();
      if (error != GR3_ERROR_NONE)
        {
          gr3_terminate();
          RETURN_ERROR(error);
        }
    }
#else
  if (error != GR3_ERROR_NONE)
    {
      gr3_terminate();
      RETURN_ERROR(error);
    }
#endif

  /* GL_ARB_framebuffer_object is core since OpenGL 3.0 */
  if (!context_struct_.use_software_renderer)
    {
#if GL_ARB_framebuffer_object
      if (!strncmp((const char *)glGetString(GL_VERSION), "3.", 2) ||
          !strncmp((const char *)glGetString(GL_VERSION), "4.", 2) ||
          gr3_extensionsupported_("GL_ARB_framebuffer_object"))
        {
          error = gr3_initFBO_ARB_();
          if (error)
            {
              gr3_terminate();
              return error;
            }
        }
      else
#endif
#if GL_EXT_framebuffer_object
          if (gr3_extensionsupported_("GL_EXT_framebuffer_object"))
        {
          error = gr3_initFBO_EXT_();
          if (error)
            {
              gr3_terminate();
              return error;
            }
        }
      else
#endif
        {
          gr3_terminate();
          RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
        }

#ifdef GR3_CAN_USE_VBO
      if (strncmp((const char *)glGetString(GL_VERSION), "2.1", 3) >= 0)
        {
          context_struct_.use_vbo = 1;
        }
      if (context_struct_.use_vbo)
        {
          GLuint program;
          GLuint fragment_shader;
          GLuint vertex_shader;
          char *vertex_shader_source[] = {
              "#version 120\n",
              "uniform mat4 ProjectionMatrix;\n",
              "uniform mat4 ViewMatrix;\n",
              "uniform mat4 ModelMatrix;\n",
              "uniform vec3 LightDirection;\n",
              "uniform vec4 Scales;\n",

              "attribute vec3 in_Vertex;\n"
              "attribute vec3 in_Normal;\n"
              "attribute vec3 in_Color;\n"

              "varying vec4 Color;\n",
              "varying vec3 Normal;\n",

              "void main(void) {\n",
              "vec4 Position = ViewMatrix*ModelMatrix*(Scales*vec4(in_Vertex,1));\n",
              "gl_Position=ProjectionMatrix*Position;\n",
              "Normal = normalize(mat3(ViewMatrix)*mat3(ModelMatrix)*(in_Normal/vec3(Scales)));\n",
              "Color = vec4(in_Color,1);\n",
              "float diffuse = Normal.z;\n",
              "if (dot(LightDirection,LightDirection) > 0.001) {",
              "diffuse = dot(mat3(ViewMatrix)*normalize(LightDirection),Normal);",
              "}",
              "Color.rgb = diffuse*Color.rgb;"
              "}\n"};

          char *fragment_shader_source[] = {"#version 120\n",
                                            "varying vec4 Color;\n",
                                            "varying vec3 Normal;\n",
                                            "uniform mat4 ViewMatrix;\n",

                                            "void main(void) {\n",
                                            "gl_FragColor=vec4(Color.rgb,Color.a);\n",
                                            "}\n"};
          program = glCreateProgram();
          vertex_shader = glCreateShader(GL_VERTEX_SHADER);
          glShaderSource(vertex_shader, sizeof(vertex_shader_source) / sizeof(char *),
                         (const GLchar **)vertex_shader_source, NULL);
          glCompileShader(vertex_shader);
          fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
          glShaderSource(fragment_shader, sizeof(fragment_shader_source) / sizeof(char *),
                         (const GLchar **)fragment_shader_source, NULL);
          glCompileShader(fragment_shader);
          glAttachShader(program, vertex_shader);
          glAttachShader(program, fragment_shader);
          glLinkProgram(program);
          glDeleteShader(vertex_shader);
          glDeleteShader(fragment_shader);
          context_struct_.program = program;
          glUseProgram(program);

          gr3_appendtorenderpathstring_("Vertex Buffer Objects");
        }
      else
#endif
        {
          gr3_appendtorenderpathstring_("Display Lists");
        }

#ifndef NO_GL
      gr3_appendtorenderpathstring_((const char *)glGetString(GL_VERSION));
      gr3_appendtorenderpathstring_((const char *)glGetString(GL_RENDERER));
#else
      gr3_appendtorenderpathstring_("Software-Renderer");
#endif
    }

  context_struct_.is_initialized = 1;
  gr3_init_convenience();
  gr3_useframebuffer(0);
  gr3_setcameraprojectionparameters(45, 1, 200);
  gr3_cameralookat(0, 0, 10, 0, 0, 0, 0, 1, 0);
  gr3_log_("init completed successfully");
  RETURN_ERROR(GR3_ERROR_NONE);
}


/*!
 * This function returns information on the most recent GR3 error.
 * \param [in] clear 1 if the error information should be cleared
 * \param [out] line the code line the error was returned from
 * \param [out] file the code file the error was returned from
 * \returns the last non-zero error code
 */
GR3API int gr3_geterror(int clear, int *line, const char **file)
{
  int error;
  if (gr3_error_ && line)
    {
      *line = gr3_error_line_;
    }
  if (gr3_error_ && file)
    {
      *file = gr3_error_file_;
    }
  error = gr3_error_;
  if (clear)
    {
      gr3_error_ = GR3_ERROR_NONE;
      gr3_error_file_ = "";
      gr3_error_line_ = 0;
    }
  return error;
}

/*!
 * This helper function checks whether an OpenGL extension is supported.
 * \param [in] extension_name The extension that should be checked for,
 *             e.g. GL_EXT_framebuffer_object. The notation has to
 *             be the same as in the GL_EXTENSIONS string.
 * \returns 1 on success, 0 otherwise.
 */
static int gr3_extensionsupported_(const char *extension_name)
{
#ifndef NO_GL
  const char *extension_string = (const char *)glGetString(GL_EXTENSIONS);
  return strstr(extension_string, extension_name) != NULL;
#else
  (void)extension_name;
  return 0;
#endif
}

/*!
 * This function terminates the gr3 context.
 * After calling this function, gr3 is in the same state as when it was first
 * loaded, except for context-independent variables, i.e. the logging callback.
 * \note It is safe to call this function even if the context is not
 * initialized.
 * \sa gr3_init()
 * \sa context_struct_
 */
GR3API void gr3_terminate(void)
{
  if (context_struct_.gl_is_initialized)
    {
#ifdef GR3_CAN_USE_VBO
      if (context_struct_.use_vbo)
        {
          glUseProgram(0);
          glDeleteProgram(context_struct_.program);
        }
#endif
      gr3_terminate_convenience();
      if (context_struct_.fbo_is_initialized)
        {
          int i;
          gr3_clear();
          for (i = 0; i < context_struct_.mesh_list_capacity_; i++)
            {
              if (context_struct_.mesh_list_[i].data.data.display_list_id != 0)
                {
#ifndef NO_GL
                  glDeleteLists(context_struct_.mesh_list_[i].data.data.display_list_id, 1);
#endif
                  context_struct_.mesh_list_[i].data.data.display_list_id = 0;
                  free(context_struct_.mesh_list_[i].data.vertices);
                  free(context_struct_.mesh_list_[i].data.normals);
                  free(context_struct_.mesh_list_[i].data.colors);
                  context_struct_.mesh_list_[i].refcount = 0;
                  context_struct_.mesh_list_[i].marked_for_deletion = 0;
                }
            }

          free(context_struct_.mesh_list_);
          context_struct_.mesh_list_ = NULL;
          context_struct_.mesh_list_capacity_ = 0;
          context_struct_.mesh_list_first_free_ = 0;

          context_struct_.terminateFBO();
        }
      if (!context_struct_.use_software_renderer)
        {
          context_struct_.terminateGL();
          context_struct_.gl_is_initialized = 0;
        }
    }
  context_struct_.is_initialized = 0;
  if (context_struct_.renderpath_string != not_initialized_)
    {
      free(context_struct_.renderpath_string);
      context_struct_.renderpath_string = not_initialized_;
    }
  if (context_struct_.use_software_renderer)
    {
      if (context_struct_.software_renderer_pixmaps_initalised)
        {
          gr3_terminateSR_();
        }
    }
  {
    GR3_ContextStruct_t_ initializer = GR3_ContextStruct_INITIALIZER;
    context_struct_ = initializer;
  }
}

/*!
 * This function clears the draw list.
 * \returns
 * - ::GR3_ERROR_NONE             on success
 * - ::GR3_ERROR_OPENGL_ERR       if an OpenGL error occured
 * - ::GR3_ERROR_NOT_INITIALIZED  if the function was called without
 *                                calling gr3_init() first
 */
GR3API int gr3_clear(void)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  gr3_log_("gr3_clear();");

  if (context_struct_.is_initialized)
    {
      GR3_DrawList_t_ *draw;
      while (context_struct_.draw_list_)
        {
          draw = context_struct_.draw_list_;
          int i;
          if (context_struct_.use_software_renderer && draw->vertices_fp)
            {
              for (i = 0; i < draw->n; i++)
                {
                  if (draw->vertices_fp[i])
                    {
                      free(draw->vertices_fp[i]);
                    }
                }
            }
          free(draw->vertices_fp);
          context_struct_.draw_list_ = draw->next;
          gr3_meshremovereference_(draw->mesh);
          free(draw->positions);
          free(draw->directions);
          free(draw->ups);
          free(draw->colors);
          free(draw->scales);
          free(draw);
        }

#ifdef NO_GL
      RETURN_ERROR(GR3_ERROR_NONE);
#else
      if (context_struct_.use_software_renderer || glGetError() == GL_NO_ERROR)
        {
          RETURN_ERROR(GR3_ERROR_NONE);
        }
      else
        {
          RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
        }
#endif
    }
  else
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }
}


GR3API void gr3_usecurrentframebuffer()
{
  GLuint framebuffer = 0;
#ifndef NO_GL
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&framebuffer);
#endif
  gr3_useframebuffer(framebuffer);
}

GR3API void gr3_useframebuffer(unsigned int framebuffer)
{
  user_framebuffer = framebuffer;
}

/*!
 * This function sets the background color.
 */
GR3API void gr3_setbackgroundcolor(float red, float green, float blue, float alpha)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return;
  if (context_struct_.is_initialized)
    {
      context_struct_.background_color[0] = red;
      context_struct_.background_color[1] = green;
      context_struct_.background_color[2] = blue;
      context_struct_.background_color[3] = alpha;
    }
}

static void gr3_getfirstfreemesh(int *mesh)
{
  void *mem;
  *mesh = context_struct_.mesh_list_first_free_;
  if (context_struct_.mesh_list_first_free_ >= context_struct_.mesh_list_capacity_)
    {
      int new_capacity = context_struct_.mesh_list_capacity_ * 2;
      if (context_struct_.mesh_list_capacity_ == 0)
        {
          new_capacity = 8;
        }
      mem = realloc(context_struct_.mesh_list_, new_capacity * sizeof(*context_struct_.mesh_list_));
      context_struct_.mesh_list_ = mem;
      while (context_struct_.mesh_list_capacity_ < new_capacity)
        {
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].next_free =
              context_struct_.mesh_list_capacity_ + 1;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].refcount = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].marked_for_deletion = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.type = kMTNormalMesh;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.data.display_list_id = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.data.vertex_buffer_id = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.number_of_vertices = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.number_of_indices = 0;
          context_struct_.mesh_list_[context_struct_.mesh_list_capacity_].data.vertices_fp = NULL;
          context_struct_.mesh_list_capacity_++;
        }
    }
  context_struct_.mesh_list_first_free_ = context_struct_.mesh_list_[*mesh].next_free;
}

void gr3_sortindexedmeshdata(int mesh)
{
  if (context_struct_.mesh_list_[mesh].data.type != kMTIndexedMesh)
    {
      return;
    }
  else if (context_struct_.mesh_list_[mesh].data.indices == NULL)
    {
      return;
    }
  else
    {
      int i;
      float *vertices = (float *)malloc(sizeof(float) * context_struct_.mesh_list_[mesh].data.number_of_indices * 3);
      float *colors = (float *)malloc(sizeof(float) * context_struct_.mesh_list_[mesh].data.number_of_indices * 3);
      float *normals = (float *)malloc(sizeof(float) * context_struct_.mesh_list_[mesh].data.number_of_indices * 3);
      for (i = 0; i < context_struct_.mesh_list_[mesh].data.number_of_indices; i++)
        {
          memmove(vertices + i * 3,
                  context_struct_.mesh_list_[mesh].data.vertices + context_struct_.mesh_list_[mesh].data.indices[i] * 3,
                  sizeof(float) * 3);
          memmove(normals + i * 3,
                  context_struct_.mesh_list_[mesh].data.normals + context_struct_.mesh_list_[mesh].data.indices[i] * 3,
                  sizeof(float) * 3);
          memmove(colors + i * 3,
                  context_struct_.mesh_list_[mesh].data.colors + context_struct_.mesh_list_[mesh].data.indices[i] * 3,
                  sizeof(float) * 3);
        }
      context_struct_.mesh_list_[mesh].data.number_of_vertices =
          context_struct_.mesh_list_[mesh].data.number_of_indices;
      free(context_struct_.mesh_list_[mesh].data.vertices);
      free(context_struct_.mesh_list_[mesh].data.normals);
      free(context_struct_.mesh_list_[mesh].data.colors);
      free(context_struct_.mesh_list_[mesh].data.indices);
      context_struct_.mesh_list_[mesh].data.vertices = vertices;
      context_struct_.mesh_list_[mesh].data.colors = colors;
      context_struct_.mesh_list_[mesh].data.normals = normals;
      context_struct_.mesh_list_[mesh].data.indices = NULL;
      context_struct_.mesh_list_[mesh].data.vertices_fp = NULL;
    }
}

/*
 * allocate memory for mesh the data arrays and check for errors
 * if indices is NULL, no memory will be allocated for that
 * returns GR3_ERROR_NONE or GR3_ERROR_OUT_OF_MEM
 */
static int gr3_allocate_meshdata_(int num_vertices, float **vertices, float **normals, float **colors, int num_indices,
                                  int **indices)
{
  *vertices = malloc(num_vertices * 3 * sizeof(float));
  if (*vertices == NULL)
    {
      goto err0;
    }
  *normals = malloc(num_vertices * 3 * sizeof(float));
  if (*normals == NULL)
    {
      goto err1;
    }
  *colors = malloc(num_vertices * 3 * sizeof(float));
  if (*colors == NULL)
    {
      goto err2;
    }
  if (indices != NULL)
    {
      *indices = malloc(num_indices * sizeof(int));
      if (*indices == NULL)
        {
          goto err3;
        }
    }
  RETURN_ERROR(GR3_ERROR_NONE);

err3:
  free(*colors);
  *colors = NULL;
err2:
  free(*normals);
  *normals = NULL;
err1:
  free(*vertices);
  *vertices = NULL;
err0:
  RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
}

/*!
 * This function creates a mesh from vertex position, normal and color data.
 * The arrays are used directly without copying.
 * The array parameters MUST be pointers to the beginning of a memory
 * region previously allocated by malloc or calloc.
 * Changing the data in these arrays or freeing them leads to
 * undefined behavior.
 * After calling this routine the arrays are owned by GR3 and will be
 * freed by it.
 * If unsure, use gr3_createmesh.
 *
 * \param [out] mesh          a pointer to a int
 * \param [in] vertices       the vertex positions
 * \param [in] normals        the vertex normals
 * \param [in] colors         the vertex colors, they should be white (1,1,1) if
 *                            you want to change the color for each drawn mesh
 * \param [in] n              the number of vertices in the mesh
 *
 * \returns
 *  - ::GR3_ERROR_NONE        on success
 *  - ::GR3_ERROR_OPENGL_ERR  if an OpenGL error occured
 *  - ::GR3_ERROR_OUT_OF_MEM  if a memory allocation failed
 */
GR3API int gr3_createmesh_nocopy(int *mesh, int n, float *vertices, float *normals, float *colors)
{
  int i;
  void *mem;

  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }
  gr3_getfirstfreemesh(mesh);

  context_struct_.mesh_list_[*mesh].data.number_of_vertices = n;
  context_struct_.mesh_list_[*mesh].data.vertices_fp = NULL;
  gr3_meshaddreference_(*mesh);
  context_struct_.mesh_list_[*mesh].data.type = kMTNormalMesh;
#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glGenBuffers(1, &context_struct_.mesh_list_[*mesh].data.data.vertex_buffer_id);
      glBindBuffer(GL_ARRAY_BUFFER, context_struct_.mesh_list_[*mesh].data.data.vertex_buffer_id);
      mem = malloc(n * 3 * 3 * sizeof(GLfloat));
      if (mem == NULL)
        {
          RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
        }
      for (i = 0; i < n; i++)
        {
          GLfloat *data = ((GLfloat *)mem) + i * 3 * 3;
          data[0] = vertices[i * 3 + 0];
          data[1] = vertices[i * 3 + 1];
          data[2] = vertices[i * 3 + 2];
          data[3] = normals[i * 3 + 0];
          data[4] = normals[i * 3 + 1];
          data[5] = normals[i * 3 + 2];
          data[6] = colors[i * 3 + 0];
          data[7] = colors[i * 3 + 1];
          data[8] = colors[i * 3 + 2];
        }
      glBufferData(GL_ARRAY_BUFFER, n * 3 * 3 * sizeof(GLfloat), mem, GL_STATIC_DRAW);
      free(mem);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
  else
#endif
    {
#ifndef NO_GL
      if (!context_struct_.use_software_renderer)
        {
          context_struct_.mesh_list_[*mesh].data.data.display_list_id = glGenLists(1);
          glNewList(context_struct_.mesh_list_[*mesh].data.data.display_list_id, GL_COMPILE);
          glBegin(GL_TRIANGLES);
          for (i = 0; i < n; i++)
            {
              glColor3fv(colors + i * 3);
              glNormal3fv(normals + i * 3);
              glVertex3fv(vertices + i * 3);
            }
          glEnd();
          glEndList();
        }
#endif
    }
  context_struct_.mesh_list_[*mesh].data.vertices = vertices;
  context_struct_.mesh_list_[*mesh].data.normals = normals;
  context_struct_.mesh_list_[*mesh].data.colors = colors;

#ifdef NO_GL
  RETURN_ERROR(GR3_ERROR_NONE);
#else
  if (context_struct_.use_software_renderer || glGetError() == GL_NO_ERROR)
    {
      RETURN_ERROR(GR3_ERROR_NONE);
    }
  else
    {
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }
#endif
}

/*!
 * This function creates a int from vertex position, normal and color data.
 * \param [out] mesh          a pointer to a int
 * \param [in] vertices       the vertex positions
 * \param [in] normals        the vertex normals
 * \param [in] colors         the vertex colors, they should be white (1,1,1) if
 *                            you want to change the color for each drawn mesh
 * \param [in] n              the number of vertices in the mesh
 *
 * \returns
 *  - ::GR3_ERROR_NONE        on success
 *  - ::GR3_ERROR_OPENGL_ERR  if an OpenGL error occured
 *  - ::GR3_ERROR_OUT_OF_MEM  if a memory allocation failed
 */
GR3API int gr3_createmesh(int *mesh, int n, const float *vertices, const float *normals, const float *colors)
{

  float *myvertices, *mynormals, *mycolors;

  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }

  gr3_allocate_meshdata_(n, &myvertices, &mynormals, &mycolors, 0, NULL);
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  memmove(myvertices, vertices, 3 * n * sizeof(float));
  memmove(mynormals, normals, 3 * n * sizeof(float));
  memmove(mycolors, colors, 3 * n * sizeof(float));
  gr3_createmesh_nocopy(mesh, n, myvertices, mynormals, mycolors);
  if (gr3_geterror(0, NULL, NULL))
    {
      free(myvertices);
      free(mynormals);
      free(mycolors);
    }
  return gr3_geterror(0, NULL, NULL);
}

/*!
 * This function creates an indexed mesh from vertex information (position,
 * normal and color) and triangle information (indices).
 * The arrays are used directly without copying.
 * The array parameters MUST be pointers to the beginning of a memory
 * region previously allocated by malloc or calloc.
 * Changing the data in these arrays or freeing them leads to
 * undefined behavior.
 * After calling this routine the arrays are owned by GR3 and will be
 * freed by it.
 * If unsure, use gr3_createindexedmesh.
 *
 * \param [out] mesh              a pointer to an int
 * \param [in] number_of_vertices the number of vertices in the mesh
 * \param [in] vertices           the vertex positions
 * \param [in] normals            the vertex normals
 * \param [in] colors             the vertex colors, they should be
 *                                white (1,1,1) if you want to change the
 *                                color for each drawn mesh
 * \param [in] number_of_indices  the number of indices in the mesh
 *                                (three times the number of triangles)
 * \param [in] indices            the index array (vertex indices for
 *                                each triangle)
 *
 * \returns
 *  - ::GR3_ERROR_NONE        on success
 *  - ::GR3_ERROR_OPENGL_ERR  if an OpenGL error occured
 *  - ::GR3_ERROR_OUT_OF_MEM  if a memory allocation failed
 */
GR3API int gr3_createindexedmesh_nocopy(int *mesh, int number_of_vertices, float *vertices, float *normals,
                                        float *colors, int number_of_indices, int *indices)
{
  int i;
  void *mem;

  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }

  gr3_getfirstfreemesh(mesh);
  gr3_meshaddreference_(*mesh);
  context_struct_.mesh_list_[*mesh].data.type = kMTIndexedMesh;
  context_struct_.mesh_list_[*mesh].data.number_of_vertices = number_of_vertices;
  context_struct_.mesh_list_[*mesh].data.number_of_indices = number_of_indices;
  context_struct_.mesh_list_[*mesh].data.vertices_fp = NULL;
#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glGenBuffers(1, &context_struct_.mesh_list_[*mesh].data.data.buffers.index_buffer_id);
      glGenBuffers(1, &context_struct_.mesh_list_[*mesh].data.data.buffers.vertex_buffer_id);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context_struct_.mesh_list_[*mesh].data.data.buffers.index_buffer_id);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, number_of_indices * sizeof(int), indices, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, context_struct_.mesh_list_[*mesh].data.data.buffers.vertex_buffer_id);
      mem = malloc(number_of_vertices * 3 * 3 * sizeof(GLfloat));
      if (mem == NULL)
        {
          RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
        }
      for (i = 0; i < number_of_vertices; i++)
        {
          GLfloat *data = ((GLfloat *)mem) + i * 3 * 3;
          data[0] = vertices[i * 3 + 0];
          data[1] = vertices[i * 3 + 1];
          data[2] = vertices[i * 3 + 2];
          data[3] = normals[i * 3 + 0];
          data[4] = normals[i * 3 + 1];
          data[5] = normals[i * 3 + 2];
          data[6] = colors[i * 3 + 0];
          data[7] = colors[i * 3 + 1];
          data[8] = colors[i * 3 + 2];
        }
      glBufferData(GL_ARRAY_BUFFER, number_of_vertices * 3 * 3 * sizeof(GLfloat), mem, GL_STATIC_DRAW);
      free(mem);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
  else
#endif
    {
#ifndef NO_GL
      if (!context_struct_.use_software_renderer)
        {
          context_struct_.mesh_list_[*mesh].data.data.display_list_id = glGenLists(1);
          glNewList(context_struct_.mesh_list_[*mesh].data.data.display_list_id, GL_COMPILE);
          glBegin(GL_TRIANGLES);
          for (i = 0; i < number_of_indices; i++)
            {
              glColor3fv(colors + indices[i] * 3);
              glNormal3fv(normals + indices[i] * 3);
              glVertex3fv(vertices + indices[i] * 3);
            }
          glEnd();
          glEndList();
        }
#else
    (void)mem;
    (void)i;
#endif
    }

  context_struct_.mesh_list_[*mesh].data.vertices_fp = NULL;
  context_struct_.mesh_list_[*mesh].data.vertices = vertices;
  context_struct_.mesh_list_[*mesh].data.normals = normals;
  context_struct_.mesh_list_[*mesh].data.colors = colors;
  context_struct_.mesh_list_[*mesh].data.indices = indices;

#ifdef NO_GL
  RETURN_ERROR(GR3_ERROR_NONE);
#else
  if (context_struct_.use_software_renderer || glGetError() == GL_NO_ERROR)
    {
      RETURN_ERROR(GR3_ERROR_NONE);
    }
  else
    {
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }
#endif
}

/*!
 * This function creates an indexed mesh from vertex information (position,
 * normal and color) and triangle information (indices).
 * \param [out] mesh              a pointer to an int
 * \param [in] number_of_vertices the number of vertices in the mesh
 * \param [in] vertices           the vertex positions
 * \param [in] normals            the vertex normals
 * \param [in] colors             the vertex colors, they should be
 *                                white (1,1,1) if you want to change the
 *                                color for each drawn mesh
 * \param [in] number_of_indices  the number of indices in the mesh
 *                                (three times the number of triangles)
 * \param [in] indices            the index array (vertex indices for
 *                                each triangle)
 *
 * \returns
 *  - ::GR3_ERROR_NONE        on success
 *  - ::GR3_ERROR_OPENGL_ERR  if an OpenGL error occured
 *  - ::GR3_ERROR_OUT_OF_MEM  if a memory allocation failed
 */
GR3API int gr3_createindexedmesh(int *mesh, int number_of_vertices, const float *vertices, const float *normals,
                                 const float *colors, int number_of_indices, const int *indices)
{
  float *myvertices, *mynormals, *mycolors;
  int *myindices;
  int err;

  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }

  err = gr3_allocate_meshdata_(number_of_vertices, &myvertices, &mynormals, &mycolors, number_of_indices, &myindices);
  if (err != GR3_ERROR_NONE)
    {
      return err;
    }
  memmove(myvertices, vertices, 3 * number_of_vertices * sizeof(float));
  memmove(mynormals, normals, 3 * number_of_vertices * sizeof(float));
  memmove(mycolors, colors, 3 * number_of_vertices * sizeof(float));
  memmove(myindices, indices, number_of_indices * sizeof(int));
  err = gr3_createindexedmesh_nocopy(mesh, number_of_vertices, myvertices, mynormals, mycolors, number_of_indices,
                                     myindices);
  if (err != GR3_ERROR_NONE && err != GR3_ERROR_OPENGL_ERR)
    {
      free(myvertices);
      free(mynormals);
      free(mycolors);
      free(myindices);
    }

  return err;
}

/*!
 * This function adds a mesh to the draw list, so it will be drawn when the user
 * calls gr3_getpixmap_(). The given data stays owned by the user, a copy will be
 * saved in the draw list and the mesh reference counter will be increased.
 * \param [in] mesh         The mesh to be drawn
 * \param [in] positions    The positions where the meshes should be drawn
 * \param [in] directions   The forward directions the meshes should be facing
 *                          at
 * \param [in] ups          The up directions
 * \param [in] colors       The colors the meshes should be drawn in, it will be
 *                          multiplied with each vertex color
 * \param [in] scales       The scaling factors
 * \param [in] n            The number of meshes to be drawn
 * \note This function does not return an error code, because of its
 * asynchronous nature. If gr3_getpixmap_() returns a ::GR3_ERROR_OPENGL_ERR, this
 * might be caused by this function saving unuseable data into the draw list.
 */
GR3API void gr3_drawmesh(int mesh, int n, const float *positions, const float *directions, const float *ups,
                         const float *colors, const float *scales)
{
  GR3_DrawList_t_ *p, *draw;

  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return;
  if (!context_struct_.is_initialized)
    {
      return;
    }

  draw = malloc(sizeof(GR3_DrawList_t_));
  draw->mesh = mesh;
  draw->positions = malloc(sizeof(float) * n * 3);
  memmove(draw->positions, positions, sizeof(float) * n * 3);
  draw->directions = malloc(sizeof(float) * n * 3);
  memmove(draw->directions, directions, sizeof(float) * n * 3);
  draw->ups = malloc(sizeof(float) * n * 3);
  memmove(draw->ups, ups, sizeof(float) * n * 3);
  draw->colors = malloc(sizeof(float) * n * 3);
  memmove(draw->colors, colors, sizeof(float) * n * 3);
  draw->scales = malloc(sizeof(float) * n * 3);
  memmove(draw->scales, scales, sizeof(float) * n * 3);
  draw->n = n;
  draw->vertices_fp = NULL;
  draw->object_id = current_object_id;
  draw->next = NULL;
  gr3_meshaddreference_(mesh);
  if (context_struct_.draw_list_ == NULL)
    {
      context_struct_.draw_list_ = draw;
    }
  else
    {
      p = context_struct_.draw_list_;
      while (p->next)
        {
          p = p->next;
        }
      p->next = draw;
    }
}

/*!
 * This function does the actual mesh drawing. It will be called with the data
 * in the draw list.
 * \param [in] mesh         The mesh to be drawn
 * \param [in] positions    The positions where the meshes should be drawn
 * \param [in] directions   The forward directions the meshes should be facing
 *                          at
 * \param [in] ups          The up directions
 * \param [in] colors       The colors the meshes should be drawn in, it will be
 *                          multiplied with each vertex color
 * \param [in] scales       The scaling factors
 * \param [in] n            The number of meshes to be drawn
 * \returns
 *  - ::GR3_ERROR_NONE on success
 *  - ::GR3_ERROR_OPENGL_ERR if an OpenGL error occured
 */
#ifndef NO_GL
static void gr3_dodrawmesh_(int mesh, int n, const float *positions, const float *directions, const float *ups,
                            const float *colors, const float *scales)
{
  int i, j;
  GLfloat forward[3], up[3], left[3];
  GLfloat model_matrix[4][4] = {{0}};
  float tmp;
  for (i = 0; i < n; i++)
    {
      {

        /* Calculate an orthonormal base in IR^3, correcting the up vector
         * in case it is not perpendicular to the forward vector. This base
         * is used to create the model matrix as a base-transformation
         * matrix.
         */
        /* forward = normalize(&directions[i*3]); */
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += directions[i * 3 + j] * directions[i * 3 + j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            forward[j] = directions[i * 3 + j] / tmp;
          } /* up = normalize(&ups[i*3]); */
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += ups[i * 3 + j] * ups[i * 3 + j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            up[j] = ups[i * 3 + j] / tmp;
          }
        /* left = cross(forward,up); */
        for (j = 0; j < 3; j++)
          {
            left[j] = forward[(j + 1) % 3] * up[(j + 2) % 3] - up[(j + 1) % 3] * forward[(j + 2) % 3];
          }
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += left[j] * left[j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            left[j] = left[j] / tmp;
          }
        /* up = cross(left,forward); */
        for (j = 0; j < 3; j++)
          {
            up[j] = left[(j + 1) % 3] * forward[(j + 2) % 3] - forward[(j + 1) % 3] * left[(j + 2) % 3];
          }
        if (!context_struct_.use_vbo)
          {
            for (j = 0; j < 3; j++)
              {
                model_matrix[0][j] = -left[j] * scales[i * 3 + 0];
                model_matrix[1][j] = up[j] * scales[i * 3 + 1];
                model_matrix[2][j] = forward[j] * scales[i * 3 + 2];
                model_matrix[3][j] = positions[i * 3 + j];
              }
          }
        else
          {
            for (j = 0; j < 3; j++)
              {
                model_matrix[0][j] = -left[j];
                model_matrix[1][j] = up[j];
                model_matrix[2][j] = forward[j];
                model_matrix[3][j] = positions[i * 3 + j];
              }
          }
        model_matrix[3][3] = 1;
      }
      glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      {
        float nil[4] = {0, 0, 0, 1};
        float one[4] = {1, 1, 1, 1};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, nil);
        glLightfv(GL_LIGHT0, GL_AMBIENT, nil);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, one);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
      }
      glBlendColor(colors[i * 3 + 0], colors[i * 3 + 1], colors[i * 3 + 2], 1);
      glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
      glEnable(GL_BLEND);
#ifdef GR3_CAN_USE_VBO
      if (context_struct_.use_vbo)
        {
          glUniform4f(glGetUniformLocation(context_struct_.program, "Scales"), scales[3 * i + 0], scales[3 * i + 1],
                      scales[3 * i + 2], 1);
          glUniformMatrix4fv(glGetUniformLocation(context_struct_.program, "ModelMatrix"), 1, GL_FALSE,
                             &model_matrix[0][0]);
          if (context_struct_.mesh_list_[mesh].data.type == kMTIndexedMesh)
            {
              glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context_struct_.mesh_list_[mesh].data.data.buffers.index_buffer_id);
              glBindBuffer(GL_ARRAY_BUFFER, context_struct_.mesh_list_[mesh].data.data.buffers.vertex_buffer_id);
            }
          else
            {
              glBindBuffer(GL_ARRAY_BUFFER, context_struct_.mesh_list_[mesh].data.data.vertex_buffer_id);
            }
          glVertexAttribPointer(glGetAttribLocation(context_struct_.program, "in_Vertex"), 3, GL_FLOAT, GL_FALSE,
                                sizeof(GLfloat) * 3 * 3, (void *)(sizeof(GLfloat) * 3 * 0));
          glVertexAttribPointer(glGetAttribLocation(context_struct_.program, "in_Normal"), 3, GL_FLOAT, GL_FALSE,
                                sizeof(GLfloat) * 3 * 3, (void *)(sizeof(GLfloat) * 3 * 1));
          glVertexAttribPointer(glGetAttribLocation(context_struct_.program, "in_Color"), 3, GL_FLOAT, GL_FALSE,
                                sizeof(GLfloat) * 3 * 3, (void *)(sizeof(GLfloat) * 3 * 2));
          glEnableVertexAttribArray(glGetAttribLocation(context_struct_.program, "in_Vertex"));
          glEnableVertexAttribArray(glGetAttribLocation(context_struct_.program, "in_Normal"));
          glEnableVertexAttribArray(glGetAttribLocation(context_struct_.program, "in_Color"));
          if (context_struct_.mesh_list_[mesh].data.type == kMTIndexedMesh)
            {
              glDrawElements(GL_TRIANGLES, context_struct_.mesh_list_[mesh].data.number_of_indices, GL_UNSIGNED_INT,
                             NULL);
            }
          else
            {
              glDrawArrays(GL_TRIANGLES, 0, context_struct_.mesh_list_[mesh].data.number_of_vertices);
            }
        }
      else
#endif
        {
          glPushMatrix();
          glMultMatrixf(&model_matrix[0][0]);
          glCallList(context_struct_.mesh_list_[mesh].data.data.display_list_id);
          glPopMatrix();
        }
      glDisable(GL_BLEND);
    }
}
#endif

/*!
 * This function marks a mesh for deletion and removes the user's reference
 * from the mesh's referenc counter, so a user must not use the mesh after
 * calling this function. If the mesh is still in use for draw calls, the mesh
 * will not be truly deleted until gr3_clear() is called.
 * \param [in] mesh     The mesh that should be marked for deletion
 */
GR3API void gr3_deletemesh(int mesh)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return;
  gr3_log_("gr3_deletemesh_();");
  if (!context_struct_.is_initialized)
    {
      return;
    }
  if (!context_struct_.mesh_list_[mesh].marked_for_deletion)
    {
      gr3_meshremovereference_(mesh);
      if (context_struct_.mesh_list_[mesh].refcount > 0)
        {
          context_struct_.mesh_list_[mesh].marked_for_deletion = 1;
        }
    }
  else
    {
      gr3_log_("Mesh already marked for deletion!");
    }
}

/*!
 * This function adds a reference to the meshes reference counter.
 * \param [in] mesh     The mesh to which a reference will be added
 */
static void gr3_meshaddreference_(int mesh)
{
  context_struct_.mesh_list_[mesh].refcount++;
}

/*!
 * This function removes a reference from the meshes reference counter and
 * deletes the mesh if the reference counter reaches zero.
 * \param [in] mesh     The mesh from which a reference will be removed
 */
static void gr3_meshremovereference_(int mesh)
{
  if (context_struct_.mesh_list_[mesh].refcount > 0)
    {
      context_struct_.mesh_list_[mesh].refcount--;
    }
  if (context_struct_.mesh_list_[mesh].refcount <= 0)
    {
#ifdef GR3_CAN_USE_VBO
      if (context_struct_.use_vbo)
        {
          if (context_struct_.mesh_list_[mesh].data.type == kMTIndexedMesh)
            {
              glDeleteBuffers(1, &context_struct_.mesh_list_[mesh].data.data.buffers.index_buffer_id);
              glDeleteBuffers(1, &context_struct_.mesh_list_[mesh].data.data.buffers.vertex_buffer_id);
            }
          else
            {
              glDeleteBuffers(1, &context_struct_.mesh_list_[mesh].data.data.vertex_buffer_id);
            }
        }
      else
#endif
        {
#ifndef NO_GL
          if (!context_struct_.use_software_renderer)
            {
              glDeleteLists(context_struct_.mesh_list_[mesh].data.data.display_list_id, 1);
            }
#endif
        }
      if (context_struct_.mesh_list_[mesh].data.type == kMTIndexedMesh)
        {
          free(context_struct_.mesh_list_[mesh].data.indices);
        }
      free(context_struct_.mesh_list_[mesh].data.vertices);
      free(context_struct_.mesh_list_[mesh].data.normals);
      free(context_struct_.mesh_list_[mesh].data.colors);
      context_struct_.mesh_list_[mesh].data.data.display_list_id = 0;
      context_struct_.mesh_list_[mesh].refcount = 0;
      context_struct_.mesh_list_[mesh].marked_for_deletion = 0;
      if (context_struct_.mesh_list_first_free_ > mesh)
        {
          context_struct_.mesh_list_[mesh].next_free = context_struct_.mesh_list_first_free_;
          context_struct_.mesh_list_first_free_ = mesh;
        }
      else
        {
          int lastf = context_struct_.mesh_list_first_free_;
          int nextf = context_struct_.mesh_list_[lastf].next_free;
          while (nextf < mesh)
            {
              lastf = nextf;
              nextf = context_struct_.mesh_list_[lastf].next_free;
            }
          context_struct_.mesh_list_[lastf].next_free = mesh;
          context_struct_.mesh_list_[mesh].next_free = nextf;
        }
    }
}

/*!
 * This function sets the direction of light. If it is called with (0, 0, 0),
 * the light is always pointing into the same direction as the camera.
 * \param [in] x The x-component of the light's direction
 * \param [in] y The y-component of the light's direction
 * \param [in] z The z-component of the light's direction
 *
 */
GR3API void gr3_setlightdirection(float x, float y, float z)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return;
  if (!context_struct_.is_initialized)
    {
      return;
    }
  context_struct_.light_dir[0] = x;
  context_struct_.light_dir[1] = y;
  context_struct_.light_dir[2] = z;
}

/*!
 * This function sets the view matrix by getting the position of the camera, the
 * position of the center of focus and the direction which should point up. This
 * function takes effect when the next image is created. Therefore if you want
 * to take pictures of the same data from different perspectives, you can call
 * and  gr3_cameralookat(), gr3_getpixmap_(), gr3_cameralookat(),
 * gr3_getpixmap_(), ... without calling gr3_clear() and gr3_drawmesh() again.
 * \param [in] camera_x The x-coordinate of the camera
 * \param [in] camera_y The y-coordinate of the camera
 * \param [in] camera_z The z-coordinate of the camera
 * \param [in] center_x The x-coordinate of the center of focus
 * \param [in] center_y The y-coordinate of the center of focus
 * \param [in] center_z The z-coordinate of the center of focus
 * \param [in] up_x The x-component of the up direction
 * \param [in] up_y The y-component of the up direction
 * \param [in] up_z The z-component of the up direction
 * \note Source: http://www.opengl.org/sdk/docs/man/xhtml/gluLookAt.xml
 * (as of 10/24/2011, licensed under SGI Free Software Licence B)
 */
GR3API void gr3_cameralookat(float camera_x, float camera_y, float camera_z, float center_x, float center_y,
                             float center_z, float up_x, float up_y, float up_z)
{
  int i, j;
  GLfloat view_matrix[4][4] = {{0}};
  GLfloat camera_pos[3];
  GLfloat center_pos[3];
  GLfloat up_dir[3];

  GLfloat F[3];
  GLfloat f[3];
  GLfloat up[3];
  GLfloat s[3];
  GLfloat u[3];
  GLfloat tmp;
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return;

  if (!context_struct_.is_initialized)
    {
      return;
    }
  context_struct_.camera_x = camera_x;
  context_struct_.camera_y = camera_y;
  context_struct_.camera_z = camera_z;
  context_struct_.center_x = center_x;
  context_struct_.center_y = center_y;
  context_struct_.center_z = center_z;
  context_struct_.up_x = up_x;
  context_struct_.up_y = up_y;
  context_struct_.up_z = up_z;
  camera_pos[0] = camera_x;
  camera_pos[1] = camera_y;
  camera_pos[2] = camera_z;
  center_pos[0] = center_x;
  center_pos[1] = center_y;
  center_pos[2] = center_z;
  up_dir[0] = up_x;
  up_dir[1] = up_y;
  up_dir[2] = up_z;


  for (i = 0; i < 3; i++)
    {
      F[i] = center_pos[i] - camera_pos[i];
    }
  /* f = normalize(F); */
  tmp = 0;
  for (i = 0; i < 3; i++)
    {
      tmp += F[i] * F[i];
    }
  tmp = sqrt(tmp);
  for (i = 0; i < 3; i++)
    {
      f[i] = F[i] / tmp;
    }
  /* up = normalize(up_dir); */
  tmp = 0;
  for (i = 0; i < 3; i++)
    {
      tmp += up_dir[i] * up_dir[i];
    }
  tmp = sqrt(tmp);
  for (i = 0; i < 3; i++)
    {
      up[i] = up_dir[i] / tmp;
    }
  /* s = cross(f,up); */
  for (i = 0; i < 3; i++)
    {
      s[i] = f[(i + 1) % 3] * up[(i + 2) % 3] - up[(i + 1) % 3] * f[(i + 2) % 3];
    }
  /* s = normalize(s); */
  tmp = 0;
  for (i = 0; i < 3; i++)
    {
      tmp += s[i] * s[i];
    }
  tmp = sqrt(tmp);
  for (i = 0; i < 3; i++)
    {
      s[i] = s[i] / tmp;
    }
  /* u = cross(s,f); */
  for (i = 0; i < 3; i++)
    {
      u[i] = s[(i + 1) % 3] * f[(i + 2) % 3] - f[(i + 1) % 3] * s[(i + 2) % 3];
    }

  /* u = normalize(u); */
  tmp = 0;
  for (i = 0; i < 3; i++)
    {
      tmp += u[i] * u[i];
    }
  tmp = sqrt(tmp);
  for (i = 0; i < 3; i++)
    {
      u[i] = u[i] / tmp;
    }
  for (i = 0; i < 3; i++)
    {
      view_matrix[i][0] = s[i];
      view_matrix[i][1] = u[i];
      view_matrix[i][2] = -f[i];
    }
  view_matrix[3][3] = 1;
  for (i = 0; i < 3; i++)
    {
      view_matrix[3][i] = 0;
      for (j = 0; j < 3; j++)
        {
          view_matrix[3][i] -= view_matrix[j][i] * camera_pos[j];
        }
    }
  memmove(&context_struct_.view_matrix[0][0], &view_matrix[0][0], sizeof(view_matrix));
}

/*!
 * This function sets the projection parameters. This function takes effect
 * when the next image is created.
 * \param [in] vertical_field_of_view   This parameter is the vertical field of
 *                                      view in degrees. It must be greater than
 *                                      0 and less than 180.
 * \param [in] zNear                    The distance to the near clipping plane.
 * \param [in] zFar                     The distance to the far clipping plane.
 * \returns
 * - ::GR3_ERROR_NONE                   on success
 * - ::GR3_ERROR_INVALID_VALUE          if one (or more) of the arguments is out of its range
 * \note The ratio between zFar and zNear influences the precision of the depth
 * buffer, the greater \f$ \frac{zFar}{zNear} \f$, the more likely are errors. So
 * you should try to keep both values as close to each other as possible while
 * making sure everything you want to be visible, is visible.
 */
GR3API int gr3_setcameraprojectionparameters(float vertical_field_of_view, float zNear, float zFar)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }

  if (zFar < zNear || zNear <= 0 || vertical_field_of_view >= 180 || vertical_field_of_view <= 0)
    {
      RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
    }
  context_struct_.vertical_field_of_view = vertical_field_of_view;
  context_struct_.zNear = zNear;
  context_struct_.zFar = zFar;
  RETURN_ERROR(GR3_ERROR_NONE);
}

/*!
 * Get the projection parameters.
 *
 * \param [out] vfov   Vertical field of view in degrees
 * \param [out] znear  The distance to the near clipping plane.
 * \param [out] zfar   The distance to the far clipping plane.
 *
 * \returns
 * - ::GR3_ERROR_NONE  on success
 */
GR3API int gr3_getcameraprojectionparameters(float *vfov, float *znear, float *zfar)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (!context_struct_.is_initialized)
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }

  *vfov = context_struct_.vertical_field_of_view;
  *znear = context_struct_.zNear;
  *zfar = context_struct_.zFar;

  RETURN_ERROR(GR3_ERROR_NONE);
}

/*!
 * Create a parallel, orthographic or perspective projection
 */
static void gr3_projectionmatrix_(float left, float right, float bottom, float top, float znear, float zfar,
                                  GLfloat *matrix)
{
  memset(matrix, 0, 16 * sizeof(GLfloat));
  if (context_struct_.projection_type == GR3_PROJECTION_PARALLEL ||
      context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC)
    {
      /* Source: http://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml */
      matrix[0 + 0 * 4] = 2.0 / (right - left);
      matrix[0 + 3 * 4] = -(right + left) / (right - left);
      matrix[1 + 1 * 4] = 2.0 / (top - bottom);
      matrix[1 + 3 * 4] = -(top + bottom) / (top - bottom);
      matrix[2 + 2 * 4] = -2.0 / (zfar - znear);
      matrix[2 + 3 * 4] = -(zfar + znear) / (zfar - znear);
      matrix[3 + 3 * 4] = 1.0;
    }
  else
    {
      /* Source: http://www.opengl.org/sdk/docs/man2/xhtml/glFrustum.xml */
      matrix[0 + 0 * 4] = 2.0 * znear / (right - left);
      matrix[0 + 2 * 4] = (right + left) / (right - left);
      matrix[1 + 1 * 4] = 2.0 * znear / (top - bottom);
      matrix[1 + 2 * 4] = (top + bottom) / (top - bottom);
      matrix[2 + 2 * 4] = -(zfar + znear) / (zfar - znear);
      matrix[2 + 3 * 4] = -2.0 * zfar * znear / (zfar - znear);
      matrix[3 + 2 * 4] = -1.0;
    }
}

/*!
 * This function iterates over the draw list and draws the image using OpenGL.
 */
static void gr3_draw_(GLuint width, GLuint height)
{
#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glUseProgram(context_struct_.program);
    }
#endif
  gr3_log_("gr3_draw_();");
  {
    GLfloat projection_matrix[4][4] = {{0}};
    GLfloat *pm;
    if (context_struct_.projection_matrix != NULL)
      {
        pm = context_struct_.projection_matrix;
      }
    else
      {
        if (context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC)
          {
            gr3_projectionmatrix_(context_struct_.left, context_struct_.right, context_struct_.bottom,
                                  context_struct_.top, context_struct_.zNear, context_struct_.zFar,
                                  &(projection_matrix[0][0]));
          }
        else
          {
            GLfloat fovy = context_struct_.vertical_field_of_view;
            GLfloat zNear = context_struct_.zNear;
            GLfloat zFar = context_struct_.zFar;
            GLfloat aspect = (GLfloat)width / height;
            GLfloat tfov2 = tan(fovy * M_PI / 360.0);
            GLfloat right = zNear * aspect * tfov2;
            GLfloat top = zNear * tfov2;
            gr3_projectionmatrix_(-right, right, -top, top, zNear, zFar, &(projection_matrix[0][0]));
          }
        pm = &projection_matrix[0][0];
      }
#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniformMatrix4fv(glGetUniformLocation(context_struct_.program, "ProjectionMatrix"), 1, GL_FALSE, pm);
      }
    else
#else
    (void)pm;
#endif
      {
#ifndef NO_GL
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(pm);
#endif
      }

#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniformMatrix4fv(glGetUniformLocation(context_struct_.program, "ViewMatrix"), 1, GL_FALSE,
                           &(context_struct_.view_matrix[0][0]));
      }
    else
#endif
      {
#ifndef NO_GL
        glMatrixMode(GL_MODELVIEW);
        if (context_struct_.light_dir[0] == 0 && context_struct_.light_dir[1] == 0 && context_struct_.light_dir[2] == 0)
          {
            GLfloat def[4] = {0, 0, 1, 0};
            glLoadIdentity();
            glLightfv(GL_LIGHT0, GL_POSITION, &def[0]);
          }
        glLoadMatrixf(&(context_struct_.view_matrix[0][0]));
#endif
      }
#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniform3f(glGetUniformLocation(context_struct_.program, "LightDirection"), context_struct_.light_dir[0],
                    context_struct_.light_dir[1], context_struct_.light_dir[2]);
      }
#endif
  }
#ifndef NO_GL
  glEnable(GL_NORMALIZE);
  if (!context_struct_.use_vbo)
    {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      if (context_struct_.light_dir[0] != 0 || context_struct_.light_dir[1] != 0 || context_struct_.light_dir[2] != 0)
        {
          glLightfv(GL_LIGHT0, GL_POSITION, &context_struct_.light_dir[0]);
        }
    }
  /* The depth test should already be enabled, but it is re-enabled here,
   * as the user might have disabled it when GR3 is using an already existing
   * OpenGL context, e.g. when using an QOpenGLWidget in Qt5.
   */
  glEnable(GL_DEPTH_TEST);
  glClearColor(context_struct_.background_color[0], context_struct_.background_color[1],
               context_struct_.background_color[2], context_struct_.background_color[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  {
    GR3_DrawList_t_ *draw;
    draw = context_struct_.draw_list_;
    while (draw)
      {
        gr3_dodrawmesh_(draw->mesh, draw->n, draw->positions, draw->directions, draw->ups, draw->colors, draw->scales);
        draw = draw->next;
      }
  }
#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glUseProgram(0);
    }
#endif
#endif
}

GR3API int gr3_drawimage(float xmin, float xmax, float ymin, float ymax, int width, int height, int drawable_type)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  switch (drawable_type)
    {
    case GR3_DRAWABLE_OPENGL:
      return gr3_drawimage_opengl_(xmin, xmax, ymin, ymax, width, height);
    case GR3_DRAWABLE_GKS:
      return gr3_drawimage_gks_(xmin, xmax, ymin, ymax, width, height);
    default:
      RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
    }
}

static int gr3_drawimage_opengl_(float xmin, float xmax, float ymin, float ymax, int width, int height)
{
  (void)xmin;
  (void)xmax;
  (void)ymin;
  (void)ymax;
  gr3_log_("gr3_drawimage_opengl_();");
#ifndef NO_GL
#if GL_ARB_framebuffer_object
  glBindFramebuffer(GL_FRAMEBUFFER, user_framebuffer);
#else
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, user_framebuffer);
#endif
  glViewport(xmin, ymin, xmax - xmin, ymax - ymin);
#endif
  gr3_draw_(width, height);
  RETURN_ERROR(GR3_ERROR_NONE);
}


static int gr3_strendswith_(const char *str, const char *ending)
{
  int str_len = strlen(str);
  int ending_len = strlen(ending);
  return (str_len >= ending_len) && !strcmp(str + str_len - ending_len, ending);
}

GR3API int gr3_setquality(int quality)
{
  int ssaa_factor = quality & ~1;
  int i;
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (quality > 33 || quality < 0)
    {
      RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
    }
  if (ssaa_factor == 0) ssaa_factor = 1;
  i = ssaa_factor;
  while (i / 2 * 2 == i)
    {
      i = i / 2;
    }
  if (i != 1)
    {
      RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
    }
  context_struct_.quality = quality;
  RETURN_ERROR(GR3_ERROR_NONE);
}

GR3API int gr3_getimage(int width, int height, int use_alpha, char *pixels)
{
  int err;
  int quality = context_struct_.quality;
  int ssaa_factor = quality & ~1;
  int use_povray = quality & 1;
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  if (ssaa_factor == 0) ssaa_factor = 1;
  if (use_povray)
    {
      err = gr3_getpovray_(pixels, width, height, use_alpha, ssaa_factor);
    }
  else
    {
      err = gr3_getpixmap_(pixels, width, height, use_alpha, ssaa_factor);
    }
  return err;
}

GR3API int gr3_export(const char *filename, int width, int height)
{
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);
  gr3_log_(filename);

  if (gr3_strendswith_(filename, ".html"))
    {
      gr3_log_("export as html file");
      return gr3_export_html_(filename, width, height);
    }
  else if (gr3_strendswith_(filename, ".pov"))
    {
      gr3_log_("export as pov file");
      return gr3_export_pov_(filename, width, height);
    }
  else if (gr3_strendswith_(filename, ".png"))
    {
      gr3_log_("export as png file");
      return gr3_export_png_(filename, width, height);
    }
  else if (gr3_strendswith_(filename, ".jpg") || gr3_strendswith_(filename, ".jpeg"))
    {
      gr3_log_("export as jpeg file");
      return gr3_export_jpeg_(filename, width, height);
    }
  RETURN_ERROR(GR3_ERROR_UNKNOWN_FILE_EXTENSION);
}


/*!
 * This function fills a bitmap of the given size (width x height) with the
 * image created by gr3.
 * \param [in] pixmap       The pixmap that the function has to fill
 * \param [in] width        The width of the bitmap
 * \param [in] height       The height of the bitmap
 * \param [in] use_alpha    Whether to include an alpha component in the pixmap
 * \param [in] ssaa_factor  The factor to use for supersampling antialiasing
 * \returns
 * - ::GR3_ERROR_NONE                     on success
 * - ::GR3_ERROR_NOT_INITIALIZED          if gr3 has not been initialized
 * - ::GR3_ERROR_OPENGL_ERR               if an OpenGL error occured
 * - ::GR3_ERROR_CAMERA_NOT_INITIALIZED   if the camera has not been initialized
 * \note The memory bitmap points to must be \f$sizeof(int) \cdot width
 * \cdot height\f$ bytes in size, so the whole image can be stored.
 */
static int gr3_getpixmap_(char *pixmap, int width, int height, int use_alpha, int ssaa_factor)
{
  int x, y;
  int fb_width, fb_height;
  int dx, dy;
  int x_patches, y_patches;
  int view_matrix_all_zeros;
  char *raw_pixels = NULL;

#ifndef NO_GL
  GLenum format = use_alpha ? GL_RGBA : GL_RGB;
#endif
  int bpp = use_alpha ? 4 : 3;
  GLfloat zNear = context_struct_.zNear;
  GLfloat zFar = context_struct_.zFar;
  GLfloat left, right, bottom, top;

  if (context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC)
    {
      double aspect = 1.0 * width / height;
      right = context_struct_.right;
      left = context_struct_.left;
      bottom = context_struct_.bottom;
      top = context_struct_.top;
      if (aspect > 1)
        {
          right *= aspect;
          left *= aspect;
        }
      else
        {
          top /= aspect;
          bottom /= aspect;
        }
    }
  else
    {
      GLfloat fovy = context_struct_.vertical_field_of_view;
      GLfloat tan_halffovy = tan(fovy * M_PI / 360.0);
      GLfloat aspect = (GLfloat)width / height;
      if (context_struct_.projection_type == GR3_PROJECTION_PARALLEL && context_struct_.aspect_override > 0)
        {
          aspect = context_struct_.aspect_override;
        }

      right = zNear * tan_halffovy * aspect;
      left = -right;
      top = zNear * tan_halffovy;
      bottom = -top;
    }

  if (context_struct_.is_initialized)
    {
      if (width == 0 || height == 0 || pixmap == NULL)
        {
          RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
        }
      view_matrix_all_zeros = 1;
      for (x = 0; x < 4; x++)
        {
          for (y = 0; y < 4; y++)
            {
              if (context_struct_.view_matrix[x][y] != 0)
                {
                  view_matrix_all_zeros = 0;
                }
            }
        }
      if (view_matrix_all_zeros)
        {
          /* gr3_cameralookat has not been called */
          RETURN_ERROR(GR3_ERROR_CAMERA_NOT_INITIALIZED);
        }
      if ((context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC &&
           (is_nan(context_struct_.left) || is_nan(context_struct_.right) || is_nan(context_struct_.bottom) ||
            is_nan(context_struct_.top) || context_struct_.zFar < context_struct_.zNear)) ||
          (context_struct_.projection_type != GR3_PROJECTION_ORTHOGRAPHIC &&
           (context_struct_.zFar < context_struct_.zNear || context_struct_.zNear <= 0 ||
            context_struct_.vertical_field_of_view >= 180 || context_struct_.vertical_field_of_view <= 0)))
        {
          /* gr3_setcameraprojectionparameters or gr3_setorthographicprojection has not been called */
          RETURN_ERROR(GR3_ERROR_CAMERA_NOT_INITIALIZED);
        }

      if (context_struct_.use_software_renderer == 1)
        {
          gr3_getpixmap_softwarerendered(pixmap, width, height, ssaa_factor);
          return (GR3_ERROR_NONE);
        }
#ifndef NO_GL
      fb_width = context_struct_.init_struct.framebuffer_width;
      fb_height = context_struct_.init_struct.framebuffer_height;
      if (ssaa_factor != 1)
        {
          raw_pixels = malloc((size_t)fb_width * fb_height * ssaa_factor * ssaa_factor * bpp);
          if (!raw_pixels)
            {
              RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
            }
          width = width * ssaa_factor;
          height = height * ssaa_factor;
        }

#if GL_ARB_framebuffer_object
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
#else
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
#endif

      x_patches = width / fb_width + (width / fb_width * fb_width < width);
      y_patches = height / fb_height + (height / fb_height * fb_height < height);
      for (y = 0; y < y_patches; y++)
        {
          for (x = 0; x < x_patches; x++)
            {
              if ((x + 1) * fb_width <= width)
                {
                  dx = fb_width;
                }
              else
                {
                  dx = width - fb_width * x;
                }
              if ((y + 1) * fb_height <= height)
                {
                  dy = fb_height;
                }
              else
                {
                  dy = height - fb_height * y;
                }
              {
                GLfloat projection_matrix[4][4] = {{0}};
                GLfloat l = left + 1.0f * (right - left) * (x * fb_width) / width;
                GLfloat r = left + 1.0f * (right - left) * (x * fb_width + dx) / width;
                GLfloat b = bottom + 1.0f * (top - bottom) * (y * fb_height) / height;
                GLfloat t = bottom + 1.0f * (top - bottom) * (y * fb_height + dy) / height;

                gr3_projectionmatrix_(l, r, b, t, zNear, zFar, &(projection_matrix[0][0]));

                context_struct_.projection_matrix = &projection_matrix[0][0];
                glViewport(0, 0, dx, dy);
                gr3_draw_(width, height);
                context_struct_.projection_matrix = NULL;
              }
              glPixelStorei(GL_PACK_ALIGNMENT, 1); /* byte-wise alignment */
              if (ssaa_factor == 1)
                {
#if defined(GR3_USE_WIN) || defined(GR3_USE_GLX)
                  /* There seems to be a driver error on windows considering
                     GL_PACK_ROW_LENGTH, so I have to roll my own loop to
                     read the pixels row-wise instead of copying whole
                     images.
                     GLX seems to have the same problem sometimes.
                     The pixels of the image are not skipped,
                     but filled with garbage instead.
                  */
                  {
                    int i;
                    for (i = 0; i < dy; i++)
                      {
                        glReadPixels(0, i, dx, 1, format, GL_UNSIGNED_BYTE,
                                     pixmap + bpp * (y * width * fb_height + i * width + x * fb_width));
                      }
                  }
#else
                  /* On other systems, GL_PACK_ROW_LENGTH works fine. */
                  glPixelStorei(GL_PACK_ROW_LENGTH, width);
                  glReadPixels(0, 0, dx, dy, format, GL_UNSIGNED_BYTE,
                               pixmap + bpp * (y * width * fb_height + x * fb_width));
#endif
                }
              else
                {
#if defined(GR3_USE_WIN) || defined(GR3_USE_GLX)
                  {
                    int i;
                    for (i = 0; i < dy; i++)
                      {
                        glReadPixels(0, i, dx, 1, format, GL_UNSIGNED_BYTE, raw_pixels + i * fb_width * bpp);
                      }
                  }
#else
                  glPixelStorei(GL_PACK_ROW_LENGTH, fb_width);
                  glReadPixels(0, 0, dx, dy, format, GL_UNSIGNED_BYTE, raw_pixels);
#endif
                  {
                    int i, j, k, l, m, v, c;
                    for (i = 0; i < dx / ssaa_factor; i++)
                      {
                        for (j = 0; j < dy / ssaa_factor; j++)
                          {
                            for (l = 0; l < bpp; l++)
                              {
                                v = 0;
                                c = 0;
                                for (k = 0; k < ssaa_factor; k++)
                                  {
                                    for (m = 0; m < ssaa_factor; m++)
                                      {
                                        if ((ssaa_factor * i + k < dx) && (ssaa_factor * j + m < dy))
                                          {
                                            v += (unsigned char)raw_pixels[bpp * ((ssaa_factor * i + k) +
                                                                                  (ssaa_factor * j + m) * fb_width) +
                                                                           l];
                                            c++;
                                          }
                                      }
                                  }
                                v = v / c;
                                pixmap[bpp * (y * fb_height / ssaa_factor * width / ssaa_factor +
                                              x * fb_width / ssaa_factor + i + j * width / ssaa_factor) +
                                       l] = v;
                              }
                          }
                      }
                  }
                }
            }
        }
      if (ssaa_factor != 1)
        {
          free(raw_pixels);
        }
      if (glGetError() == GL_NO_ERROR)
        {
          RETURN_ERROR(GR3_ERROR_NONE);
        }
      else
        {
          RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
        }
#else
      (void)bottom;
      (void)top;
      (void)left;
      (void)zFar;
      (void)bpp;
      (void)raw_pixels;
      (void)x_patches;
      (void)y_patches;
      (void)dy;
      (void)dx;
      (void)fb_width;
      (void)fb_height;
#endif
    }
  else
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }
}

/*!
 * This function is used by other gr3 functions to provide the user with debug
 * information. If the user has set a logging function with
 * gr3_setlogcallback(), this function will be called. Otherwise logging
 * messages will be ignored.
 * \param [in] log_message  The logging message
 */
void gr3_log_(const char *log_message)
{
  char *debug;
  debug = getenv("GR3_DEBUG");
  if (debug != NULL && debug[0] != 0)
    {
      fprintf(stderr, "gr3: %s\n", log_message);
    }
  if (gr3_log_func_)
    {
      gr3_log_func_(log_message);
    }
}

GR3API void gr3_free(void *pointer)
{
  free(pointer);
}

/*!
 * During software development it will often be helpful to get debug output
 * from gr3. This information is not printed, but reported directly to the
 * user by calling a logging callback. This function allows to set this
 * callback or disable it again by calling it with NULL.
 * \param [in] gr3_log_func The logging callback, a function which gets a const
 *                          char pointer as its only argument and returns
 *                          nothing.
 */
GR3API void gr3_setlogcallback(void (*gr3_log_func)(const char *log_message))
{
  gr3_log_func_ = gr3_log_func;
}

/*!
 * This array holds string representations of the different gr3 error codes as
 * defined in ::int. The elements of this array will be returned by
 * gr3_geterrorstring. The last element should always be "kEUnknownError" in
 * case an unkown error code is requested.
 */
static char *error_strings_[] = {"GR3_ERROR_NONE",
                                 "GR3_ERROR_INVALID_VALUE",
                                 "GR3_ERROR_INVALID_ATTRIBUTE",
                                 "GR3_ERROR_INIT_FAILED",
                                 "GR3_ERROR_OPENGL_ERR",
                                 "GR3_ERROR_OUT_OF_MEM",
                                 "GR3_ERROR_NOT_INITIALIZED",
                                 "GR3_ERROR_CAMERA_NOT_INITIALIZED",
                                 "GR3_ERROR_UNKNOWN_FILE_EXTENSION",
                                 "GR3_ERROR_CANNOT_OPEN_FILE",
                                 "GR3_ERROR_EXPORT",
                                 "GR3_ERROR_UNKNOWN"};

/*!
 * This function returns a string representation of a given error code.
 * \param [in] error    The error code whose represenation will be returned.
 */
GR3API const char *gr3_geterrorstring(int error)
{
  int num_errors = sizeof(error_strings_) / sizeof(char *) - 1;
  if (error >= num_errors) error = num_errors;
  return error_strings_[error];
}

/*!
 * This function allows the user to find out how his commands are rendered.
 * \returns If gr3 is initialized, a string in the format:
 *          "gr3 - " + window toolkit + " - " + framebuffer extension + " - "
 *          + OpenGL version + " - " + OpenGL renderer string
 *          For example "gr3 - GLX - GL_ARB_framebuffer_object - 2.1 Mesa
 *          7.10.2 - Software Rasterizer" might be returned on a Linux system
 *          (using GLX) with an available GL_ARB_framebuffer_object
 *          implementation.
 *          If gr3 is not initialized "Not initialized" is returned.
 */
GR3API const char *gr3_getrenderpathstring(void)
{
  GR3_DO_INIT;
  return context_struct_.renderpath_string;
}

/*!
 * This function appends a string to the renderpath string returned by
 * gr3_getrenderpathstring().
 * \param [in] string The string to append
 */
void gr3_appendtorenderpathstring_(const char *string)
{
  char *tmp = malloc(strlen(context_struct_.renderpath_string) + 3 + strlen(string) + 1);
  strcpy(tmp, context_struct_.renderpath_string);
  strcpy(tmp + strlen(context_struct_.renderpath_string), " - ");
  strcpy(tmp + strlen(context_struct_.renderpath_string) + 3, string);
  if (context_struct_.renderpath_string != not_initialized_)
    {
      free(context_struct_.renderpath_string);
    }
  context_struct_.renderpath_string = tmp;
}

/*!
 * The id of the renderbuffer object used for color data.
 */
static GLuint color_renderbuffer = 0;

/*!
 * The id of the renderbuffer object used for depth data.
 */
static GLuint depth_renderbuffer = 0;

#if GL_ARB_framebuffer_object
/* Framebuffer Object using OpenGL 3.0 or GL_ARB_framebuffer_object */
/*!
 * This function implements Framebuffer creation using the
 * ARB_framebuffer_object extension.
 * \returns
 * - ::GR3_ERROR_NONE          on success
 * - ::GR3_ERROR_OPENGL_ERR    if an OpenGL error occurs
 */
static int gr3_initFBO_ARB_(void)
{
  GLenum framebuffer_status;
  GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
  GLuint _width = context_struct_.init_struct.framebuffer_width;
  GLuint _height = context_struct_.init_struct.framebuffer_height;

  gr3_log_("gr3_initFBO_ARB_();");

  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glGenRenderbuffers(1, &color_renderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, _width, _height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer);

  glGenRenderbuffers(1, &depth_renderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);

  glDrawBuffers(1, draw_buffers);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
    {
      gr3_log_("failed to create an ARB framebuffer object (fbo wasn't complete)");
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }
  glViewport(0, 0, _width, _height);
  glEnable(GL_DEPTH_TEST);
  if (glGetError() != GL_NO_ERROR)
    {
      gr3_terminateFBO_ARB_();
      gr3_log_("failed to create an ARB framebuffer object (an OpenGL error occurred)");
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }

  context_struct_.terminateFBO = gr3_terminateFBO_ARB_;
  context_struct_.fbo_is_initialized = 1;
  gr3_appendtorenderpathstring_("GL_ARB_framebuffer_object");
  RETURN_ERROR(GR3_ERROR_NONE);
}

/*!
 * This function destroys the Framebuffer Object using the
 * ARB_framebuffer_object extension.
 */
static void gr3_terminateFBO_ARB_(void)
{
  gr3_log_("gr3_terminateFBO_ARB_();");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDeleteFramebuffers(1, &framebuffer);
  glDeleteRenderbuffers(1, &color_renderbuffer);
  glDeleteRenderbuffers(1, &depth_renderbuffer);

  context_struct_.fbo_is_initialized = 0;
}
#endif


#if GL_EXT_framebuffer_object
/* Framebuffer Object using GL_EXT_framebuffer_object */
/*!
 * This function implements Framebuffer creation using the
 * EXT_framebuffer_object extension.
 * \returns
 * - ::GR3_ERROR_NONE          on success
 * - ::GR3_ERROR_OPENGL_ERR    if an OpenGL error occurs
 */
static int gr3_initFBO_EXT_(void)
{
#ifdef FRAMEBUFFER_STATUS
  GLenum framebuffer_status;
#endif
  GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0_EXT};
  GLuint _width = context_struct_.init_struct.framebuffer_width;
  GLuint _height = context_struct_.init_struct.framebuffer_height;

  gr3_log_("gr3_initFBO_EXT_();");

  glGenFramebuffersEXT(1, &framebuffer);
  gr3_log_("glGenFramebuffersEXT");
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
  gr3_log_("glBindFramebufferEXT");
  glGenRenderbuffersEXT(1, &color_renderbuffer);
  gr3_log_("glGenRenderbuffersEXT");
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, color_renderbuffer);
  gr3_log_("glBindRenderbufferEXT");
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, _width, _height);
  gr3_log_("glRenderbufferStorageEXT");
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, color_renderbuffer);
  gr3_log_("glFramebufferRenderbufferEXT");
  glGenRenderbuffersEXT(2, &depth_renderbuffer);
  gr3_log_("glGenRenderbuffersEXT");
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_renderbuffer);
  gr3_log_("glBindRenderbufferEXT");
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, _width, _height);
  gr3_log_("glRenderbufferStorageEXT");
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_renderbuffer);
  gr3_log_("glFramebufferRenderbufferEXT");

  glDrawBuffers(1, draw_buffers);
  gr3_log_("glDrawBuffers");
  glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
  gr3_log_("glReadBuffer");
#ifdef FRAMEBUFFER_STATUS
  framebuffer_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  gr3_log_("glCheckFramebufferStatusEXT");
  if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
      gr3_log_("failed to create an EXT framebuffer object (fbo wasn't complete)");
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }
#endif
  glViewport(0, 0, _width, _height);
  gr3_log_("glViewport");
  glEnable(GL_DEPTH_TEST);
  gr3_log_("glEnable");
  if (glGetError() != GL_NO_ERROR)
    {
      gr3_terminateFBO_EXT_();
      gr3_log_("failed to create an EXT framebuffer object (an OpenGL error occurred)");
      RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
    }

  context_struct_.terminateFBO = gr3_terminateFBO_EXT_;
  context_struct_.fbo_is_initialized = 1;
  gr3_appendtorenderpathstring_("GL_EXT_framebuffer_object");
  RETURN_ERROR(GR3_ERROR_NONE);
}

/*!
 * This function destroys the Framebuffer Object using the
 * EXT_framebuffer_object extension.
 */
static void gr3_terminateFBO_EXT_(void)
{
  gr3_log_("gr3_terminateFBO_EXT_();");

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  glDeleteFramebuffersEXT(1, &framebuffer);
  glDeleteRenderbuffersEXT(1, &color_renderbuffer);
  glDeleteRenderbuffersEXT(1, &depth_renderbuffer);

  context_struct_.fbo_is_initialized = 0;
}
#endif


GR3API void gr3_setobjectid(int id)
{
  GR3_DO_INIT;
  current_object_id = id;
}

static int gr3_selectiondraw_(int px, int py, GLuint width, GLuint height);
#ifndef NO_GL
GR3API int gr3_selectid(int px, int py, int width, int height, int *object_id)
{
  int x, y;
  int fb_width, fb_height;
  int dx, dy;
  int x_patches, y_patches;
  int view_matrix_all_zeros;

  GLfloat zNear = context_struct_.zNear;
  GLfloat zFar = context_struct_.zFar;
  GLfloat left, right, bottom, top;

  if (context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC)
    {
      left = context_struct_.left;
      right = context_struct_.right;
      bottom = context_struct_.bottom;
      top = context_struct_.top;
    }
  else
    {
      GLfloat fovy = context_struct_.vertical_field_of_view;
      GLfloat tan_halffovy = tan(fovy * M_PI / 360.0);
      GLfloat aspect = (GLfloat)width / height;
      right = zNear * tan_halffovy * aspect;
      left = -right;
      top = zNear * tan_halffovy;
      bottom = -top;
    }
  int id;
  GR3_DO_INIT;
  if (gr3_geterror(0, NULL, NULL)) return gr3_geterror(0, NULL, NULL);

  *object_id = 0;

  if (context_struct_.is_initialized)
    {
      if (width == 0 || height == 0)
        {
          RETURN_ERROR(GR3_ERROR_INVALID_VALUE);
        }
      view_matrix_all_zeros = 1;
      for (x = 0; x < 4; x++)
        {
          for (y = 0; y < 4; y++)
            {
              if (context_struct_.view_matrix[x][y] != 0)
                {
                  view_matrix_all_zeros = 0;
                }
            }
        }
      if (view_matrix_all_zeros)
        {
          /* gr3_cameralookat has not been called */
          RETURN_ERROR(GR3_ERROR_CAMERA_NOT_INITIALIZED);
        }
      if ((context_struct_.projection_type == GR3_PROJECTION_ORTHOGRAPHIC &&
           (is_nan(context_struct_.left) || is_nan(context_struct_.right) || is_nan(context_struct_.bottom) ||
            is_nan(context_struct_.top) || context_struct_.zFar < context_struct_.zNear)) ||
          (context_struct_.projection_type != GR3_PROJECTION_ORTHOGRAPHIC &&
           (context_struct_.zFar < context_struct_.zNear || context_struct_.zNear <= 0 ||
            context_struct_.vertical_field_of_view >= 180 || context_struct_.vertical_field_of_view <= 0)))
        {
          /* gr3_setcameraprojectionparameters or gr3_setorthographicprojection has not been called */
          RETURN_ERROR(GR3_ERROR_CAMERA_NOT_INITIALIZED);
        }

      fb_width = context_struct_.init_struct.framebuffer_width;
      fb_height = context_struct_.init_struct.framebuffer_height;

#if GL_ARB_framebuffer_object
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
#else
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
#endif

      x_patches = width / fb_width + (width / fb_width * fb_width < width);
      y_patches = height / fb_height + (height / fb_height * fb_height < height);
      for (y = 0; y < y_patches; y++)
        {
          for (x = 0; x < x_patches; x++)
            {
              if ((x + 1) * fb_width <= width)
                {
                  dx = fb_width;
                }
              else
                {
                  dx = width - fb_width * x;
                }
              if ((y + 1) * fb_height <= height)
                {
                  dy = fb_height;
                }
              else
                {
                  dy = height - fb_height * y;
                }
              if (px >= x * fb_width && px < x * fb_width + dx && py >= y * fb_height && py < y * fb_height + dy)
                {
                  {
                    GLfloat projection_matrix[4][4] = {{0}};
                    GLfloat l = left + 1.0f * (right - left) * (x * fb_width) / width;
                    GLfloat r = left + 1.0f * (right - left) * (x * fb_width + dx) / width;
                    GLfloat b = bottom + 1.0f * (top - bottom) * (y * fb_height) / height;
                    GLfloat t = bottom + 1.0f * (top - bottom) * (y * fb_height + dy) / height;

                    gr3_projectionmatrix_(l, r, b, t, zNear, zFar, &(projection_matrix[0][0]));
                    context_struct_.projection_matrix = &projection_matrix[0][0];
                    glViewport(0, 0, dx, dy);
                    id = gr3_selectiondraw_(px - x * fb_width, py - y * fb_height, width, height);
                    context_struct_.projection_matrix = NULL;
                    if (id != 0)
                      {
                        *object_id = id;
                      }
                  }
                }
            }
        }
      if (glGetError() == GL_NO_ERROR)
        {
          RETURN_ERROR(GR3_ERROR_NONE);
        }
      else
        {
          RETURN_ERROR(GR3_ERROR_OPENGL_ERR);
        }
    }
  else
    {
      RETURN_ERROR(GR3_ERROR_NOT_INITIALIZED);
    }
}
#endif

#ifndef NO_GL
static int gr3_selectiondraw_(int px, int py, GLuint width, GLuint height)
{
  int object_id = 0;
  unsigned int color;

#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glUseProgram(context_struct_.program);
    }
#endif
  gr3_log_("gr3_draw_();");
  {
    GLfloat projection_matrix[4][4] = {{0}};
    GLfloat *pm;
    if (context_struct_.projection_matrix != NULL)
      {
        pm = context_struct_.projection_matrix;
      }
    else
      {
        GLfloat fovy = context_struct_.vertical_field_of_view;
        GLfloat zNear = context_struct_.zNear;
        GLfloat zFar = context_struct_.zFar;


        {
          /* Source: http://www.opengl.org/sdk/docs/man/xhtml/gluPerspective.xml */
          GLfloat aspect = (GLfloat)width / height;
          GLfloat f = 1 / tan(fovy * M_PI / 360.0);
          projection_matrix[0][0] = f / aspect;
          projection_matrix[1][1] = f;
          projection_matrix[2][2] = (zFar + zNear) / (zNear - zFar);
          projection_matrix[3][2] = 2 * zFar * zNear / (zNear - zFar);
          projection_matrix[2][3] = -1;
        }
        pm = &projection_matrix[0][0];
      }
#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniformMatrix4fv(glGetUniformLocation(context_struct_.program, "ProjectionMatrix"), 1, GL_FALSE, pm);
      }
    else
#endif
      {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(pm);
      }

#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniformMatrix4fv(glGetUniformLocation(context_struct_.program, "ViewMatrix"), 1, GL_FALSE,
                           &(context_struct_.view_matrix[0][0]));
      }
    else
#endif
      {
        glMatrixMode(GL_MODELVIEW);
        if (context_struct_.light_dir[0] == 0 && context_struct_.light_dir[1] == 0 && context_struct_.light_dir[2] == 0)
          {
            GLfloat def[4] = {0, 0, 1, 0};
            glLoadIdentity();
            glLightfv(GL_LIGHT0, GL_POSITION, &def[0]);
          }
        glLoadMatrixf(&(context_struct_.view_matrix[0][0]));
      }
#ifdef GR3_CAN_USE_VBO
    if (context_struct_.use_vbo)
      {
        glUniform3f(glGetUniformLocation(context_struct_.program, "LightDirection"), context_struct_.light_dir[0],
                    context_struct_.light_dir[1], context_struct_.light_dir[2]);
      }
#endif
  }
  glEnable(GL_NORMALIZE);
  if (!context_struct_.use_vbo)
    {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      if (context_struct_.light_dir[0] != 0 || context_struct_.light_dir[1] != 0 || context_struct_.light_dir[2] != 0)
        {
          glLightfv(GL_LIGHT0, GL_POSITION, &context_struct_.light_dir[0]);
        }
    }
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  {
    GR3_DrawList_t_ *draw;
    draw = context_struct_.draw_list_;
    while (draw)
      {
        glClear(GL_COLOR_BUFFER_BIT);
        gr3_dodrawmesh_(draw->mesh, draw->n, draw->positions, draw->directions, draw->ups, draw->colors, draw->scales);
        color = 0;
        glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
        if (color != 0)
          {

            object_id = draw->object_id;
          }
        draw = draw->next;
      }
  }
#ifdef GR3_CAN_USE_VBO
  if (context_struct_.use_vbo)
    {
      glUseProgram(0);
    }
#endif
  return object_id;
}
#endif

/*!
 * \param [out] m the 4x4 column major view matrix
 */
GR3API void gr3_getviewmatrix(float *m)
{
  memmove(m, &context_struct_.view_matrix[0][0], 16 * sizeof(float));
}

/*!
 * \param [in] m the 4x4 column major view matrix
 */
GR3API void gr3_setviewmatrix(const float *m)
{
  memmove(&context_struct_.view_matrix[0][0], m, 16 * sizeof(float));
}

/*!
 * \returns the current projection type:
 * GR3_PROJECTION_PERSPECTIVE, GR3_PROJECTION_PARALLEL or GR3_PROJECTION_ORTHOGRAPHIC
 */
GR3API int gr3_getprojectiontype()
{
  return context_struct_.projection_type;
}

/*!
 * \param [in] type the new projection type:
 * GR3_PROJECTION_PERSPECTIVE, GR3_PROJECTION_PARALLEL or GR3_PROJECTION_ORTHOGRAPHIC
 */
GR3API void gr3_setprojectiontype(int type)
{
  if (type == GR3_PROJECTION_PARALLEL)
    {
      context_struct_.projection_type = GR3_PROJECTION_PARALLEL;
    }
  else if (type == GR3_PROJECTION_PERSPECTIVE)
    {
      context_struct_.projection_type = GR3_PROJECTION_PERSPECTIVE;
    }
  else if (type == GR3_PROJECTION_ORTHOGRAPHIC)
    {
      context_struct_.projection_type = GR3_PROJECTION_ORTHOGRAPHIC;
    }
}

/*!
 * Set parameters for orthographic transformation
 *
 * \param left xmin of the volume in worldcoordinates
 * \param right xmax of volume in worldcoordinates
 * \param bottom ymin of volume in worldcoordinates
 * \param top ymax of volume in worldcoordinates
 * \param near_plane distance to near clipping plane
 * \param far_plane distance to far clipping plane
 *
 * Switches projection type to orthographic
 */
GR3API void gr3_setorthographicprojection(float left, float right, float bottom, float top, float znear, float zfar)
{
  context_struct_.left = left;
  context_struct_.right = right;
  context_struct_.bottom = bottom;
  context_struct_.top = top;
  context_struct_.zNear = znear;
  context_struct_.zFar = zfar;

  gr3_setprojectiontype(GR3_PROJECTION_ORTHOGRAPHIC);
}
