#ifndef _GR_H_
#define _GR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GR_PROJECTION_DEFAULT 0
#define GR_PROJECTION_ORTHOGRAPHIC 1
#define GR_PROJECTION_PERSPECTIVE 2

typedef struct
{
  double x, y;
} vertex_t;

void gr_initgr(void);
void gr_opengks(void);
void gr_closegks(void);
void gr_inqdspsize(double *, double *, int *, int *);
void gr_openws(int, char *, int);
void gr_closews(int);
void gr_activatews(int);
void gr_deactivatews(int);
void gr_configurews(void);
void gr_clearws(void);
void gr_updatews(void);
void gr_polyline(int, double *, double *);
void gr_polymarker(int, double *, double *);
void gr_text(double, double, char *);
void gr_inqtext(double, double, char *, double *, double *);
void gr_fillarea(int, double *, double *);
void gr_cellarray(double, double, double, double, int, int, int, int, int, int, int *);
void gr_nonuniformcellarray(double *, double *, int, int, int, int, int, int, int *);
void gr_polarcellarray(double, double, double, double, double, double, int, int, int, int, int, int, int *);
void gr_nonuniformpolarcellarray(double, double, double *, double *, int, int, int, int, int, int, int *);
void gr_gdp(int, double *, double *, int, int, int *);
void gr_spline(int, double *, double *, int, int);
void gr_gridit(int, double *, double *, double *, int, int, double *, double *, double *);
void gr_setlinetype(int);
void gr_inqlinetype(int *);
void gr_setlinewidth(double);
void gr_inqlinewidth(double *);
void gr_setlinecolorind(int);
void gr_inqlinecolorind(int *);
void gr_setmarkertype(int);
void gr_inqmarkertype(int *);
void gr_setmarkersize(double);
void gr_inqmarkersize(double *);
void gr_setmarkercolorind(int);
void gr_inqmarkercolorind(int *);
void gr_settextfontprec(int, int);
void gr_setcharexpan(double);
void gr_setcharspace(double);
void gr_settextcolorind(int);
void gr_inqtextcolorind(int *);
void gr_setcharheight(double);
void gr_inqcharheight(double *);
void gr_setcharup(double, double);
void gr_settextpath(int);
void gr_settextalign(int, int);
void gr_setfillintstyle(int);
void gr_inqfillintstyle(int *);
void gr_setfillstyle(int);
void gr_inqfillstyle(int *);
void gr_setfillcolorind(int);
void gr_inqfillcolorind(int *);
void gr_setcolorrep(int, double, double, double);
void gr_setwindow(double, double, double, double);
void gr_inqwindow(double *, double *, double *, double *);
void gr_setviewport(double, double, double, double);
void gr_inqviewport(double *, double *, double *, double *);
void gr_selntran(int);
void gr_setclip(int);
void gr_setwswindow(double, double, double, double);
void gr_setwsviewport(double, double, double, double);
void gr_createseg(int);
void gr_copysegws(int);
void gr_redrawsegws(void);
void gr_setsegtran(int, double, double, double, double, double, double, double);
void gr_closeseg(void);
void gr_emergencyclosegks(void);
void gr_updategks(void);
int gr_setspace(double, double, int, int);
void gr_inqspace(double *, double *, int *, int *);
int gr_setscale(int);
void gr_inqscale(int *);
int gr_textext(double, double, char *);
void gr_inqtextext(double, double, char *, double *, double *);
void gr_axes(double, double, double, double, int, int, double);
void gr_axeslbl(double, double, double, double, int, int, double, void (*)(double, double, const char *, double),
                void (*)(double, double, const char *, double));
