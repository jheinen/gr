GR.is_ready = false;
GR.ready_callbacks = [];
GR.ready = function(callback){
    if(!GR.is_ready) {
        GR.ready_callbacks.push(callback);
    } else {
        callback();
    }
};

Module.onRuntimeInitialized = function() {
    GR.is_ready = true;
    GR.ready_callbacks.forEach(function (callback) {
        callback();
    });
};

select_canvas = function() {
    Module.canvas = this.current_canvas;
    Module.context = this.current_context;
    Module.set_dpr();
};

function GR(canvas_id) {
    this.opengks = gr_opengks;
    this.closegks = gr_closegks;
    this.inqdspsize = gr_inqdspsize;
    this.openws = gr_openws;
    this.closews = gr_closews;
    this.activatews = gr_activatews;
    this.deactivatews = gr_deactivatews;
    this.clearws = gr_clearws;
    this.updatews = gr_updatews;
    this.polyline = gr_polyline;
    this.polymarker = gr_polymarker;
    this.text = gr_text;
    this.inqtext = gr_inqtext;
    this.fillarea = gr_fillarea;
    this.cellarray = gr_cellarray;
    this.spline = gr_spline;
    this.gridit = gr_gridit;
    this.setlinetype = gr_setlinetype;
    this.inqlinetype = gr_inqlinetype;
    this.setlinewidth = gr_setlinewidth;
    this.inqlinewidth = gr_inqlinewidth;
    this.setlinecolorind = gr_setlinecolorind;
    this.inqlinecolorind = gr_inqlinecolorind;
    this.setmarkertype = gr_setmarkertype;
    this.inqmarkertype = gr_inqmarkertype;
    this.setmarkersize = gr_setmarkersize;
    this.setmarkercolorind = gr_setmarkercolorind;
    this.inqmarkercolorind = gr_inqmarkercolorind;
    this.settextfontprec = gr_settextfontprec;
    this.setcharexpan = gr_setcharexpan;
    this.setcharspace = gr_setcharspace;
    this.settextcolorind = gr_settextcolorind;
    this.setcharheight = gr_setcharheight;
    this.setcharup = gr_setcharup;
    this.settextpath = gr_settextpath;
    this.settextalign = gr_settextalign;
    this.setfillintstyle = gr_setfillintstyle;
    this.setfillstyle = gr_setfillstyle;
    this.setfillcolorind = gr_setfillcolorind;
    this.setcolorrep = gr_setcolorrep;
    this.setscale = gr_setscale;
    this.inqscale = gr_inqscale;
    this.setwindow = gr_setwindow;
    this.inqwindow = gr_inqwindow;
    this.setviewport = gr_setviewport;
    this.inqviewport = gr_inqviewport;
    this.selntran = gr_selntran;
    this.setclip = gr_setclip;
    this.setwswindow = gr_setwswindow;
    this.setwsviewport = gr_setwsviewport;
    this.createseg = gr_createseg;
    this.copysegws = gr_copysegws;
    this.redrawsegws = gr_redrawsegws;
    this.setsegtran = gr_setsegtran;
    this.closeseg = gr_closeseg;
    this.emergencyclosegks = gr_emergencyclosegks;
    this.updategks = gr_updategks;
    this.setspace = gr_setspace;
    this.inqspace = gr_inqspace;
    this.textext = gr_textext;
    this.inqtextext = gr_inqtextext;
    this.axes = gr_axes;
    this.grid = gr_grid;
    this.verrorbars = gr_verrorbars;
    this.herrorbars = gr_herrorbars;
    this.polyline3d = gr_polyline3d;
    this.axes3d = gr_axes3d;
    this.titles3d = gr_titles3d;
    this.surface = gr_surface;
    this.contour = gr_contour;
    this.hexbin = gr_hexbin;
    this.setcolormap = gr_setcolormap;
    this.inqcolormap = gr_inqcolormap;
    this.colormap = gr_colorbar;
    this.inqcolor = gr_inqcolor;
    this.inqcolorfromrgb = gr_inqcolorfromrgb;
    this.hsvtorgb = gr_hsvtorgb;
    this.tick = gr_tick;
    this.validaterange = gr_validaterange;
    this.adjustrange = gr_adjustrange;
    this.beginprint = gr_beginprint;
    this.beginprintext = gr_beginprintext;
    this.endprint = gr_endprint;
    this.ndctowc = gr_ndctowc;
    this.wctondc = gr_wctondc;
    this.drawrect = gr_drawrect;
    this.fillrect = gr_fillrect;
    this.drawarc = gr_drawarc;
    this.fillarc = gr_fillarc;
    this.drawpath = gr_drawpath;
    this.setarrowstyle = gr_setarrowstyle;
    this.drawarrow = gr_drawarrow;
    this.readimage = gr_readimage;
    this.drawimage = gr_drawimage;
    this.importgraphics = gr_importgraphics;
    this.setshadow = gr_setshadow;
    this.settransparency = gr_settransparency;
    this.setcoordxform = gr_setcoordxform;
    this.begingraphics = gr_begingraphics;
    this.endgraphics = gr_endgraphics;
    this.drawgraphics = gr_drawgraphics;
    this.mathtex = gr_mathtex;
    this.beginselection = gr_beginselection;
    this.endselection = gr_endselection;
    this.moveselection = gr_moveselection;
    this.resizeselection = gr_resizeselection;
    this.inqbbox = gr_inqbbox;
    this.precision = gr_precision;
    this.setregenflags = gr_setregenflags;
    this.inqregenflags = gr_inqregenflags;
    this.savestate = gr_savestate;
    this.restorestate = gr_restorestate;
    this.selectcontext = gr_selectcontext;
    this.destroycontext = gr_destroycontext;
    this.uselinespec = gr_uselinespec;
    this.selntran = gr_selntran;
    this.shade = gr_shade;
    this.shadepoints = gr_shadepoints;
    this.shadelines = gr_shadelines;
    this.panzoom = gr_panzoom;
    this.path = gr_path;
    this.setborderwidth = gr_setborderwidth;
    this.inqborderwidth = gr_inqborderwidth;
    this.setbordercolorind = gr_setbordercolorind;
    this.inqbordercolorind = gr_inqbordercolorind;
    this.selectclipxform = gr_selectclipxform;
    this.inqclipxform = gr_inqclipxform;

    // set canvas and context
    Module.set_canvas(canvas_id);
    this.current_canvas = Module.canvas;
    this.current_context = Module.context;
    this.select_canvas = select_canvas;

    // constants
    this.NOCLIP = 0;
    this.CLIP = 1;

    this.COORDINATES_WC = 0;
    this.COORDINATES_NDC = 1;

    this.INTSTYLE_HOLLOW = 0;
    this.INTSTYLE_SOLID = 1;
    this.INTSTYLE_PATTERN = 2;
    this.INTSTYLE_HATCH = 3;

    this.TEXT_HALIGN_NORMAL = 0;
    this.TEXT_HALIGN_LEFT = 1;
    this.TEXT_HALIGN_CENTER = 2;
    this.TEXT_HALIGN_RIGHT = 3;
    this.TEXT_VALIGN_NORMAL = 0;
    this.TEXT_VALIGN_TOP = 1;
    this.TEXT_VALIGN_CAP = 2;
    this.TEXT_VALIGN_HALF = 3;
    this.TEXT_VALIGN_BASE = 4;
    this.TEXT_VALIGN_BOTTOM = 5;

    this.TEXT_PATH_RIGHT = 0;
    this.TEXT_PATH_LEFT = 1;
    this.TEXT_PATH_UP = 2;
    this.TEXT_PATH_DOWN = 3;

    this.TEXT_PRECISION_STRING = 0;
    this.TEXT_PRECISION_CHAR = 1;
    this.TEXT_PRECISION_STROKE = 2;

    this.LINETYPE_SOLID = 1;
    this.LINETYPE_DASHED = 2;
    this.LINETYPE_DOTTED = 3;
    this.LINETYPE_DASHED_DOTTED = 4;
    this.LINETYPE_DASH_2_DOT = -1;
    this.LINETYPE_DASH_3_DOT = -2;
    this.LINETYPE_LONG_DASH = -3;
    this.LINETYPE_LONG_SHORT_DASH = -4;
    this.LINETYPE_SPACED_DASH = -5;
    this.LINETYPE_SPACED_DOT = -6;
    this.LINETYPE_DOUBLE_DOT = -7;
    this.LINETYPE_TRIPLE_DOT = -8;

    this.MARKERTYPE_DOT = 1;
    this.MARKERTYPE_PLUS = 2;
    this.MARKERTYPE_ASTERISK = 3;
    this.MARKERTYPE_CIRCLE = 4;
    this.MARKERTYPE_DIAGONAL_CROSS = 5;
    this.MARKERTYPE_SOLID_CIRCLE = -1;
    this.MARKERTYPE_TRIANGLE_UP = -2;
    this.MARKERTYPE_SOLID_TRI_UP = -3;
    this.MARKERTYPE_TRIANGLE_DOWN = -4;
    this.MARKERTYPE_SOLID_TRI_DOWN = -5;
    this.MARKERTYPE_SQUARE = -6;
    this.MARKERTYPE_SOLID_SQUARE = -7;
    this.MARKERTYPE_BOWTIE = -8;
    this.MARKERTYPE_SOLID_BOWTIE = -9;
    this.MARKERTYPE_HOURGLASS = -10;
    this.MARKERTYPE_SOLID_HGLASS = -11;
    this.MARKERTYPE_DIAMOND = -12;
    this.MARKERTYPE_SOLID_DIAMOND = -13;
    this.MARKERTYPE_STAR = -14;
    this.MARKERTYPE_SOLID_STAR = -15;
    this.MARKERTYPE_TRI_UP_DOWN = -16;
    this.MARKERTYPE_SOLID_TRI_RIGHT = -17;
    this.MARKERTYPE_SOLID_TRI_LEFT = -18;
    this.MARKERTYPE_HOLLOW_PLUS = -19;
    this.MARKERTYPE_SOLID_PLUS = -20;
    this.MARKERTYPE_PENTAGON = -21;
    this.MARKERTYPE_HEXAGON = -22;
    this.MARKERTYPE_HEPTAGON = -23;
    this.MARKERTYPE_OCTAGON = -24;
    this.MARKERTYPE_STAR_4 = -25;
    this.MARKERTYPE_STAR_5 = -26;
    this.MARKERTYPE_STAR_6 = -27;
    this.MARKERTYPE_STAR_7 = -28;
    this.MARKERTYPE_STAR_8 = -29;
    this.MARKERTYPE_VLINE = -30;
    this.MARKERTYPE_HLINE = -31;
    this.MARKERTYPE_OMARK = -32;

    this.OPTION_X_LOG = 1;
    this.OPTION_Y_LOG = 2;
    this.OPTION_Z_LOG = 4;
    this.OPTION_FLIP_X = 8;
    this.OPTION_FLIP_Y = 16;
    this.OPTION_FLIP_Z = 32;

    this.OPTION_LINES = 0;
    this.OPTION_MESH = 1;
    this.OPTION_FILLED_MESH = 2;
    this.OPTION_Z_SHADED_MESH = 3;
    this.OPTION_COLORED_MESH = 4;
    this.OPTION_CELL_ARRAY = 5;
    this.OPTION_SHADED_MESH = 6;

    this.MODEL_RGB = 0;
    this.MODEL_HSV = 1;

    this.COLORMAP_UNIFORM = 0;
    this.COLORMAP_TEMPERATURE = 1;
    this.COLORMAP_GRAYSCALE = 2;
    this.COLORMAP_GLOWING = 3;
    this.COLORMAP_RAINBOWLIKE = 4;
    this.COLORMAP_GEOLOGIC = 5;
    this.COLORMAP_GREENSCALE = 6;
    this.COLORMAP_CYANSCALE = 7;
    this.COLORMAP_BLUESCALE = 8;
    this.COLORMAP_MAGENTASCALE = 9;
    this.COLORMAP_REDSCALE = 10;
    this.COLORMAP_FLAME = 11;
    this.COLORMAP_BROWNSCALE = 12;
    this.COLORMAP_PILATUS = 13;
    this.COLORMAP_AUTUMN = 14;
    this.COLORMAP_BONE = 15;
    this.COLORMAP_COOL = 16;
    this.COLORMAP_COPPER = 17;
    this.COLORMAP_GRAY = 18;
    this.COLORMAP_HOT = 19;
    this.COLORMAP_HSV = 20;
    this.COLORMAP_JET = 21;
    this.COLORMAP_PINK = 22;
    this.COLORMAP_SPECTRAL = 23;
    this.COLORMAP_SPRING = 24;
    this.COLORMAP_SUMMER = 25;
    this.COLORMAP_WINTER = 26;
    this.COLORMAP_GIST_EARTH = 27;
    this.COLORMAP_GIST_HEAT = 28;
    this.COLORMAP_GIST_NCAR = 29;
    this.COLORMAP_GIST_RAINBOW = 30;
    this.COLORMAP_GIST_STERN = 31;
    this.COLORMAP_AFMHOT = 32;
    this.COLORMAP_BRG = 33;
    this.COLORMAP_BWR = 34;
    this.COLORMAP_COOLWARM = 35;
    this.COLORMAP_CMRMAP = 36;
    this.COLORMAP_CUBEHELIX = 37;
    this.COLORMAP_GNUPLOT = 38;
    this.COLORMAP_GNUPLOT2 = 39;
    this.COLORMAP_OCEAN = 40;
    this.COLORMAP_RAINBOW = 41;
    this.COLORMAP_SEISMIC = 42;
    this.COLORMAP_TERRAIN = 43;
    this.COLORMAP_VIRIDIS = 44;
    this.COLORMAP_INFERNO = 45;
    this.COLORMAP_PLASMA = 46;
    this.COLORMAP_MAGMA = 47;

    this.FONT_TIMES_ROMAN = 101;
    this.FONT_TIMES_ITALIC = 102;
    this.FONT_TIMES_BOLD = 103;
    this.FONT_TIMES_BOLDITALIC = 104;
    this.FONT_HELVETICA = 105;
    this.FONT_HELVETICA_OBLIQUE = 106;
    this.FONT_HELVETICA_BOLD = 107;
    this.FONT_HELVETICA_BOLDOBLIQUE = 108;
    this.FONT_COURIER = 109;
    this.FONT_COURIER_OBLIQUE = 110;
    this.FONT_COURIER_BOLD = 111;
    this.FONT_COURIER_BOLDOBLIQUE = 112;
    this.FONT_SYMBOL = 113;
    this.FONT_BOOKMAN_LIGHT = 114;
    this.FONT_BOOKMAN_LIGHTITALIC = 115;
    this.FONT_BOOKMAN_DEMI = 116;
    this.FONT_BOOKMAN_DEMIITALIC = 117;
    this.FONT_NEWCENTURYSCHLBK_ROMAN = 118;
    this.FONT_NEWCENTURYSCHLBK_ITALIC = 119;
    this.FONT_NEWCENTURYSCHLBK_BOLD = 120;
    this.FONT_NEWCENTURYSCHLBK_BOLDITALIC = 121;
    this.FONT_AVANTGARDE_BOOK = 122;
    this.FONT_AVANTGARDE_BOOKOBLIQUE = 123;
    this.FONT_AVANTGARDE_DEMI = 124;
    this.FONT_AVANTGARDE_DEMIOBLIQUE = 125;
    this.FONT_PALATINO_ROMAN = 126;
    this.FONT_PALATINO_ITALIC = 127;
    this.FONT_PALATINO_BOLD = 128;
    this.FONT_PALATINO_BOLDITALIC = 129;
    this.FONT_ZAPFCHANCERY_MEDIUMITALIC = 130;
    this.FONT_ZAPFDINGBATS = 131;

    // gr.beginprint types;
    this.PRINT_PS = "ps";
    this.PRINT_EPS = "eps";
    this.PRINT_PDF = "pdf";
    this.PRINT_PGF = "pgf";
    this.PRINT_BMP = "bmp";
    this.PRINT_JPEG = "jpeg";
    this.PRINT_JPG = "jpg";
    this.PRINT_PNG = "png";
    this.PRINT_TIFF = "tiff";
    this.PRINT_TIF = "tif";
    this.PRINT_FIG = "fig";
    this.PRINT_SVG = "svg";
    this.PRINT_WMF = "wmf";
}

