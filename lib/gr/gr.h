#ifndef _GR_H_
#define _GR_H_

#ifdef _WIN32

#define HAVE_BOOLEAN

#include <windows.h> /* required for all Windows applications */
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GR_PROJECTION_DEFAULT 0
#define GR_PROJECTION_ORTHOGRAPHIC 1
#define GR_PROJECTION_PERSPECTIVE 2

#define GR_VOLUME_WITHOUT_BORDER 0
#define GR_VOLUME_WITH_BORDER 1

#define GR_VOLUME_EMISSION 0
#define GR_VOLUME_ABSORPTION 1
#define GR_VOLUME_MIP 2

typedef struct
{
  double x, y;
} vertex_t;

DLLEXPORT void gr_initgr(void);
DLLEXPORT void gr_opengks(void);
DLLEXPORT void gr_closegks(void);
DLLEXPORT void gr_inqdspsize(double *, double *, int *, int *);
DLLEXPORT void gr_openws(int, char *, int);
DLLEXPORT void gr_closews(int);
DLLEXPORT void gr_activatews(int);
DLLEXPORT void gr_deactivatews(int);
DLLEXPORT void gr_configurews(void);
DLLEXPORT void gr_clearws(void);
DLLEXPORT void gr_updatews(void);
DLLEXPORT void gr_polyline(int, double *, double *);
DLLEXPORT void gr_polymarker(int, double *, double *);
DLLEXPORT void gr_text(double, double, char *);
DLLEXPORT void gr_inqtext(double, double, char *, double *, double *);
DLLEXPORT void gr_fillarea(int, double *, double *);
DLLEXPORT void gr_cellarray(double, double, double, double, int, int, int, int, int, int, int *);
DLLEXPORT void gr_nonuniformcellarray(double *, double *, int, int, int, int, int, int, int *);
DLLEXPORT void gr_polarcellarray(double, double, double, double, double, double, int, int, int, int, int, int, int *);
DLLEXPORT void gr_nonuniformpolarcellarray(double, double, double *, double *, int, int, int, int, int, int, int *);
DLLEXPORT void gr_gdp(int, double *, double *, int, int, int *);
DLLEXPORT void gr_spline(int, double *, double *, int, int);
DLLEXPORT void gr_gridit(int, double *, double *, double *, int, int, double *, double *, double *);
DLLEXPORT void gr_setlinetype(int);
DLLEXPORT void gr_inqlinetype(int *);
DLLEXPORT void gr_setlinewidth(double);
DLLEXPORT void gr_inqlinewidth(double *);
DLLEXPORT void gr_setlinecolorind(int);
DLLEXPORT void gr_inqlinecolorind(int *);
DLLEXPORT void gr_setmarkertype(int);
DLLEXPORT void gr_inqmarkertype(int *);
DLLEXPORT void gr_setmarkersize(double);
DLLEXPORT void gr_inqmarkersize(double *);
DLLEXPORT void gr_setmarkercolorind(int);
DLLEXPORT void gr_inqmarkercolorind(int *);
DLLEXPORT void gr_settextfontprec(int, int);
DLLEXPORT void gr_setcharexpan(double);
DLLEXPORT void gr_setcharspace(double);
DLLEXPORT void gr_settextcolorind(int);
DLLEXPORT void gr_inqtextcolorind(int *);
DLLEXPORT void gr_setcharheight(double);
DLLEXPORT void gr_inqcharheight(double *);
DLLEXPORT void gr_setcharup(double, double);
DLLEXPORT void gr_settextpath(int);
DLLEXPORT void gr_settextalign(int, int);
DLLEXPORT void gr_setfillintstyle(int);
DLLEXPORT void gr_inqfillintstyle(int *);
DLLEXPORT void gr_setfillstyle(int);
DLLEXPORT void gr_inqfillstyle(int *);
DLLEXPORT void gr_setfillcolorind(int);
DLLEXPORT void gr_inqfillcolorind(int *);
DLLEXPORT void gr_setcolorrep(int, double, double, double);
DLLEXPORT void gr_setwindow(double, double, double, double);
DLLEXPORT void gr_inqwindow(double *, double *, double *, double *);
DLLEXPORT void gr_setviewport(double, double, double, double);
DLLEXPORT void gr_inqviewport(double *, double *, double *, double *);
DLLEXPORT void gr_selntran(int);
DLLEXPORT void gr_setclip(int);
DLLEXPORT void gr_setwswindow(double, double, double, double);
DLLEXPORT void gr_setwsviewport(double, double, double, double);
DLLEXPORT void gr_createseg(int);
DLLEXPORT void gr_copysegws(int);
DLLEXPORT void gr_redrawsegws(void);
DLLEXPORT void gr_setsegtran(int, double, double, double, double, double, double, double);
DLLEXPORT void gr_closeseg(void);
DLLEXPORT void gr_emergencyclosegks(void);
DLLEXPORT void gr_updategks(void);
DLLEXPORT int gr_setspace(double, double, int, int);
DLLEXPORT void gr_inqspace(double *, double *, int *, int *);
DLLEXPORT int gr_setscale(int);
DLLEXPORT void gr_inqscale(int *);
DLLEXPORT int gr_textext(double, double, char *);
DLLEXPORT void gr_inqtextext(double, double, char *, double *, double *);
DLLEXPORT void gr_axes(double, double, double, double, int, int, double);
DLLEXPORT void gr_axeslbl(double, double, double, double, int, int, double,
                          void (*)(double, double, const char *, double),
                          void (*)(double, double, const char *, double));