void gr_grid(double, double, double, double, int, int);
void gr_grid3d(double, double, double, double, double, double, int, int, int);
void gr_verrorbars(int, double *, double *, double *, double *);
void gr_herrorbars(int, double *, double *, double *, double *);
void gr_polyline3d(int, double *, double *, double *);
void gr_polymarker3d(int, double *, double *, double *);
void gr_axes3d(double, double, double, double, double, double, int, int, int, double);
void gr_titles3d(char *, char *, char *);
void gr_surface(int, int, double *, double *, double *, int);
void gr_contour(int, int, int, double *, double *, double *, double *, int);
void gr_contourf(int, int, int, double *, double *, double *, double *, int);
void gr_tricontour(int, double *, double *, double *, int, double *);
int gr_hexbin(int, double *, double *, int);
void gr_setcolormap(int);
void gr_inqcolormap(int *);
void gr_setcolormapfromrgb(int n, double *r, double *g, double *b, double *x);
void gr_colorbar(void);
void gr_inqcolor(int, int *);
int gr_inqcolorfromrgb(double, double, double);
void gr_hsvtorgb(double h, double s, double v, double *r, double *g, double *b);
double gr_tick(double, double);
int gr_validaterange(double, double);
void gr_adjustlimits(double *, double *);
void gr_adjustrange(double *, double *);
void gr_beginprint(char *);
void gr_beginprintext(char *, char *, char *, char *);
void gr_endprint(void);
void gr_ndctowc(double *, double *);
void gr_wctondc(double *, double *);
void gr_wc3towc(double *, double *, double *);
void gr_drawrect(double, double, double, double);
void gr_fillrect(double, double, double, double);
void gr_drawarc(double, double, double, double, double, double);
void gr_fillarc(double, double, double, double, double, double);
void gr_drawpath(int, vertex_t *, unsigned char *, int);
void gr_setarrowstyle(int);
void gr_setarrowsize(double);
void gr_drawarrow(double, double, double, double);
int gr_readimage(char *, int *, int *, int **);
void gr_drawimage(double, double, double, double, int, int, int *, int);
int gr_importgraphics(char *);
void gr_setshadow(double, double, double);
void gr_settransparency(double);
void gr_setcoordxform(double[3][2]);
void gr_begingraphics(char *);
void gr_endgraphics(void);
char *gr_getgraphics(void);
int gr_drawgraphics(char *);
void gr_mathtex(double, double, char *);
void gr_inqmathtex(double, double, char *, double *, double *);
void gr_beginselection(int, int);
void gr_endselection(void);
void gr_moveselection(double, double);
void gr_resizeselection(int, double, double);
void gr_inqbbox(double *, double *, double *, double *);
double gr_precision(void);
void gr_setregenflags(int);
int gr_inqregenflags(void);
void gr_savestate(void);
void gr_restorestate(void);
void gr_selectcontext(int);
void gr_destroycontext(int);
int gr_uselinespec(char *);
void gr_delaunay(int, const double *, const double *, int *, int **);
void gr_reducepoints(int, const double *, const double *, int, double *, double *);
void gr_trisurface(int, double *, double *, double *);
void gr_gradient(int, int, double *, double *, double *, double *, double *);
void gr_quiver(int, int, double *, double *, double *, double *, int);
void gr_interp2(int nx, int ny, const double *x, const double *y, const double *z, int nxq, int nyq, const double *xq,
                const double *yq, double *zq, int method, double extrapval);
const char *gr_version(void);
void gr_shade(int, double *, double *, int, int, double *, int, int, int *);
void gr_shadepoints(int, double *, double *, int, int, int);
void gr_shadelines(int, double *, double *, int, int, int);
void gr_panzoom(double, double, double, double, double *, double *, double *, double *);
int gr_findboundary(int, double *, double *, double, double (*)(double, double), int, int *);
void gr_setresamplemethod(unsigned int);
void gr_inqresamplemethod(unsigned int *);
void gr_path(int, double *, double *, const char *);
void gr_setborderwidth(double);
void gr_inqborderwidth(double *);
void gr_setbordercolorind(int);
void gr_inqbordercolorind(int *);
void gr_selectclipxform(int);
void gr_inqclipxform(int *);
void gr_setprojectiontype(int);
void gr_inqprojectiontype(int *);
void gr_setperspectiveprojection(double, double, double);
void gr_inqperspectiveprojection(double *, double *, double *);
void gr_settransformationparameters(double, double, double, double, double, double, double, double, double);
void gr_inqtransformationparameters(double *, double *, double *, double *, double *, double *, double *, double *,
                                    double *);
void gr_setorthographicprojection(double, double, double, double, double, double);
void gr_inqorthographicprojection(double *, double *, double *, double *, double *, double *);
void gr_camerainteraction(double, double, double, double);
void gr_setwindow3d(double, double, double, double, double, double);
void gr_inqwindow3d(double *, double *, double *, double *, double *, double *);
void gr_setscalefactors3d(double, double, double);
void gr_inqscalefactors3d(double *, double *, double *);
void gr_setspace3d(double, double, double, double);
void gr_text3d(double, double, double, char *, int axis);
void gr_inqtext3d(double, double, double, char *, int axis, double *, double *);
void gr_settextencoding(int);
void gr_inqtextencoding(int *);
void gr_setcallback(char *(*)(const char *));

#ifdef __cplusplus
}
#endif

#endif