function GRM(canvas_id) {
    this.args_new = grm_args_new;
    this.args_push = grm_args_push;
    this.args_delete = grm_args_delete;
    this.get_stdout = grm_get_stdout;
    this.read = grm_read;
    this.plot = grm_plot;
    this.dump_json = grm_dump_json;
    this.dump = grm_dump;
    this.input = grm_input;
    this.merge = grm_merge;
    this.merge_named = grm_merge_named;
    this.switch = grm_switch;
    this.get_box = grm_get_box;
    this.get_tooltip = grm_get_tooltip;
    this.register = grm_register;
    this.unregister = grm_unregister;
    this.dump_json_str = grm_dump_json_str;
    this.load_from_str = grm_load_from_str;

    // set canvas and context
    Module.set_canvas(canvas_id);
    this.current_canvas = Module.canvas;
    this.current_context = Module.context;
    this.select_canvas = select_canvas;

    this.EVENT_NEW_PLOT = 0;
    this.EVENT_UPDATE_PLOT = 1;
    this.EVENT_SIZE = 2;
    this.EVENT_MERGE_END = 3;

    this.callbacks = [Function.prototype, Function.prototype, Function.prototype, Function.prototype];

    grm_register_c(this.EVENT_NEW_PLOT, Module.addFunction(function(evt) {
        var evt_data = {
            'evt_type': Module.HEAP32.subarray(evt / 4, evt / 4 + 1)[0],
            'plot_id': Module.HEAP32.subarray(evt / 4 + 1, evt / 4 + 2)[0]
        };
        this.callbacks[this.EVENT_NEW_PLOT](evt_data);
    }.bind(this), 'vi'));

    grm_register_c(this.EVENT_UPDATE_PLOT, Module.addFunction(function(evt) {
        var evt_data = {
            'evt_type': Module.HEAP32.subarray(evt / 4, evt / 4 + 1)[0],
            'plot_id': Module.HEAP32.subarray(evt / 4 + 1, evt / 4 + 2)[0]
        };
        this.callbacks[this.EVENT_UPDATE_PLOT](evt_data);
    }.bind(this), 'vi'));

    grm_register_c(this.EVENT_SIZE, Module.addFunction(function(evt) {
        var evt_data = {
            'evt_type': Module.HEAP32.subarray(evt / 4, evt / 4 + 1)[0],
            'plot_id': Module.HEAP32.subarray(evt / 4 + 1, evt / 4 + 2)[0],
            'width': Module.HEAP32.subarray(evt / 4 + 2, evt / 4 + 3)[0],
            'height': Module.HEAP32.subarray(evt / 4 + 3, evt / 4 + 4)[0]
        };
        this.callbacks[this.EVENT_SIZE](evt_data);
    }.bind(this), 'vi'));

    grm_register_c(this.EVENT_MERGE_END, Module.addFunction(function(evt) {
        var evt_data = {
            'evt_type': Module.HEAP32.subarray(evt / 4, evt / 4 + 1)[0],
            'identificator': Module.UTF8ToString(Module.HEAP32.subarray(evt / 4 + 1, evt / 4 + 2)[0])
        };
        this.callbacks[this.EVENT_MERGE_END](evt_data);
    }.bind(this), 'vi'));
}

