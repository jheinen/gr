#ifndef GR3_INTERNALS_H_INCLUDED
#define GR3_INTERNALS_H_INCLUDED

#include "gr3_sr.h"
#ifndef M_PI
#define M_PI 3.141592653589793238462643383279
#endif

#if defined(EMSCRIPTEN)
#ifdef NO_GL
#define GR3_USE_SR
#define GLfloat float
#define GLuint unsigned int
#define GLint int
#else
#define GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#ifndef _MSC_VER
#include <unistd.h> /* for getpid() for tempfile names */
#else
#include <process.h>
#endif
#endif
#elif defined(__APPLE__)
/* Core OpenGL (CGL) on Mac OS X */
#define GR3_USE_CGL
#include "gr3_cgl.h"
#elif defined(__linux__) || defined(__FreeBSD__)
/* OpenGL Extension to the X Window System (GLX) on Linux */
#define GR3_USE_GLX
#include "gr3_glx.h"
#elif defined(_WIN32)
/* Windows */
#define GR3_USE_WIN
#include "gr3_win.h"
#else
#error "This operating system is currently not supported by gr3"
#endif

#define GR3_DO_INIT                        \
  do                                       \
    {                                      \
      if (!context_struct_.is_initialized) \
        {                                  \
          gr3_log_("auto-init");           \
          gr3_init(NULL);                  \
        }                                  \
    }                                      \
  while (0)

#ifndef DONT_USE_RETURN_ERROR
#define RETURN_ERROR(error) return _return_error_helper(error, __LINE__, __FILE__)
#endif


/*!
 * An instance of this struct contains all information from the
 * ::int list given to gr3_init() or the default values from
 * ::GR3_InitStruct_INITIALIZER.
 * \note Except for when gr3_init() is run, the only existing instance of this
 * struct should be the one in #context_struct_.
 */
typedef struct _GR3_InitStruct_t_
{
  int framebuffer_width;  /*!< The width of the framebuffer used for
                           generating images */
  int framebuffer_height; /*!< The height of the framebuffer used for
                           generating images */
  int num_threads;
} GR3_InitStruct_t_;

typedef enum _GR3_MeshType_t
{
  kMTNormalMesh,
  kMTIndexedMesh,
  kMTConeMesh,
  kMTSphereMesh,
  kMTCylinderMesh
} GR3_MeshType_t;

/*!
 * This union contains all information required for using a mesh. Either the
 * display list id, or the vertex buffer object id and number of vertices.
 * Instances of this unions are kept and
 * reference counted in the ::GR3_MeshList_t_.
 */
typedef struct _GR3_MeshData_t_
{
  GR3_MeshType_t type;
  union
  {
    int display_list_id; /*!< The OpenGL display list of the mesh. */
    unsigned int vertex_buffer_id;
    struct
    {
      unsigned int index_buffer_id;
      unsigned int vertex_buffer_id;
    } buffers;
  } data;
  float *vertices;
  float *normals;
  float *colors;
  int *indices;
  int number_of_vertices;
  int number_of_indices;
  vertex_fp *vertices_fp;
} GR3_MeshData_t_;


/*!
 * This struct is for managing ::GR3_MeshData_t_ objects.
 * Instances of this array are meant to be kept in an array so they can be
 * accessed by an index. Each instance refers to the next free instance in the
 * list, so when the user needs a new one, a free one can be found just by
 * indexing the array. They are reference counted and are 'free' if there are
 * no references left and _GR3_MeshList_t_::refcount reaches zero. A user can
 * only get one reference to the mesh by calling gr3_createmesh() and therefore
 * should only be able to release one by calling gr3_deletemesh(). This is
 * realized by saving a flag (whether a mesh is marked for deletion or not)
 * in _GR3_MeshList_t_::marked_for_deletion.
 * \note Instances of this struct should only appear in the #context_struct_.
 * \note Reference counting is handled by calling gr3_meshaddreference_() and
 * gr3_meshremovereference_().
 */
typedef struct _GR3_MeshList_t_
{
  GR3_MeshData_t_ data;    /*!<  The data of the actual mesh managed by this
                            struct. */
  int refcount;            /*!< A reference counter. If refcount reaches zero,
                            the instance is unused or 'free' and can be used
                            for a different mesh. */
  int marked_for_deletion; /*!< A flag whether the user called
                            gr3_deletemesh() for this mesh. */
  int next_free;           /*!< If refcount is zero (so this object is free),
                            next_free is the index of the next free object
                            in the array it is stored in. */
} GR3_MeshList_t_;

/*!
 * Each call to gr3_drawmesh() gets saved in an instance of this struct, so it
 * can be used for later drawing. They form a linked-list, with each draw call
 * pointing to the next one.
 */
typedef struct _GR3_DrawList_t_
{
  int mesh;          /*!< The id of the mesh that should be drawn */
  float *positions;  /*!< A list of 3 * _GR3_DrawList_t_::n floats.
                      Each triple is one position
                      for one mesh to be drawn. */
  float *directions; /*!< A list of 3 * _GR3_DrawList_t_::n floats.
                      Each triple is one (forward) direction
                      for one mesh to be drawn. */
  float *ups;        /*!< A list of 3 * _GR3_DrawList_t_::n floats.
                      Each triple is one up vector
                      for one mesh to be drawn. */
  float *colors;     /*!< A list of 3 * _GR3_DrawList_t_::n floats.
                      Each triple is one color
                      for one mesh to be drawn. */
  float *scales;     /*!< A list of 3 * _GR3_DrawList_t_::n floats.
                      Each triple means the scaling factors
                      for one mesh to be drawn. */
  int n;             /*!< The number of meshes to be drawn. */
  int object_id;
  vertex_fp **vertices_fp;
  struct _GR3_DrawList_t_ *next; /*!< The pointer to the next GR3_DrawList_t_. */
} GR3_DrawList_t_;