DLLEXPORT void gr_grid(double, double, double, double, int, int);
DLLEXPORT void gr_grid3d(double, double, double, double, double, double, int, int, int);
DLLEXPORT void gr_verrorbars(int, double *, double *, double *, double *);
DLLEXPORT void gr_herrorbars(int, double *, double *, double *, double *);
DLLEXPORT void gr_polyline3d(int, double *, double *, double *);
DLLEXPORT void gr_polymarker3d(int, double *, double *, double *);
DLLEXPORT void gr_axes3d(double, double, double, double, double, double, int, int, int, double);
DLLEXPORT void gr_titles3d(char *, char *, char *);
DLLEXPORT void gr_surface(int, int, double *, double *, double *, int);
DLLEXPORT void gr_contour(int, int, int, double *, double *, double *, double *, int);
DLLEXPORT void gr_contourf(int, int, int, double *, double *, double *, double *, int);
DLLEXPORT void gr_tricontour(int, double *, double *, double *, int, double *);
DLLEXPORT int gr_hexbin(int, double *, double *, int);
DLLEXPORT void gr_setcolormap(int);
DLLEXPORT void gr_inqcolormap(int *);
DLLEXPORT void gr_setcolormapfromrgb(int n, double *r, double *g, double *b, double *x);
DLLEXPORT void gr_inqcolormapinds(int *, int *);
DLLEXPORT void gr_colorbar(void);
DLLEXPORT void gr_inqcolor(int, int *);
DLLEXPORT int gr_inqcolorfromrgb(double, double, double);
DLLEXPORT void gr_hsvtorgb(double h, double s, double v, double *r, double *g, double *b);
DLLEXPORT double gr_tick(double, double);
DLLEXPORT int gr_validaterange(double, double);
DLLEXPORT void gr_adjustlimits(double *, double *);
DLLEXPORT void gr_adjustrange(double *, double *);
DLLEXPORT void gr_beginprint(char *);
DLLEXPORT void gr_beginprintext(char *, char *, char *, char *);
DLLEXPORT void gr_endprint(void);
DLLEXPORT void gr_ndctowc(double *, double *);
DLLEXPORT void gr_wctondc(double *, double *);
DLLEXPORT void gr_wc3towc(double *, double *, double *);
DLLEXPORT void gr_drawrect(double, double, double, double);
DLLEXPORT void gr_fillrect(double, double, double, double);
DLLEXPORT void gr_drawarc(double, double, double, double, double, double);
DLLEXPORT void gr_fillarc(double, double, double, double, double, double);
DLLEXPORT void gr_drawpath(int, vertex_t *, unsigned char *, int);
DLLEXPORT void gr_setarrowstyle(int);
DLLEXPORT void gr_setarrowsize(double);
DLLEXPORT void gr_drawarrow(double, double, double, double);
DLLEXPORT int gr_readimage(char *, int *, int *, int **);
DLLEXPORT void gr_drawimage(double, double, double, double, int, int, int *, int);
DLLEXPORT int gr_importgraphics(char *);
DLLEXPORT void gr_setshadow(double, double, double);
DLLEXPORT void gr_settransparency(double);
DLLEXPORT void gr_setcoordxform(double[3][2]);
DLLEXPORT void gr_begingraphics(char *);
DLLEXPORT void gr_endgraphics(void);
DLLEXPORT char *gr_getgraphics(void);
DLLEXPORT int gr_drawgraphics(char *);
DLLEXPORT void gr_mathtex(double, double, char *);
DLLEXPORT void gr_inqmathtex(double, double, char *, double *, double *);
DLLEXPORT void gr_beginselection(int, int);
DLLEXPORT void gr_endselection(void);
DLLEXPORT void gr_moveselection(double, double);
DLLEXPORT void gr_resizeselection(int, double, double);
DLLEXPORT void gr_inqbbox(double *, double *, double *, double *);
DLLEXPORT double gr_precision(void);
DLLEXPORT void gr_setregenflags(int);
DLLEXPORT int gr_inqregenflags(void);
DLLEXPORT void gr_savestate(void);
DLLEXPORT void gr_restorestate(void);
DLLEXPORT void gr_selectcontext(int);
DLLEXPORT void gr_destroycontext(int);
DLLEXPORT int gr_uselinespec(char *);
DLLEXPORT void gr_delaunay(int, const double *, const double *, int *, int **);
DLLEXPORT void gr_reducepoints(int, const double *, const double *, int, double *, double *);
DLLEXPORT void gr_trisurface(int, double *, double *, double *);
DLLEXPORT void gr_gradient(int, int, double *, double *, double *, double *, double *);
DLLEXPORT void gr_quiver(int, int, double *, double *, double *, double *, int);
DLLEXPORT void gr_interp2(int nx, int ny, const double *x, const double *y, const double *z, int nxq, int nyq,
                          const double *xq, const double *yq, double *zq, int method, double extrapval);