floatarray = function(a) {
    var ptr = Module._malloc(a.length * 8);
    var data = Module.HEAPF64.subarray(ptr / 8, ptr / 8 + a.length);

    for (i = 0; i < a.length; i++){
        data[i] = a[i];
    }

    return ptr;
};

intarray = function(a) {
    var ptr = Module._malloc(a.length * 4);
    var data = Module.HEAP32.subarray(ptr / 4, ptr / 4 + a.length);
    for (i = 0; i < a.length; i++) {
        data[i] = a[i];
    }

    return ptr;
};

uint8array = function(a) {
    var ptr = Module._malloc(a.length + 1);
    a = intArrayFromString(a, true);
    var data = Module.HEAPU8.subarray(ptr, ptr + a.length + 1);
    for (i = 0; i < a.length; i++) {
        data[i] = a[i];
    }
    data[a.length] = 0x00;
    return ptr;
};

freearray = function(ptr) {
    Module._free(ptr);
};

gr_opengks = Module.cwrap('gr_opengks', '', []);

gr_closegks = Module.cwrap('gr_closegks', '', []);

gr_inqdspsize_c = Module.cwrap('gr_inqdspsize', '', ['number', 'number', 'number', 'number', ]);
gr_inqdspsize = function() {
    var _mwidth = Module._malloc(8);
    var _mheight = Module._malloc(8);
    var _width = Module._malloc(4);
    var _height = Module._malloc(4);
    gr_inqdspsize_c(_mwidth, _mheight, _width, _height);
    var result = new Array(4);
    result[0] = Module.HEAPF64.subarray(_mwidth / 8, _mwidth / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_mheight / 8, _mheight / 8 + 1)[0];
    result[2] = Module.HEAP32.subarray(_width / 4, _width / 4 + 1)[0];
    result[3] = Module.HEAP32.subarray(_height / 4, _height / 4 + 1)[0];
    freearray(_mwidth);
    freearray(_mheight);
    freearray(_width);
    freearray(_height);
    return result;
};

gr_openws_c = Module.cwrap('gr_openws', '', ['number', 'number', 'number', ]);
gr_openws = function(workstation_id, connection, type) {
    _connection = uint8array(connection);
    gr_openws_c(workstation_id, _connection, type);
    freearray(_connection);
};

gr_closews = Module.cwrap('gr_closews', '', ['number', ]);

gr_activatews = Module.cwrap('gr_activatews', '', ['number', ]);

gr_deactivatews = Module.cwrap('gr_deactivatews', '', ['number', ]);

gr_clearws = Module.cwrap('gr_clearws', '', []);

gr_updatews = Module.cwrap('gr_updatews', '', []);

gr_polyline_c = Module.cwrap('gr_polyline', '', ['number', 'number', 'number', ]);
gr_polyline = function(n, x, y) {
    this.select_canvas();
    _x = floatarray(x);
    _y = floatarray(y);

    gr_polyline_c(n, _x, _y);
    freearray(_x);
    freearray(_y);
};