/*!
 * This struct holds all context data. All data that is dependent on gr3 to
 * be initialized is saved here. It is set up by gr3_init() and turned back
 * into its default state by gr3_terminate().
 * \note #context_struct_ should be the only instance of this struct.
 */
typedef struct _GR3_ContextStruct_t_
{
  GR3_InitStruct_t_ init_struct; /*!< The information given to gr3_init().
                                  \note This member and its members must not
                                  be changed outside of gr3_init() or
                                  gr3_terminate().
                                  \sa _GR3_InitStruct_t_ */

  int is_initialized; /*!< This flag is set to 1 if gr3 was
                       initialized successfully. */

  int gl_is_initialized; /*!< This flag is set to 1 if an OpenGL context
                          has been created successfully. */

  int convenience_is_initialized; /*!< This flag is set to 1 if the convenience
                                   layer has been initialized successfully. */

  void (*terminateGL)(void); /*!< This member holds a pointer to the
                              function which must be used for destroying
                              the OpenGL context.
                              \sa gr3_terminateGL_WIN_()
                              \sa gr3_terminateGL_GLX_()
                              \sa gr3_terminateGL_CGL_()
                              */

  int fbo_is_initialized; /*!< This flag is set to 1 if a framebuffer
                           object has been created successfully. */

  void (*terminateFBO)(void); /*!< This member holds a pointer to the
                               function which must be used for destroying
                               the framebuffer object.
                               \sa gr3_terminateFBO_ARB_()
                               \sa gr3_terminateFBO_EXT_()
                               */

  char *renderpath_string; /*!< This string holds information on the gr3
                            renderpath.
                            \sa gr3_getrenderpathstring()
                            \sa gr3_appendtorenderpathstring_()
                            */

  GR3_DrawList_t_ *draw_list_; /*!< This member holds a pointer to the first
                                element of the linked draw list or NULL. */

  GR3_MeshList_t_ *mesh_list_; /*!< This member holds a pointer to the mesh
                                list or NULL. */

  int mesh_list_first_free_; /*!< The index of the first free element
                              in the mesh list*/
  int mesh_list_capacity_;   /*!< The number of elements in the mesh
                              list */

  GLfloat view_matrix[4][4];    /*!< The view matrix used to transform vertices
                                 from world to eye space. */
  float vertical_field_of_view; /*!< The vertical field of view, used for the
                                 projection marix */
  float zNear;                  /*!< distance to the near clipping plane */
  float zFar;                   /*!< distance to the far clipping plane */
  float left;                   /*!< distance to the left plane of the orthographic projection */
  float right;                  /*!< distance to the right plane of the orthographic projection */
  float bottom;                 /*!< distance to the bottom plane of the orthographic projection */
  float top;                    /*!< distance to the top plane of the orthographic projection */

  GLfloat light_dir[4]; /*!< The direction of light + 0 for showing that it is
                         not a position, but a direction */
  int use_vbo;

  int cylinder_mesh;
  int sphere_mesh;
  int cone_mesh;
  int cube_mesh;
  GLfloat background_color[4];
  GLuint program;
  float camera_x;
  float camera_y;
  float camera_z;
  float center_x;
  float center_y;
  float center_z;
  float up_x;
  float up_y;
  float up_z;
  GLfloat *projection_matrix;
  int quality;
  int projection_type;
  int num_threads;
  int use_software_renderer;
  int option; /* cf. gr_surface_option_t in gr3_gr.c, used for the software renderer */
  int software_renderer_pixmaps_initalised;
  unsigned char *pixmaps[MAX_NUM_THREADS]; /* pixels to be drawn created by the Software Renderer */
  float *depth_buffers[MAX_NUM_THREADS];
#ifndef NO_THREADS
  pthread_t threads[MAX_NUM_THREADS];
#endif
  queue *queues[MAX_NUM_THREADS];
  int last_width;
  int last_height;
} GR3_ContextStruct_t_;

extern GR3_ContextStruct_t_ context_struct_;

extern int gr3_error_;
extern int gr3_error_line_;
extern const char *gr3_error_file_;

#ifndef DONT_USE_RETURN_ERROR
static int _return_error_helper(int error, int line, const char *file)
{
  if (error)
    {
      gr3_error_ = error;
      gr3_error_line_ = line;
      gr3_error_file_ = file;
    }
  return error;
}
#endif

void gr3_log_(const char *log_message);
void gr3_appendtorenderpathstring_(const char *string);
void gr3_init_convenience(void);
void gr3_terminate_convenience(void);
int gr3_export_html_(const char *filename, int width, int height);
int gr3_export_pov_(const char *filename, int width, int height);
int gr3_getpovray_(char *bitmap, int width, int height, int use_alpha, int ssaa_factor);
int gr3_export_png_(const char *filename, int width, int height);
int gr3_readpngtomemory_(int *pixels, const char *pngfile, int width, int height);
int gr3_export_jpeg_(const char *filename, int width, int height);
int gr3_drawimage_gks_(float xmin, float xmax, float ymin, float ymax, int width, int height);
void gr3_sortindexedmeshdata(int mesh);
#endif