DLLEXPORT const char *gr_version(void);
DLLEXPORT void gr_shade(int, double *, double *, int, int, double *, int, int, int *);
DLLEXPORT void gr_shadepoints(int, double *, double *, int, int, int);
DLLEXPORT void gr_shadelines(int, double *, double *, int, int, int);
DLLEXPORT void gr_panzoom(double, double, double, double, double *, double *, double *, double *);
DLLEXPORT int gr_findboundary(int, double *, double *, double, double (*)(double, double), int, int *);
DLLEXPORT void gr_setresamplemethod(unsigned int);
DLLEXPORT void gr_inqresamplemethod(unsigned int *);
DLLEXPORT void gr_path(int, double *, double *, const char *);
DLLEXPORT void gr_setborderwidth(double);
DLLEXPORT void gr_inqborderwidth(double *);
DLLEXPORT void gr_setbordercolorind(int);
DLLEXPORT void gr_inqbordercolorind(int *);
DLLEXPORT void gr_selectclipxform(int);
DLLEXPORT void gr_inqclipxform(int *);
DLLEXPORT void gr_setprojectiontype(int);
DLLEXPORT void gr_inqprojectiontype(int *);
DLLEXPORT void gr_setperspectiveprojection(double, double, double);
DLLEXPORT void gr_inqperspectiveprojection(double *, double *, double *);
DLLEXPORT void gr_settransformationparameters(double, double, double, double, double, double, double, double, double);
DLLEXPORT void gr_inqtransformationparameters(double *, double *, double *, double *, double *, double *, double *,
                                              double *, double *);
DLLEXPORT void gr_setorthographicprojection(double, double, double, double, double, double);
DLLEXPORT void gr_inqorthographicprojection(double *, double *, double *, double *, double *, double *);
DLLEXPORT void gr_camerainteraction(double, double, double, double);
DLLEXPORT void gr_setwindow3d(double, double, double, double, double, double);
DLLEXPORT void gr_inqwindow3d(double *, double *, double *, double *, double *, double *);
DLLEXPORT void gr_setscalefactors3d(double, double, double);
DLLEXPORT void gr_inqscalefactors3d(double *, double *, double *);
DLLEXPORT void gr_setspace3d(double, double, double, double);
DLLEXPORT void gr_text3d(double, double, double, char *, int axis);
DLLEXPORT void gr_inqtext3d(double, double, double, char *, int axis, double *, double *);
DLLEXPORT void gr_settextencoding(int);
DLLEXPORT void gr_inqtextencoding(int *);
DLLEXPORT void gr_loadfont(char *, int *);
DLLEXPORT void gr_setcallback(char *(*)(const char *));
DLLEXPORT void gr_setthreadnumber(int);
DLLEXPORT void gr_setpicturesizeforvolume(int, int);
DLLEXPORT void gr_setvolumebordercalculation(int);
DLLEXPORT void gr_setapproximativecalculation(int);
DLLEXPORT void gr_inqvolumeflags(int *, int *, int *, int *, int *);
DLLEXPORT void gr_cpubasedvolume(int, int, int, double *, int, double *, double *, double *, double *);

#ifdef __cplusplus
}
#endif

#endif