gr_polymarker_c = Module.cwrap('gr_polymarker', '', ['number', 'number', 'number', ]);
gr_polymarker = function(n, x, y) {
    this.select_canvas();
    _x = floatarray(x);
    _y = floatarray(y);

    gr_polymarker_c(n, _x, _y);
    freearray(_x);
    freearray(_y);
};

gr_text_c = Module.cwrap('gr_text', '', ['number', 'number', 'number', ]);
gr_text = function(x, y, string) {
    this.select_canvas();
    _string = uint8array(string);
    gr_text_c(x, y, _string);
    freearray(_string);
};

gr_inqtext_c = Module.cwrap('gr_inqtext', '', ['number', 'number', 'number', 'number', 'number', ]);
gr_inqtext = function(x, y, string) {
    var _string = uint8array(string);
    var _tbx = Module._malloc(8);
    var _tby = Module._malloc(8);
    gr_inqtext_c(x, y, _string, _tbx, _tby);
    var result = new Array(2);
    result[0] = Module.HEAPF64.subarray(_tbx / 8, _tbx / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_tby / 8, _tby / 8 + 1)[0];
    freearray(_string);
    freearray(_tbx);
    freearray(_tby);
    return result;
};

gr_fillarea_c = Module.cwrap('gr_fillarea', '', ['number', 'number', 'number', ]);
gr_fillarea = function(n, x, y) {
    this.select_canvas();
    _x = floatarray(x);
    _y = floatarray(y);

    gr_fillarea_c(n, _x, _y);
    freearray(_x);
    freearray(_y);
};

gr_cellarray_c = Module.cwrap('gr_cellarray', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);
gr_cellarray = function(xmin, xmax, ymin, ymax, dimx, dimy, scol, srow, ncol, nrow, color) {
    this.select_canvas();
    var _color = intarray(color);
    gr_cellarray_c(xmin, xmax, ymin, ymax, dimx, dimy, scol, srow, ncol, nrow, _color);
    freearray(_color);
};

gr_spline_c = Module.cwrap('gr_spline', '', ['number', 'number', 'number', 'number', 'number', ]);
gr_spline = function(n, px, py, m, method) {
    var _px = floatarray(px);
    var _py = floatarray(py);

    gr_spline_c(n, _px, _py, m, method);
    freearray(_px);
    freearray(_py);
};


gr_gridit_c = Module.cwrap('gr_gridit', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);
gr_gridit = function(nd, xd, yd, zd, nx, ny) {
    var _xd = floatarray(xd);
    var _yd = floatarray(yd);
    var _zd = floatarray(zd);
    var _x = Module._malloc(nx * 8);
    var x = Module.HEAPF64.subarray(_x / 8, _x / 8 + nx);
    var _y = Module._malloc(ny * 8);
    var y = Module.HEAPF64.subarray(_y / 8, _y / 8 + ny);
    var _z = Module._malloc(nx*ny * 8);
    var z = Module.HEAPF64.subarray(_z / 8, _z / 8 + nx*ny);

    gr_gridit_c(nd, _xd, _yd, _zd, nx, ny, _x, _y, _z);
    var result = new Array(3);
    result[0] = new Float64Array(new ArrayBuffer(nx * 8));
    result[0].set(x);
    result[1] = new Float64Array(new ArrayBuffer(ny * 8));
    result[1].set(y);
    result[2] = new Float64Array(new ArrayBuffer(nx*ny * 8));
    result[2].set(z);
    freearray(_xd);
    freearray(_yd);
    freearray(_zd);
    freearray(_x);
    freearray(_y);
    freearray(_z);
    return result;
};

gr_setlinetype = Module.cwrap('gr_setlinetype', '', ['number', ]);

gr_inqlinetype_c = Module.cwrap('gr_inqlinetype', '', ['number', ]);
gr_inqlinetype = function() {
    var _ltype = Module._malloc(4);
    gr_inqlinetype_c(_ltype);
    result = Module.HEAP32.subarray(_ltype / 4, _ltype / 4 + 1)[0];
    freearray(_ltype);
    return result;
};

gr_setlinewidth = Module.cwrap('gr_setlinewidth', '', ['number', ]);

gr_inqlinewidth_c = Module.cwrap('gr_inqlinewidth', '', ['number', ]);
gr_inqlinewidth = function() {
    var _width = Module._malloc(8);
    gr_inqlinewidth_c(_width);
    result = Module.HEAPF64.subarray(_width / 8, _width / 8 + 1)[0];
    freearray(_width);
    return result;
};

gr_setlinecolorind = Module.cwrap('gr_setlinecolorind', '', ['number', ]);

gr_inqlinecolorind_c = Module.cwrap('gr_inqlinecolorind', '', ['number', ]);
gr_inqlinecolorind = function() {
    var _coli = Module._malloc(4);
    gr_inqlinecolorind_c(_coli);
    result = Module.HEAP32.subarray(_coli / 4, _coli / 4 + 1)[0];
    freearray(_coli);
    return result;
};

gr_setmarkertype = Module.cwrap('gr_setmarkertype', '', ['number', ]);

gr_inqmarkertype_c = Module.cwrap('gr_inqmarkertype', '', ['number', ]);
gr_inqmarkertype = function() {
    var _mtype = Module._malloc(4);
    gr_inqmarkertype_c(_mtype);
    result = Module.HEAP32.subarray(_mtype / 4, _mtype / 4 + 1)[0];
    freearray(_mtype);
    return result;
};

gr_setmarkersize = Module.cwrap('gr_setmarkersize', '', ['number', ]);

gr_setmarkercolorind = Module.cwrap('gr_setmarkercolorind', '', ['number', ]);

gr_inqmarkercolorind_c = Module.cwrap('gr_inqmarkercolorind', '', ['number', ]);
gr_inqmarkercolorind = function() {
    var _coli = Module._malloc(4);
    gr_inqmarkercolorind_c(_coli);
    result = Module.HEAP32.subarray(_coli / 4, _coli / 4 + 1)[0];
    freearray(_coli);
    return result;
};

gr_settextfontprec = Module.cwrap('gr_settextfontprec', '', ['number', 'number', ]);

gr_setcharexpan = Module.cwrap('gr_setcharexpan', '', ['number', ]);

gr_setcharspace = Module.cwrap('gr_setcharspace', '', ['number', ]);

gr_settextcolorind = Module.cwrap('gr_settextcolorind', '', ['number', ]);

gr_setcharheight = Module.cwrap('gr_setcharheight', '', ['number', ]);

gr_setcharup = Module.cwrap('gr_setcharup', '', ['number', 'number', ]);

gr_settextpath = Module.cwrap('gr_settextpath', '', ['number', ]);

gr_settextalign = Module.cwrap('gr_settextalign', '', ['number', 'number', ]);

gr_setfillintstyle = Module.cwrap('gr_setfillintstyle', '', ['number', ]);

gr_setfillstyle = Module.cwrap('gr_setfillstyle', '', ['number', ]);

gr_setfillcolorind = Module.cwrap('gr_setfillcolorind', '', ['number', ]);

gr_setcolorrep = Module.cwrap('gr_setcolorrep', '', ['number', 'number', 'number', 'number', ]);

gr_setscale = Module.cwrap('gr_setscale', 'number', ['number', ]);

gr_inqscale_c = Module.cwrap('gr_inqscale', '', ['number', ]);
gr_inqscale = function() {
    var _options = Module._malloc(4);
    gr_inqscale_c(_options);
    result = Module.HEAP32.subarray(_options / 4, _options / 4 + 1)[0];
    freearray(_options);
    return result;
};

gr_setwindow = Module.cwrap('gr_setwindow', '', ['number', 'number', 'number', 'number', ]);

gr_inqwindow_c = Module.cwrap('gr_inqwindow', '', ['number', 'number', 'number', 'number', ]);
gr_inqwindow = function() {
    var _xmin = Module._malloc(8);
    var _xmax = Module._malloc(8);
    var _ymin = Module._malloc(8);
    var _ymax = Module._malloc(8);
    gr_inqwindow_c(_xmin, _xmax, _ymin, _ymax);
    var result = new Array(4);
    result[0] = Module.HEAPF64.subarray(_xmin / 8, _xmin / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_xmax / 8, _xmax / 8 + 1)[0];
    result[2] = Module.HEAPF64.subarray(_ymin / 8, _ymin / 8 + 1)[0];
    result[3] = Module.HEAPF64.subarray(_ymax / 8, _ymax / 8 + 1)[0];
    freearray(_xmin);
    freearray(_xmax);
    freearray(_ymin);
    freearray(_ymax);
    return result;
};

gr_setviewport = Module.cwrap('gr_setviewport', '', ['number', 'number', 'number', 'number', ]);

gr_inqviewport_c = Module.cwrap('gr_inqviewport', '', ['number', 'number', 'number', 'number', ]);
gr_inqviewport = function() {
    var _xmin = Module._malloc(8);
    var _xmax = Module._malloc(8);
    var _ymin = Module._malloc(8);
    var _ymax = Module._malloc(8);
    gr_inqviewport_c(_xmin, _xmax, _ymin, _ymax);
    var result = new Array(4);
    result[0] = Module.HEAPF64.subarray(_xmin / 8, _xmin / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_xmax / 8, _xmax / 8 + 1)[0];
    result[2] = Module.HEAPF64.subarray(_ymin / 8, _ymin / 8 + 1)[0];
    result[3] = Module.HEAPF64.subarray(_ymax / 8, _ymax / 8 + 1)[0];
    freearray(_xmin);
    freearray(_xmax);
    freearray(_ymin);
    freearray(_ymax);
    return result;
};

gr_selntran = Module.cwrap('gr_selntran', '', ['number', ]);

gr_setclip = Module.cwrap('gr_setclip', '', ['number', ]);

gr_setwswindow = Module.cwrap('gr_setwswindow', '', ['number', 'number', 'number', 'number', ]);

gr_setwsviewport = Module.cwrap('gr_setwsviewport', '', ['number', 'number', 'number', 'number', ]);

gr_createseg = Module.cwrap('gr_createseg', '', ['number', ]);

gr_copysegws = Module.cwrap('gr_copysegws', '', ['number', ]);

gr_redrawsegws = Module.cwrap('gr_redrawsegws', '', []);

gr_setsegtran = Module.cwrap('gr_setsegtran', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);

gr_closeseg = Module.cwrap('gr_closeseg', '', []);

gr_emergencyclosegks = Module.cwrap('gr_emergencyclosegks', '', []);

gr_updategks = Module.cwrap('gr_updategks', '', []);

gr_setspace = Module.cwrap('gr_setspace', 'number', ['number', 'number', 'number', 'number', ]);

gr_inqspace_c = Module.cwrap('gr_inqspace', '', ['number', 'number', 'number', 'number', ]);
gr_inqspace = function() {
    var _zmin = Module._malloc(8);
    var _zmax = Module._malloc(8);
    var _rotation = Module._malloc(4);
    var _tilt = Module._malloc(4);
    gr_inqspace_c(_zmin, _zmax, _rotation, _tilt);
    var result = new Array(4);
    result[0] = Module.HEAPF64.subarray(_zmin / 8, _zmin / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_zmax / 8, _zmax / 8 + 1)[0];
    result[2] = Module.HEAP32.subarray(_rotation / 4, _rotation / 4 + 1)[0];
    result[3] = Module.HEAP32.subarray(_tilt / 4, _tilt / 4 + 1)[0];
    freearray(_zmin);
    freearray(_zmax);
    freearray(_rotation);
    freearray(_tilt);
    return result;
};

gr_textext_c = Module.cwrap('gr_textext', 'number', ['number', 'number', 'number', ]);
gr_textext = function(x, y, string) {
    var _string = uint8array(string);
    gr_textext_c(x, y, _string);
    freearray(_string);
};

gr_inqtextext_c = Module.cwrap('gr_inqtextext', '', ['number', 'number', 'number', 'number', 'number', ]);
gr_inqtextext = function(x, y, string) {
    var _string = uint8array(string);
    var _tbx = Module._malloc(8*4);
    var _tby = Module._malloc(8*4);
    gr_inqtextext_c(x, y, _string, _tbx, _tby);
    var result = new Array(2);
    result[0] = Module.HEAPF64.subarray(_tbx / 8, _tbx / 8 + 4);
    result[0] = Array.prototype.slice.call(result[0]);
    result[1] = Module.HEAPF64.subarray(_tby / 8, _tby / 8 + 4);
    result[1] = Array.prototype.slice.call(result[1]);
    freearray(_string);
    freearray(_tbx);
    freearray(_tby);
    return result;
};

gr_axes = Module.cwrap('gr_axes', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', ]);

gr_grid = Module.cwrap('gr_grid', '', ['number', 'number', 'number', 'number', 'number', 'number', ]);

gr_verrorbars_c = Module.cwrap('gr_verrorbars', '', ['number', 'number', 'number', 'number', 'number', ]);
gr_verrorbars = function(n, px, py, e1, e2) {
    var _px = floatarray(px);
    var _py = floatarray(py);
    var _e1 = floatarray(e1);
    var _e2 = floatarray(e2);

    gr_verrorbars_c(n, _px, _py, _e1, _e2);
    freearray(_px);
    freearray(_py);
    freearray(_e1);
    freearray(_e2);
};

gr_herrorbars_c = Module.cwrap('gr_herrorbars', '', ['number', 'number', 'number', 'number', 'number', ]);
gr_herrorbars = function(n, px, py, e1, e2) {
    var _px = floatarray(px);
    var _py = floatarray(py);
    var _e1 = floatarray(e1);
    var _e2 = floatarray(e2);

    gr_herrorbars_c(n, _px, _py, _e1, _e2);
    freearray(_px);
    freearray(_py);
    freearray(_e1);
    freearray(_e2);
};

gr_polyline3d_c = Module.cwrap('gr_polyline3d', '', ['number', 'number', 'number', 'number', ]);
gr_polyline3d = function(n, px, py, pz) {
    var _px = floatarray(px);
    var _py = floatarray(py);
    var _pz = floatarray(pz);

    gr_polyline3d_c(n, _px, _py, _pz);
    freearray(_px);
    freearray(_py);
    freearray(_pz);
};

gr_axes3d = Module.cwrap('gr_axes3d', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);

gr_titles3d_c = Module.cwrap('gr_titles3d', '', ['number', 'number', 'number', ]);
gr_titles3d = function(x_title, y_title, z_title) {
    var _x_title = uint8array(x_title);
    var _y_title = uint8array(y_title);
    var _z_title = uint8array(z_title);
    gr_titles3d_c(_x_title, _y_title, _z_title);
    freearray(_x_title);
    freearray(_y_title);
    freearray(_z_title);
};

gr_surface_c = Module.cwrap('gr_surface', '', ['number', 'number', 'number', 'number', 'number', 'number', ]);
gr_surface = function(nx, ny, px, py, pz, option) {
    var _px = floatarray(px);
    var _py = floatarray(py);
    var _pz = floatarray(pz);
    gr_surface_c(nx, ny, _px, _py, _pz, option);
    freearray(_px);
    freearray(_py);
    freearray(_pz);
};

gr_contour_c = Module.cwrap('gr_contour', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);
gr_contour = function(nx, ny, nh, px, py, h, pz, major_h) {
    var _px = floatarray(px);
    var _py = floatarray(py);
    var _h = floatarray(h);
    var _pz = floatarray(pz);
    gr_contour_c(nx, ny, nh, _px, _py, _h, _pz, major_h);
    freearray(_px);
    freearray(_py);
    freearray(_h);
    freearray(_pz);
};

gr_hexbin_c = Module.cwrap('gr_hexbin', 'number', ['number', 'number', 'number', 'number', ]);
gr_hexbin = function(n, x, y, nbins) {
    var _x = floatarray(x);
    var _y = floatarray(y);
    cntmax = gr_hexbin_c(n, _x, _y, nbins);
    freearray(_x);
    freearray(_y);
    return cntmax;
};

gr_setcolormap = Module.cwrap('gr_setcolormap', '', ['number', ]);

gr_inqcolormap_c = Module.cwrap('gr_inqcolormap', '', ['number', ]);
gr_inqcolormap = function() {
    var _index = Module._malloc(4);
    gr_inqcolormap_c(_index);
    result = Module.HEAP32.subarray(_index / 4, _index / 4 + 1)[0];
    freearray(_index);
    return result;
};

gr_colorbar = Module.cwrap('gr_colorbar', '', []);

gr_inqcolor_c = Module.cwrap('gr_inqcolor', '', ['number', 'number', ]);
gr_inqcolor = function(color) {
    var _rgb = Module._malloc(4);
    gr_inqcolor_c(color, _rgb);
    result = Module.HEAP32.subarray(_rgb / 4, _rgb / 4 + 1)[0];
    freearray(_rgb);
    return result;
};

gr_inqcolorfromrgb = Module.cwrap('gr_inqcolorfromrgb', 'number', ['number', 'number', 'number', ]);

gr_hsvtorgb_c = Module.cwrap('gr_hsvtorgb', '', ['number', 'number', 'number', 'number', 'number', 'number', ]);
gr_hsvtorgb = function(h, s, v, r, g, b) {
    var _r = floatarray(r);
    var _g = floatarray(g);
    var _b = floatarray(b);
    gr_hsvtorgb_c(h, s, v, _r, _g, _b);
    freearray(_r);
    freearray(_g);
    freearray(_b);
};

gr_tick = Module.cwrap('gr_tick', 'number', ['number', 'number', ]);

gr_validaterange = Module.cwrap('gr_validaterange', 'number', ['number', 'number', ]);

gr_adjustrange_c = Module.cwrap('gr_adjustrange', '', ['number', 'number', ]);
gr_adjustrange = function(amin, amax) {
    var _amin = floatarray([amin]);
    var _amax = floatarray([amax]);
    gr_adjustrange_c(_amin, _amax);
    amin = Module.HEAPF64[_amin/8];
    amax = Module.HEAPF64[_amax/8];
    freearray(_amin);
    freearray(_amax);
    return [amin, amax];
};

gr_beginprint_c = Module.cwrap('gr_beginprint', '', ['number', ]);
gr_beginprint = function(pathname) {
    var _pathname = uint8array(pathname);
    gr_beginprint_c(_pathname);
    freearray(_pathname);
};

gr_beginprintext_c = Module.cwrap('gr_beginprintext', '', ['number', 'number', 'number', 'number', ]);
gr_beginprintext = function(pathname, mode, format, orientation) {
    var _pathname = uint8array(pathname);
    var _mode = uint8array(mode);
    var _format = uint8array(format);
    var _orientation = uint8array(orientation);
    gr_beginprintext_c(_pathname, _mode, _format, _orientation);
    freearray(_pathname);
    freearray(_mode);
    freearray(_format);
    freearray(_orientation);
};

gr_endprint = Module.cwrap('gr_endprint', '', []);


gr_ndctowc_c = Module.cwrap('gr_ndctowc', '', ['number', 'number', ]);
gr_ndctowc = function(x, y) {
    var __x = Module._malloc(8);
    var _x = Module.HEAPF64.subarray(__x / 8, __x / 8 + 1);
    _x[0] = x;
    var __y = Module._malloc(8);
    var _y = Module.HEAPF64.subarray(__y / 8, __y / 8 + 1);
    _y[0] = y;
    gr_ndctowc_c(__x, __y);
    result = new Array(2);
    result[0] = _x[0];
    result[1] = _y[0];
    freearray(__x);
    freearray(__y);
    return result;
};


gr_wctondc_c = Module.cwrap('gr_wctondc', '', ['number', 'number', ]);
gr_wctondc = function(x, y) {
    var __x = Module._malloc(8);
    var _x = Module.HEAPF64.subarray(__x / 8, __x / 8 + 1);
    _x[0] = x;
    var __y = Module._malloc(8);
    var _y = Module.HEAPF64.subarray(__y / 8, __y / 8 + 1);
    _y[0] = y;
    gr_wctondc_c(__x, __y);
    result = new Array(2);
    result[0] = _x[0];
    result[1] = _y[0];
    freearray(__x);
    freearray(__y);
    return result;
};

gr_drawrect = Module.cwrap('gr_drawrect', '', ['number', 'number', 'number', 'number', ]);

gr_fillrect = Module.cwrap('gr_fillrect', '', ['number', 'number', 'number', 'number', ]);

gr_drawarc = Module.cwrap('gr_drawarc', '', ['number', 'number', 'number', 'number', 'number', 'number', ]);

gr_fillarc = Module.cwrap('gr_fillarc', '', ['number', 'number', 'number', 'number', 'number', 'number', ]);

gr_drawpath_c = Module.cwrap('gr_drawpath', '', ['number', 'number', 'number', 'number', ]);
gr_drawpath = function(n, vertices, codes, fill) {
    _codes = uint8array(codes);
    gr_drawpath_c(n, _vertices, _codes, fill);
    freearray(_vertices);
    freearray(_codes);
};

gr_setarrowstyle = Module.cwrap('gr_setarrowstyle', '', ['number', ]);

gr_drawarrow = Module.cwrap('gr_drawarrow', '', ['number', 'number', 'number', 'number', ]);

gr_readimage_c = Module.cwrap('gr_readimage', 'number', ['number', 'number', 'number', 'number', ]);
gr_readimage = function(path) {
    var _path = uint8array(path);
    var _width = Module._malloc(4);
    var _height = Module._malloc(4);
    var _data = Module._malloc(4);
    gr_readimage_c(_path, _width, _height, _data);
    var result = new Array(3);
    result[0] = Module.HEAP32.subarray(_width / 4, _width / 4 + 1)[0];
    result[1] = Module.HEAP32.subarray(_height / 4, _height / 4 + 1)[0];
    result[2] = Module.HEAP32.subarray(_data / 4, _data / 4 + 1)[0];
    freearray(_path);
    freearray(_width);
    freearray(_height);
    freearray(_data);
    return result;
};

gr_drawimage_c = Module.cwrap('gr_drawimage', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ]);
gr_drawimage = function(xmin, xmax, ymin, ymax, width, height, data, model) {
    this.select_canvas();
    var _data = intarray(data);
    gr_drawimage_c(xmin, xmax, ymin, ymax, width, height, _data, model);
    freearray(_data);
};

gr_importgraphics_c = Module.cwrap('gr_importgraphics', 'number', ['number', ]);
gr_importgraphics = function(path) {
    var _path = uint8array(path);
    gr_importgraphics_c(_path);
    freearray(_path);
};

gr_setshadow = Module.cwrap('gr_setshadow', '', ['number', 'number', 'number', ]);

gr_settransparency = Module.cwrap('gr_settransparency', '', ['number', ]);

gr_setcoordxform = Module.cwrap('gr_setcoordxform', '', ['number', ]);

gr_begingraphics_c = Module.cwrap('gr_begingraphics', '', ['number', ]);
gr_begingraphics = function(path) {
    var _path = uint8array(path);
    gr_begingraphics_c(_path);
    freearray(_path);
};

gr_endgraphics = Module.cwrap('gr_endgraphics', '', []);

gr_drawgraphics_c = Module.cwrap('gr_drawgraphics', 'number', ['number', ]);
gr_drawgraphics = function(string) {
    _string = uint8array(string);
    gr_drawgraphics_c(_string);
    freearray(_string);
};

gr_mathtex_c = Module.cwrap('gr_mathtex', '', ['number', 'number', 'number', ]);
gr_mathtex = function(x, y, string) {
    _string = uint8array(string);
    gr_mathtex_c(x, y, _string);
    freearray(_string);
};

gr_beginselection = Module.cwrap('gr_beginselection', '', ['number', 'number', ]);

gr_endselection = Module.cwrap('gr_endselection', '', []);

gr_moveselection = Module.cwrap('gr_moveselection', '', ['number', 'number', ]);

gr_resizeselection = Module.cwrap('gr_resizeselection', '', ['number', 'number', 'number', ]);

gr_inqbbox_c = Module.cwrap('gr_inqbbox', '', ['number', 'number', 'number', 'number', ]);
gr_inqbbox = function() {
    var _xmin = Module._malloc(8);
    var _xmax = Module._malloc(8);
    var _ymin = Module._malloc(8);
    var _ymax = Module._malloc(8);
    gr_inqbbox_c(_xmin, _xmax, _ymin, _ymax);
    var result = new Array(4);
    result[0] = Module.HEAPF64.subarray(_xmin / 8, _xmin / 8 + 1)[0];
    result[1] = Module.HEAPF64.subarray(_xmax / 8, _xmax / 8 + 1)[0];
    result[2] = Module.HEAPF64.subarray(_ymin / 8, _ymin / 8 + 1)[0];
    result[3] = Module.HEAPF64.subarray(_ymax / 8, _ymax / 8 + 1)[0];
    freearray(_xmin);
    freearray(_xmax);
    freearray(_ymin);
    freearray(_ymax);
    return result;
};

gr_precision = Module.cwrap('gr_precision', 'number', []);

gr_setregenflags = Module.cwrap('gr_setregenflags', '', ['number', ]);

gr_inqregenflags = Module.cwrap('gr_inqregenflags', 'number', ['number', ]);

gr_savestate = Module.cwrap('gr_savestate', '', []);

gr_restorestate = Module.cwrap('gr_restorestate', '', []);

gr_selectcontext = Module.cwrap('gr_selectcontext', '', ['number', ]);

gr_destroycontext = Module.cwrap('gr_destroycontext', '', ['number', ]);

gr_uselinespec_c = Module.cwrap('gr_uselinespec', 'number', ['number', ]);
gr_uselinespec = function(string) {
    _string = uint8array(string);
    result = gr_uselinespec_c(_string);
    freearray(_string);
    return result;
};

gr_selntran = Module.cwrap('gr_selntran', '', ['number']);

grm_args_new_c = Module.cwrap('grm_args_new', 'number', []);
grm_args_new = function() {
    return grm_args_new_c();
};

grm_args_push_c = Module.cwrap('grm_args_push', 'number', ['number', 'string', 'string', 'number']);
grm_args_push = function(args, key, format, ...vals) {
    function strip_format(format) {
        return format.replaceAll("n", "");
    }

    function is_non_empty_string(str) {
        return Boolean(str);
    }

    function is_homogeneous_format(format) {
        var type = format[0];
        return RegExp("^" + type + "+$").test(format);
    }

    function stringarray(vals) {
        var arr;
        var i;

        arr = [];
        for (i = 0; i < vals.length; ++i) {
            arr.push(uint8array(vals[i]));
        }
        return intarray(arr);
    }

    function argsarray(vals) {
        return intarray(vals);
    }

    format = strip_format(format);
    if (!is_non_empty_string(format)) {
        throw new Error("The \"format\" parameter must not be an empty value!");
    }
    if (!is_homogeneous_format(format)) {
        throw new Error("The \"format\" parameter must be an homogenous string (e.g \"iii\", \"DD\")!");
    }

    var type_to_conversion_function = {
        "d": floatarray,
        "i": intarray,
        "s": stringarray,
        "a": argsarray,
    };

    var type = format[0];
    var arr;
    var i;

    if(type == type.toLowerCase()) {
        arr = type_to_conversion_function[type](vals);
        format = type.repeat(vals.length);
    } else {
        arr = [];
        for (i = 0; i < vals.length; ++i) {
            arr.push(vals[i].length, type_to_conversion_function[type.toLowerCase()](vals[i]));
        }
        arr = intarray(arr);
        format = ("n" + type).repeat(vals.length);
    }

    return grm_args_push_c(args, key, format, arr);
};

grm_merge_c = Module.cwrap('grm_merge', 'number', ['number']);
grm_merge = function(args) {
    return grm_merge_c(args);
};

grm_merge_named_c = Module.cwrap('grm_merge_named', 'number', ['number', 'number']);
grm_merge_named = function(args, identificator) {
    let bufferSize = Module.lengthBytesUTF8(identificator);
    let bufferPtr = Module._malloc(bufferSize + 1);
    Module.stringToUTF8(identificator, bufferPtr, bufferSize + 1);
    let result = grm_merge_named_c(args, bufferPtr);
    freearray(bufferPtr);
    return result;
};

grm_input_c = Module.cwrap('grm_input', 'number', ['number', 'number']);
grm_input = function(args, mouse_args) {
    return grm_input_c(args, mouse_args);
};

grm_args_delete_c = Module.cwrap('grm_args_delete', '', ['number']);
grm_args_delete = function(args) {
    grm_args_delete_c(args);
};

grm_read_c = Module.cwrap('grm_read', 'number', ['number', 'number']);
grm_read = function(args, string) {
    var bufferSize = Module.lengthBytesUTF8(string);
    var bufferPtr = Module._malloc(bufferSize + 1);
    Module.stringToUTF8(string, bufferPtr, bufferSize + 1);
    result = grm_read_c(args, bufferPtr);
    freearray(bufferPtr);
    return result;
};

grm_plot_c = Module.cwrap('grm_plot', '', ['number']);
grm_plot = function(args) {
    if (typeof args === 'undefined') {
        grm_plot_c(0);
    } else {
        grm_plot_c(args);
    }
};

grm_dump_json_c = Module.cwrap('grm_dump_json', 'number', ['number', 'number']);
grm_dump_json = function(args, file) {
    if (typeof file === 'undefined') {
        file = this.get_stdout();
    }
    return grm_dump_json_c(args, file);
};

grm_dump_c = Module.cwrap('grm_dump', 'number', ['number', 'number']);
grm_dump = function(args, file) {
    if (typeof file === 'undefined') {
        file = this.get_stdout();
    }
    return grm_dump_c(args, file);
};

grm_switch_c = Module.cwrap('grm_switch', 'number', ['number']);
grm_switch = function(id) {
    return grm_switch_c(id);
};

grm_get_stdout_c = Module.cwrap('grm_get_stdout', 'number', []);
grm_get_stdout = function() {
    return grm_get_stdout_c();
};

gr_shade_c = Module.cwrap('gr_shade', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']);
gr_shade = function(n, x, y, lines, xform, roi, w, h) {
    _x = floatarray(x);
    _y = floatarray(y);
    _roi = floatarray(roi);
    var _bins = Module._malloc(w * h * 4);
    gr_shade_c(n, _x, _y, lines, xform, _roi, w, h, _bins);
    var result = Module.HEAP32.subarray(_bins / 4, _bins / 4 + w * h);
    freearray(_bins);
    return result;
};

gr_shadepoints_c = Module.cwrap('gr_shadepoints', '', ['number', 'number', 'number', 'number', 'number', 'number']);
gr_shadepoints = function(n, x, y, xform, w, h) {
    _x = floatarray(x);
    _y = floatarray(y);
    gr_shadepoints_c(n, _x, _y, xform, w, h);
};

gr_shadelines_c = Module.cwrap('gr_shadelines', '', ['number', 'number', 'number', 'number', 'number', 'number']);
gr_shadelines = function(n, x, y, xform, w, h) {
    _x = floatarray(x);
    _y = floatarray(y);
    gr_shadelines_c(n, _x, _y, xform, w, h);
};

gr_panzoom_c = Module.cwrap('gr_panzoom', '', ['number', 'number', 'number', 'number', 'number', 'number', 'number']);
gr_panzoom = function(x, y, zoom, xmin, xmax, ymin, ymax) {
    var __xmin = Module._malloc(8);
    var _xmin = Module.HEAPF64.subarray(__xmin / 8, __xmin / 8 + 1);
    _xmin[0] = xmin;
    var __xmax = Module._malloc(8);
    var _xmax = Module.HEAPF64.subarray(__xmax / 8, __xmax / 8 + 1);
    _xmax[0] = xmax;
    var __ymin = Module._malloc(8);
    var _ymin = Module.HEAPF64.subarray(__ymin / 8, __ymin / 8 + 1);
    _ymin[0] = ymin;
    var __ymax = Module._malloc(8);
    var _ymax = Module.HEAPF64.subarray(__ymax / 8, __ymax / 8 + 1);
    _ymax[0] = ymax;
    gr_panzoom_c(x, y, zoom, __xmin, __xmax, __ymin, __ymax);
    result = new Array(4);
    result[0] = _xmin[0];
    result[1] = _xmax[0];
    result[2] = _ymin[0];
    result[3] = _ymax[0];
    freearray(__xmin);
    freearray(__xmax);
    freearray(__ymin);
    freearray(__ymax);
    return result;
};

gr_path_c = Module.cwrap('gr_path', '', ['number', 'number', 'number', 'number', ]);
gr_path = function(n, x, y, codes) {
    _x = floatarray(x);
    _y = floatarray(y);
    _codes = uint8array(codes);
    gr_path_c(n, _x, _y, _codes);
    freearray(_x);
    freearray(_y);
    freearray(_codes);
};

gr_setborderwidth = Module.cwrap('gr_setborderwidth', '', ['number', ]);

gr_inqborderwidth_c = Module.cwrap('gr_inqborderwidth', '', ['number', ]);
gr_inqborderwidth = function() {
    var _width = Module._malloc(8);
    gr_inqborderwidth_c(_width);
    result = Module.HEAPF64.subarray(_width / 8, _width / 8 + 1)[0];
    freearray(_width);
    return result;
};

gr_setbordercolorind = Module.cwrap('gr_setbordercolorind', '', ['number', ]);

gr_inqbordercolorind_c = Module.cwrap('gr_inqbordercolorind', '', ['number', ]);
gr_inqbordercolorind = function() {
    var _coli = Module._malloc(4);
    gr_inqbordercolorind_c(_coli);
    result = Module.HEAP32.subarray(_coli / 4, _coli / 4 + 1)[0];
    freearray(_coli);
    return result;
};

gr_selectclipxform = Module.cwrap('gr_selectclipxform', '', ['number', ]);

gr_inqclipxform_c = Module.cwrap('gr_inqclipxform', '', ['number', ]);
gr_inqclipxform = function() {
    var _tnr = Module._malloc(4);
    gr_inqclipxform_c(_tnr);
    result = Module.HEAP32.subarray(_tnr / 4, _tnr / 4 + 1)[0];
    freearray(_tnr);
    return result;
};

grm_get_box_c = Module.cwrap('grm_get_box', 'number', ['number', 'number', 'number', 'number', 'number']);
grm_get_box = function(top, right, bottom, left, keepAspectRatio) {
    var result = new Array(4);
    var _x = Module._malloc(4);
    var _y = Module._malloc(4);
    var _w = Module._malloc(4);
    var _h = Module._malloc(4);
    grm_get_box_c(top, right, bottom, left, keepAspectRatio, _x, _y, _w, _h);
    result[0] = Module.HEAP32.subarray(_x / 4, _x / 4 + 1)[0];
    result[1] = Module.HEAP32.subarray(_y / 4, _y / 4 + 1)[0];
    result[2] = Module.HEAP32.subarray(_w / 4, _w / 4 + 1)[0];
    result[3] = Module.HEAP32.subarray(_h / 4, _h / 4 + 1)[0];
    freearray(_x);
    freearray(_y);
    freearray(_w);
    freearray(_h);
    return result;
};

grm_get_tooltip_c = Module.cwrap('grm_get_tooltip', 'number', ['number', 'number']);
grm_get_tooltip = function(x, y) {
    var info = grm_get_tooltip_c(x, y);
    var data = {
        'x': Module.HEAPF64.subarray(info / 8, info / 8 + 1)[0],
        'y': Module.HEAPF64.subarray(info / 8 + 1, info / 8 + 2)[0],
        'xpx': Module.HEAP32.subarray(info / 4 + 4, info / 4 + 5)[0],
        'ypx': Module.HEAP32.subarray(info / 4 + 5, info / 4 + 6)[0],
        'xlabel':  Module.UTF8ToString(Module.HEAP32.subarray(info / 4 + 6, info / 4 + 7)[0]),
        'ylabel':  Module.UTF8ToString(Module.HEAP32.subarray(info / 4 + 7, info / 4 + 8)[0]),
        'label': Module.UTF8ToString(Module.HEAP32.subarray(info / 4 + 8, info / 4 + 9)[0])
    };
    freearray(info)
    return data;
};

grm_register_c = Module.cwrap('grm_register', 'number', ['number', 'number']);
grm_register = function(type, callback) {
    if (type <= 3) {
      this.callbacks[type] = callback;
    } else {
      console.error('gr.register: unknown event type:', type);
      return;
    }
};

grm_unregister_c = Module.cwrap('grm_unregister', 'number', ['number']);
grm_unregister = function(type) {
    Module.removeFunction(callbacks[type]);
    delete callbacks[type];
    return grm_unregister_c(type);
};

grm_dump_json_str_c = Module.cwrap('grm_dump_json_str', 'number', []);
grm_dump_json_str = function() {
  let str_p = grm_dump_json_str_c();
  let str = Module.UTF8ToString(str_p);
  freearray(str_p);
  return str;
};

grm_load_from_str_c = Module.cwrap('grm_load_from_str', 'number', ['number']);
grm_load_from_str = function(json_string) {
  let cstr = uint8array(json_string);
  let result = grm_load_from_str_c(cstr);
  freearray(cstr);
  return result;
};
