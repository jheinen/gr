/*
 * This code is derived from matplotlib's mathtext module.
 *
 * The code was rewritten in C and yacc, adapted to the GR framework's text and path rendering, and modified so that
 * its results resemble those of the previous gr_mathtex implementation which uses LaTeX directly.
 *
 * The original mathtext module can be found at:
 * https://github.com/matplotlib/matplotlib/blob/baaec371364deac71de24d8f564fb43f70db7297/lib/matplotlib/mathtext.py
 */

static const char *spaced_symbols[] = {":",
                                       "<",
                                       "=",
                                       ">",
                                       "\\Downarrow",
                                       "\\Join",
                                       "\\Leftarrow",
                                       "\\Leftrightarrow",
                                       "\\Longleftarrow",
                                       "\\Longleftrightarrow",
                                       "\\Longrightarrow",
                                       "\\Rightarrow",
                                       "\\Uparrow",
                                       "\\Updownarrow",
                                       "\\approx",
                                       "\\asymp",
                                       "\\bowtie",
                                       "\\cong",
                                       "\\dashv",
                                       "\\doteq",
                                       "\\doteqdot",
                                       "\\dotplus",
                                       "\\dots",
                                       "\\downarrow",
                                       "\\equiv",
                                       "\\frown",
                                       "\\geq",
                                       "\\gg",
                                       "\\hookleftarrow",
                                       "\\hookrightarrow",
                                       "\\in",
                                       "\\leadsto",
                                       "\\leftarrow",
                                       "\\leftharpoondown",
                                       "\\leftharpoonup",
                                       "\\leftrightarrow",
                                       "\\leq",
                                       "\\ll",
                                       "\\longleftarrow",
                                       "\\longleftrightarrow",
                                       "\\longmapsto",
                                       "\\longrightarrow",
                                       "\\mapsto",
                                       "\\mid",
                                       "\\models",
                                       "\\nearrow",
                                       "\\neq",
                                       "\\ni",
                                       "\\nwarrow",
                                       "\\parallel",
                                       "\\perp",
                                       "\\prec",
                                       "\\preceq",
                                       "\\propto",
                                       "\\rightarrow",
                                       "\\rightharpoondown",
                                       "\\rightharpoonup",
                                       "\\rightleftharpoons",
                                       "\\searrow",
                                       "\\sim",
                                       "\\simeq",
                                       "\\smile",
                                       "\\sqsubset",
                                       "\\sqsubseteq",
                                       "\\sqsupset",
                                       "\\sqsupseteq",
                                       "\\subset",
                                       "\\subseteq",
                                       "\\succ",
                                       "\\succeq",
                                       "\\supset",
                                       "\\supseteq",
                                       "\\swarrow",
                                       "\\to",
                                       "\\uparrow",
                                       "\\updownarrow",
                                       "\\vdash"};

static const char *binary_operators[] = {"*",
                                         "+",
                                         "-",
                                         "\\amalg",
                                         "\\ast",
                                         "\\bigcirc",
                                         "\\bigtriangledown",
                                         "\\bigtriangleup",
                                         "\\bullet",
                                         "\\cap",
                                         "\\cdot",
                                         "\\circ",
                                         "\\cup",
                                         "\\dagger",
                                         "\\ddagger",
                                         "\\diamond",
                                         "\\div",
                                         "\\lhd",
                                         "\\mp",
                                         "\\odot",
                                         "\\ominus",
                                         "\\oplus",
                                         "\\oslash",
                                         "\\otimes",
                                         "\\pm",
                                         "\\rhd",
                                         "\\setminus",
                                         "\\sqcap",
                                         "\\sqcup",
                                         "\\star",
                                         "\\times",
                                         "\\triangleleft",
                                         "\\triangleright",
                                         "\\unlhd",
                                         "\\unrhd",
                                         "\\uplus",
                                         "\\vee",
                                         "\\wedge",
                                         "\\wr"};

static const unsigned int binary_operator_codepoints[] = {'*',  '+',  '-',  8852,  '*',  9711, 9661, 9651,  8226,  8745,
                                                          183,  9675, 8746, 8224,  8225, 8900, 247,  9665,  8723,  8857,
                                                          8854, 8853, 248,  10754, 177,  9655, 8726, 10757, 10758, 8902,
                                                          215,  9665, 9655, 8884,  8885, 8846, 8744, 8743,  8768};

static const char *symbol_names[] = {"\\#",
                                     "\\$",
                                     "\\%",
                                     "\\AA",
                                     "\\AE",
                                     "\\BbbC",
                                     "\\BbbN",
                                     "\\BbbP",
                                     "\\BbbQ",
                                     "\\BbbR",
                                     "\\BbbZ",
                                     "\\Bumpeq",
                                     "\\Cap",
                                     "\\Colon",
                                     "\\Cup",
                                     "\\Delta",
                                     "\\Doteq",
                                     "\\Downarrow",
                                     "\\Equiv",
                                     "\\Finv",
                                     "\\Game",
                                     "\\Gamma",
                                     "\\H",
                                     "\\Im",
                                     "\\Join",
                                     "\\L",
                                     "\\Lambda",
                                     "\\Ldsh",
                                     "\\Leftarrow",
                                     "\\Leftrightarrow",
                                     "\\Lleftarrow",
                                     "\\Longleftarrow",
                                     "\\Longleftrightarrow",
                                     "\\Longrightarrow",
                                     "\\Lsh",
                                     "\\Nearrow",
                                     "\\Nwarrow",
                                     "\\O",
                                     "\\OE",
                                     "\\Omega",
                                     "\\P",
                                     "\\Phi",
                                     "\\Pi",
                                     "\\Psi",
                                     "\\Rdsh",
                                     "\\Re",
                                     "\\Rightarrow",
                                     "\\Rrightarrow",
                                     "\\Rsh",
                                     "\\S",
                                     "\\Searrow",
                                     "\\Sigma",
                                     "\\Subset",
                                     "\\Supset",
                                     "\\Swarrow",
                                     "\\Theta",
                                     "\\Thorn",
                                     "\\Uparrow",
                                     "\\Updownarrow",
                                     "\\Upsilon",
                                     "\\Vdash",
                                     "\\Vert",
                                     "\\Vvdash",
                                     "\\Xi",
                                     "\\_",
                                     "\\__sqrt__",
                                     "\\ac",
                                     "\\acute",
                                     "\\acwopencirclearrow",
                                     "\\adots",
                                     "\\ae",
                                     "\\aleph",
                                     "\\alpha",
                                     "\\angle",
                                     "\\approx",
                                     "\\approxeq",
                                     "\\approxident",
                                     "\\arceq",
                                     "\\ast",
                                     "\\asterisk",
                                     "\\asymp",
                                     "\\backcong",
                                     "\\backepsilon",
                                     "\\backprime",
                                     "\\backsim",
                                     "\\backsimeq",
                                     "\\backslash",
                                     "\\bar",
                                     "\\barleftarrow",
                                     "\\barwedge",
                                     "\\because",
                                     "\\beta",
                                     "\\beth",
                                     "\\between",
                                     "\\bigcap",
                                     "\\bigcirc",
                                     "\\bigcup",
                                     "\\bigodot",
                                     "\\bigoplus",
                                     "\\bigotimes",
                                     "\\bigstar",
                                     "\\bigtriangledown",
                                     "\\bigtriangleup",
                                     "\\biguplus",
                                     "\\bigvee",
                                     "\\bigwedge",
                                     "\\blacksquare",
                                     "\\blacktriangle",
                                     "\\blacktriangledown",
                                     "\\blacktriangleleft",
                                     "\\blacktriangleright",
                                     "\\bot",
                                     "\\bowtie",
                                     "\\boxbar",
                                     "\\boxdot",
                                     "\\boxminus",
                                     "\\boxplus",
                                     "\\boxtimes",
                                     "\\breve",
                                     "\\bullet",
                                     "\\bumpeq",
                                     "\\c",
                                     "\\candra",
                                     "\\cap",
                                     "\\carriagereturn",
                                     "\\cdot",
                                     "\\cdotp",
                                     "\\cdots",
                                     "\\cent",
                                     "\\check",
                                     "\\checkmark",
                                     "\\chi",
                                     "\\circ",
                                     "\\circeq",
                                     "\\circlearrowleft",
                                     "\\circlearrowright",
                                     "\\circledR",
                                     "\\circledS",
                                     "\\circledast",
                                     "\\circledcirc",
                                     "\\circleddash",
                                     "\\circumflexaccent",
                                     "\\clubsuit",
                                     "\\clubsuitopen",
                                     "\\colon",
                                     "\\coloneq",
                                     "\\combiningacuteaccent",
                                     "\\combiningbreve",
                                     "\\combiningdiaeresis",
                                     "\\combiningdotabove",
                                     "\\combininggraveaccent",
                                     "\\combiningoverline",
                                     "\\combiningrightarrowabove",
                                     "\\combiningtilde",
                                     "\\complement",
                                     "\\cong",
                                     "\\coprod",
                                     "\\copyright",
                                     "\\cup",
                                     "\\cupdot",
                                     "\\curlyeqprec",
                                     "\\curlyeqsucc",
                                     "\\curlyvee",
                                     "\\curlywedge",
                                     "\\curvearrowleft",
                                     "\\curvearrowright",
                                     "\\cwopencirclearrow",
                                     "\\d",
                                     "\\dag",
                                     "\\dagger",
                                     "\\daleth",
                                     "\\danger",
                                     "\\dashleftarrow",
                                     "\\dashrightarrow",
                                     "\\dashv",
                                     "\\ddag",
                                     "\\ddddot",
                                     "\\dddot",
                                     "\\ddot",
                                     "\\ddots",
                                     "\\degree",
                                     "\\delta",
                                     "\\diamond",
                                     "\\diamondsuit",
                                     "\\digamma",
                                     "\\div",
                                     "\\divideontimes",
                                     "\\dot",
                                     "\\doteq",
                                     "\\doteqdot",
                                     "\\dotminus",
                                     "\\dotplus",
                                     "\\dots",
                                     "\\doublebarwedge",
                                     "\\downarrow",
                                     "\\downdownarrows",
                                     "\\downharpoonleft",
                                     "\\downharpoonright",
                                     "\\downzigzagarrow",
                                     "\\ell",
                                     "\\emdash",
                                     "\\emptyset",
                                     "\\endash",
                                     "\\epsilon",
                                     "\\eqcirc",
                                     "\\eqcolon",
                                     "\\eqdef",
                                     "\\eqgtr",
                                     "\\eqless",
                                     "\\eqsim",
                                     "\\eqslantgtr",
                                     "\\eqslantless",
                                     "\\equal",
                                     "\\equiv",
                                     "\\eta",
                                     "\\eth",
                                     "\\exists",
                                     "\\fallingdotseq",
                                     "\\flat",
                                     "\\forall",
                                     "\\frakC",
                                     "\\frakZ",
                                     "\\frown",
                                     "\\gamma",
                                     "\\geq",
                                     "\\geqq",
                                     "\\geqslant",
                                     "\\gg",
                                     "\\ggg",
                                     "\\gimel",
                                     "\\gnapprox",
                                     "\\gneqq",
                                     "\\gnsim",
                                     "\\grave",
                                     "\\greater",
                                     "\\gtrapprox",
                                     "\\gtrdot",
                                     "\\gtreqless",
                                     "\\gtreqqless",
                                     "\\gtrless",
                                     "\\gtrsim",
                                     "\\guillemotleft",
                                     "\\guillemotright",
                                     "\\guilsinglleft",
                                     "\\guilsinglright",
                                     "\\hat",
                                     "\\hbar",
                                     "\\heartsuit",
                                     "\\hookleftarrow",
                                     "\\hookrightarrow",
                                     "\\hslash",
                                     "\\i",
                                     "\\iiint",
                                     "\\iint",
                                     "\\imageof",
                                     "\\imath",
                                     "\\in",
                                     "\\infty",
                                     "\\int",
                                     "\\intercal",
                                     "\\invnot",
                                     "\\iota",
                                     "\\jmath",
                                     "\\k",
                                     "\\kappa",
                                     "\\kernelcontraction",
                                     "\\l",
                                     "\\lambda",
                                     "\\lambdabar",
                                     "\\langle",
                                     "\\lasp",
                                     "\\lbrace",
                                     "\\lbrack",
                                     "\\lceil",
                                     "\\ldots",
                                     "\\leadsto",
                                     "\\leftangle",
                                     "\\leftarrow",
                                     "\\leftarrowtail",
                                     "\\leftbrace",
                                     "\\leftharpoonaccent",
                                     "\\leftharpoondown",
                                     "\\leftharpoonup",
                                     "\\leftleftarrows",
                                     "\\leftparen",
                                     "\\leftrightarrow",
                                     "\\leftrightarrows",
                                     "\\leftrightharpoons",
                                     "\\leftrightsquigarrow",
                                     "\\leftsquigarrow",
                                     "\\leftthreetimes",
                                     "\\leq",
                                     "\\leqq",
                                     "\\leqslant",
                                     "\\less",
                                     "\\lessapprox",
                                     "\\lessdot",
                                     "\\lesseqgtr",
                                     "\\lesseqqgtr",
                                     "\\lessgtr",
                                     "\\lesssim",
                                     "\\lfloor",
                                     "\\ll",
                                     "\\llcorner",
                                     "\\lll",
                                     "\\lnapprox",
                                     "\\lneqq",
                                     "\\lnsim",
                                     "\\longleftarrow",
                                     "\\longleftrightarrow",
                                     "\\longmapsto",
                                     "\\longrightarrow",
                                     "\\looparrowleft",
                                     "\\looparrowright",
                                     "\\lq",
                                     "\\lrcorner",
                                     "\\ltimes",
                                     "\\macron",
                                     "\\maltese",
                                     "\\mapsdown",
                                     "\\mapsfrom",
                                     "\\mapsto",
                                     "\\mapsup",
                                     "\\measeq",
                                     "\\measuredangle",
                                     "\\mho",
                                     "\\mid",
                                     "\\minus",
                                     "\\models",
                                     "\\mp",
                                     "\\mu",
                                     "\\multimap",
                                     "\\nLeftarrow",
                                     "\\nLeftrightarrow",
                                     "\\nRightarrow",
                                     "\\nVDash",
                                     "\\nVdash",
                                     "\\nabla",
                                     "\\napprox",
                                     "\\natural",
                                     "\\ncong",
                                     "\\ne",
                                     "\\nearrow",
                                     "\\neg",
                                     "\\neq",
                                     "\\nequiv",
                                     "\\nexists",
                                     "\\ngeq",
                                     "\\ngtr",
                                     "\\ni",
                                     "\\nleftarrow",
                                     "\\nleftrightarrow",
                                     "\\nleq",
                                     "\\nless",
                                     "\\nmid",
                                     "\\not",
                                     "\\notin",
                                     "\\nparallel",
                                     "\\nprec",
                                     "\\nrightarrow",
                                     "\\nsim",
                                     "\\nsime",
                                     "\\nsubset",
                                     "\\nsubseteq",
                                     "\\nsucc",
                                     "\\nsupset",
                                     "\\nsupseteq",
                                     "\\ntriangleleft",
                                     "\\ntrianglelefteq",
                                     "\\ntriangleright",
                                     "\\ntrianglerighteq",
                                     "\\nu",
                                     "\\nvDash",
                                     "\\nvdash",
                                     "\\nwarrow",
                                     "\\o",
                                     "\\obar",
                                     "\\ocirc",
                                     "\\odot",
                                     "\\oe",
                                     "\\oiiint",
                                     "\\oiint",
                                     "\\oint",
                                     "\\omega",
                                     "\\ominus",
                                     "\\oplus",
                                     "\\origof",
                                     "\\oslash",
                                     "\\otimes",
                                     "\\overarc",
                                     "\\overleftarrow",
                                     "\\overleftrightarrow",
                                     "\\parallel",
                                     "\\partial",
                                     "\\perp",
                                     "\\perthousand",
                                     "\\phi",
                                     "\\pi",
                                     "\\pitchfork",
                                     "\\plus",
                                     "\\pm",
                                     "\\prec",
                                     "\\precapprox",
                                     "\\preccurlyeq",
                                     "\\preceq",
                                     "\\precnapprox",
                                     "\\precnsim",
                                     "\\precsim",
                                     "\\prime",
                                     "\\prod",
                                     "\\propto",
                                     "\\prurel",
                                     "\\psi",
                                     "\\quad",
                                     "\\questeq",
                                     "\\rangle",
                                     "\\rasp",
                                     "\\rbrace",
                                     "\\rbrack",
                                     "\\rceil",
                                     "\\rfloor",
                                     "\\rho",
                                     "\\rightangle",
                                     "\\rightarrow",
                                     "\\rightarrowbar",
                                     "\\rightarrowtail",
                                     "\\rightbrace",
                                     "\\rightharpoonaccent",
                                     "\\rightharpoondown",
                                     "\\rightharpoonup",
                                     "\\rightleftarrows",
                                     "\\rightleftharpoons",
                                     "\\rightparen",
                                     "\\rightrightarrows",
                                     "\\rightsquigarrow",
                                     "\\rightthreetimes",
                                     "\\rightzigzagarrow",
                                     "\\ring",
                                     "\\risingdotseq",
                                     "\\rq",
                                     "\\rtimes",
                                     "\\scrB",
                                     "\\scrE",
                                     "\\scrF",
                                     "\\scrH",
                                     "\\scrI",
                                     "\\scrL",
                                     "\\scrM",
                                     "\\scrR",
                                     "\\scre",
                                     "\\scrg",
                                     "\\scro",
                                     "\\scurel",
                                     "\\searrow",
                                     "\\sharp",
                                     "\\sigma",
                                     "\\sim",
                                     "\\simeq",
                                     "\\slash",
                                     "\\smallsetminus",
                                     "\\smile",
                                     "\\solbar",
                                     "\\spadesuit",
                                     "\\spadesuitopen",
                                     "\\sphericalangle",
                                     "\\sqcap",
                                     "\\sqcup",
                                     "\\sqsubset",
                                     "\\sqsubseteq",
                                     "\\sqsupset",
                                     "\\sqsupseteq",
                                     "\\ss",
                                     "\\star",
                                     "\\stareq",
                                     "\\sterling",
                                     "\\subset",
                                     "\\subseteq",
                                     "\\subseteqq",
                                     "\\subsetneq",
                                     "\\subsetneqq",
                                     "\\succ",
                                     "\\succapprox",
                                     "\\succcurlyeq",
                                     "\\succeq",
                                     "\\succnapprox",
                                     "\\succnsim",
                                     "\\succsim",
                                     "\\sum",
                                     "\\supset",
                                     "\\supseteq",
                                     "\\supseteqq",
                                     "\\supsetneq",
                                     "\\supsetneqq",
                                     "\\swarrow",
                                     "\\t",
                                     "\\tau",
                                     "\\textasciiacute",
                                     "\\textasciicircum",
                                     "\\textasciigrave",
                                     "\\textasciitilde",
                                     "\\textexclamdown",
                                     "\\textquestiondown",
                                     "\\textquotedblleft",
                                     "\\textquotedblright",
                                     "\\therefore",
                                     "\\theta",
                                     "\\thickspace",
                                     "\\thorn",
                                     "\\tilde",
                                     "\\times",
                                     "\\to",
                                     "\\top",
                                     "\\triangledown",
                                     "\\triangleeq",
                                     "\\triangleleft",
                                     "\\trianglelefteq",
                                     "\\triangleq",
                                     "\\triangleright",
                                     "\\trianglerighteq",
                                     "\\turnednot",
                                     "\\twoheaddownarrow",
                                     "\\twoheadleftarrow",
                                     "\\twoheadrightarrow",
                                     "\\twoheaduparrow",
                                     "\\ulcorner",
                                     "\\underbar",
                                     "\\uparrow",
                                     "\\updownarrow",
                                     "\\updownarrowbar",
                                     "\\updownarrows",
                                     "\\upharpoonleft",
                                     "\\upharpoonright",
                                     "\\uplus",
                                     "\\upsilon",
                                     "\\upuparrows",
                                     "\\urcorner",
                                     "\\vDash",
                                     "\\varDelta",
                                     "\\varGamma",
                                     "\\varLambda",
                                     "\\varOmega",
                                     "\\varPhi",
                                     "\\varPi",
                                     "\\varPsi",
                                     "\\varSigma",
                                     "\\varTheta",
                                     "\\varUpsilon",
                                     "\\varXi",
                                     "\\varepsilon",
                                     "\\varkappa",
                                     "\\varnothing",
                                     "\\varphi",
                                     "\\varpi",
                                     "\\varpropto",
                                     "\\varrho",
                                     "\\varsigma",
                                     "\\vartheta",
                                     "\\vartriangle",
                                     "\\vartriangleleft",
                                     "\\vartriangleright",
                                     "\\vdash",
                                     "\\vdots",
                                     "\\vec",
                                     "\\vee",
                                     "\\veebar",
                                     "\\veeeq",
                                     "\\vert",
                                     "\\wedge",
                                     "\\wedgeq",
                                     "\\widebar",
                                     "\\widehat",
                                     "\\widetilde",
                                     "\\wp",
                                     "\\wr",
                                     "\\xi",
                                     "\\yen",
                                     "\\zeta",
                                     "\\{",
                                     "\\|",
                                     "\\}"};

static const unsigned int symbol_codepoints[] = {
    35,     36,     37,     197,    198,    8450,   8469,   8473,   8474,  8477,  8484,  8782,  8914,   8759,   8915,
    916,    8785,   8659,   8803,   8498,   8513,   915,    779,    8465,  10781, 321,   923,   8626,   8656,   8660,
    8666,   10232,  10234,  10233,  8624,   8663,   8662,   216,    338,   937,   182,   934,   928,    936,    8627,
    8476,   8658,   8667,   8625,   167,    8664,   931,    8912,   8913,  8665,  920,   222,   8657,   8661,   933,
    8873,   8214,   8874,   926,    95,     8730,   8766,   769,    8634,  8944,  230,   8501,  945,    8736,   8776,
    8778,   8779,   8792,   8727,   42,     8781,   8780,   1014,   8245,  8765,  8909,  92,    772,    8676,   8892,
    8757,   946,    8502,   8812,   8898,   9675,   8899,   10752,  10753, 10754, 9733,  9661,  9651,   10756,  8897,
    8896,   9632,   9652,   9662,   9664,   9654,   8869,   8904,   9707,  8865,  8863,  8862,  8864,   774,    8729,
    8783,   807,    784,    8745,   8629,   8901,   183,    8943,   162,   780,   10003, 967,   8728,   8791,   8634,
    8635,   174,    9416,   8859,   8858,   8861,   770,    9827,   9831,  58,    8788,  769,   774,    776,    775,
    768,    772,    8407,   771,    8705,   8773,   8720,   169,    8746,  8845,  8926,  8927,  8910,   8911,   8630,
    8631,   8635,   803,    8224,   8224,   8504,   9761,   10510,  10511, 8867,  8225,  8412,  8411,   776,    8945,
    176,    948,    8900,   9826,   989,    247,    8903,   775,    8784,  8785,  8760,  8724,  8230,   8966,   8595,
    8650,   8643,   8642,   8623,   8467,   8212,   8709,   8211,   949,   8790,  8789,  8797,  8925,   8924,   8770,
    10902,  10901,  61,     8801,   951,    240,    8707,   8786,   9837,  8704,  8493,  8488,  8994,   947,    8805,
    8807,   10878,  8811,   8921,   8503,   10938,  8809,   8935,   768,   62,    10886, 8919,  8923,   10892,  8823,
    8819,   171,    187,    8249,   8250,   770,    8463,   9825,   8617,  8618,  8463,  305,   8749,   8748,   8887,
    305,    8712,   8734,   8747,   8890,   8976,   953,    567,    808,   954,   8763,  322,   955,    411,    10216,
    701,    123,    91,     8968,   8230,   8669,   10216,  8592,   8610,  124,   8400,  8637,  8636,   8647,   40,
    8596,   8646,   8651,   8621,   8604,   8907,   8804,   8806,   10877, 60,    10885, 8918,  8922,   10891,  8822,
    8818,   8970,   8810,   8990,   8920,   10937,  8808,   8934,   10229, 10231, 10236, 10230, 8619,   8620,   8216,
    8991,   8905,   175,    10016,  8615,   8612,   8614,   8613,   8798,  8737,  8487,  8739,  8722,   8871,   8723,
    956,    8888,   8653,   8654,   8655,   8879,   8878,   8711,   8777,  9838,  8775,  8800,  8599,   172,    8800,
    8802,   8708,   8817,   8815,   8715,   8602,   8622,   8816,   8814,  8740,  824,   8713,  8742,   8832,   8603,
    8769,   8772,   8836,   8840,   8833,   8837,   8841,   8938,   8940,  8939,  8941,  957,   8877,   8876,   8598,
    248,    9021,   778,    8857,   339,    8752,   8751,   8750,   969,   8854,  8853,  8886,  8856,   8855,   785,
    8406,   8417,   8741,   8706,   10178,  8240,   966,    960,    8916,  43,    177,   8826,  10935,  8828,   8828,
    10937,  8936,   8830,   8242,   8719,   8733,   8880,   968,    8195,  8799,  10217, 700,   125,    93,     8969,
    8971,   961,    10217,  8594,   8677,   8611,   125,    8401,   8641,  8640,  8644,  8652,  41,     8649,   8605,
    8908,   8669,   730,    8787,   8217,   8906,   8492,   8496,   8497,  8459,  8464,  8466,  8499,   8475,   8495,
    8458,   8500,   8881,   8600,   9839,   963,    8764,   8771,   8725,  8726,  8995,  9023,  9824,   9828,   8738,
    8851,   8852,   8847,   8849,   8848,   8850,   223,    8902,   8795,  163,   8834,  8838,  10949,  8842,   10955,
    8827,   10936,  8829,   8829,   10938,  8937,   8831,   8721,   8835,  8839,  10950, 8843,  10956,  8601,   865,
    964,    180,    94,     96,     126,    161,    191,    8220,   8221,  8756,  952,   8197,  254,    771,    215,
    8594,   8868,   9663,   8796,   9665,   8884,   8796,   9655,   8885,  8985,  8609,  8606,  8608,   8607,   8988,
    817,    8593,   8597,   8616,   8645,   8639,   8638,   8846,   965,   8648,  8989,  8872,  120549, 120548, 120556,
    120570, 120567, 120561, 120569, 120564, 120553, 120566, 120559, 949,   1008,  8709,  981,   982,    8733,   1009,
    962,    977,    9653,   8882,   8883,   8866,   8942,   8407,   8744,  8891,  8794,  124,   8743,   8793,   773,
    770,    771,    8472,   8768,   958,    165,    950,    123,    124,   125};


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include "gr.h"
#include "gks.h"
#include "gkscore.h"

#ifndef isnan
#define isnan(x) ((x) != (x))
#endif
#ifndef isinf
#define isinf(x) (!isnan(x) && isnan((x) - (x)))
#endif
#ifndef INFINITY
#define INFINITY (1.0 / 0.0)
#endif
#ifndef NAN
#define NAN (0.0 / 0.0)
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef round
#define round(x) (((x) < 0) ? ceil((x)-.5) : floor((x) + .5))
#endif

#include "mathtex2.h"

#define SHRINK_FACTOR 0.7
#define NUM_SIZE_LEVELS 2

#define MATH_FONT 232

typedef enum FontVariant_
{
  FV_CAL = 0,
  FV_RM = 1,
  FV_IT = 2,
  FV_TT = 3,
  FV_FRAK = 4,
  FV_BB = 5,
  FV_BF = 6,
} FontVariant;

static unsigned int get_codepoint_for_character_variant(unsigned int codepoint, FontVariant variant);

typedef struct BoxModelState_
{
  size_t index;
  size_t previous_state_index;
  FontVariant font;
  double fontsize;
  unsigned int dpi;
} BoxModelState;

typedef enum BoxModelNodeType_
{
  BT_KERN = 0,
  BT_HLIST = 1,
  BT_HLIST_ENTRY = 2,
  BT_CHAR = 3,
  BT_VLIST = 4,
  BT_VLIST_ENTRY = 5,
  BT_HRULE = 6,
  BT_GLUE = 7,
  BT_HBOX = 8,
  BT_VBOX = 9,
} BoxModelNodeType;

typedef enum GlueType_
{
  GT_FIL = 0,
  GT_FILL = 1,
  GT_FILLL = 2,
  GT_NEG_FIL = 3,
  GT_NEG_FILL = 4,
  GT_NEG_FILLL = 5,
  GT_EMPTY = 6,
  GT_SS = 7
} GlueType;

typedef struct GlueSpec_
{
  double width;
  double stretch;
  int stretch_order;
  double shrink;
  int shrink_order;
} GlueSpec;

static const GlueSpec glue_specs[] = {{0., 1., 1, 0., 0}, {0., 1., 2, 0., 0}, {0., 1., 3, 0., 0}, {0., 0., 0, 1., 1},
                                      {0., 0., 0, 1., 2}, {0., 0., 0, 1., 3}, {0., 0., 0, 0., 0}, {0., 1., 1, -1., 1}};

typedef struct BoxModelKernNode_
{
  int is_auto;
  double width;
} BoxModelKernNode;

typedef struct BoxModelGlueNode_
{
  double factor;
  const GlueSpec *spec;
} BoxModelGlueNode;

typedef struct BoxModelHListNode_
{
  size_t first_entry_index;
  double width;
  double height;
  double depth;
  double shift_amount;
  double glue_set;
  int glue_sign;
  int glue_order;
  double subsuper_width;
  double subsuper_height;
  double subsuper_depth;
  double subsuper_kern_index;
  double subsuper_hlist_index;
  int subsuper_is_overunder;
  const char *function_name_start;
  size_t function_name_length;
} BoxModelHListNode;

typedef struct BoxModelHRuleNode_
{
  double width;
  double height;
  double depth;
} BoxModelHRuleNode;

typedef struct BoxModelHBoxNode_
{
  double width;
} BoxModelHBoxNode;

typedef struct BoxModelVBoxNode_
{
  double height;
  double depth;
} BoxModelVBoxNode;

typedef struct BoxModelHListEntryNode_
{
  size_t next_entry_index;
  size_t node_index;
} BoxModelHListEntryNode;

typedef struct BoxModelVListNode_
{
  size_t first_entry_index;
  double width;
  double height;
  double depth;
  double shift_amount;
  double glue_set;
  int glue_sign;
  int glue_order;
  const char *function_name_start;
  size_t function_name_length;
} BoxModelVListNode;

typedef struct BoxModelVListEntryNode_
{
  size_t next_entry_index;
  size_t node_index;
} BoxModelVListEntryNode;

typedef struct BoxModelCharNode_
{
  unsigned int codepoint;
  double width;
  double height;
  double depth;
  double shift_amount;
  double advance;
  double bearing;
  BoxModelState state;
} BoxModelCharNode;


typedef struct BoxModelNode_
{
  size_t index;
  BoxModelNodeType type;
  int size;
  union
  {
    BoxModelKernNode kern;
    BoxModelHListNode hlist;
    BoxModelHListEntryNode hlist_entry;
    BoxModelVListNode vlist;
    BoxModelVListEntryNode vlist_entry;
    BoxModelCharNode character;
    BoxModelHRuleNode hrule;
    BoxModelGlueNode glue;
    BoxModelHBoxNode hbox;
    BoxModelVBoxNode vbox;
  } u;
} BoxModelNode;


double canvas_height = 0;
double canvas_width = 0;

static double font_size;
static double transformation[6];
static double window[4];

const char *input;
extern const char *cursor;
extern enum State state;
extern const char *symbol_start;
extern int ignore_whitespace;

int yyparse();

int has_parser_error = 0;

void yyerror(char const *s)
{
  fprintf(stderr, "ERROR: %s\n", s);
  has_parser_error = 1;
}

size_t result_box_model_node_index;

#define VALUE_TYPE ParserNode
#define VALUE_NAME parser_node

#include "tempbuffer.inl"

#define VALUE_TYPE BoxModelNode
#define VALUE_NAME box_model_node

#include "tempbuffer.inl"

#define VALUE_TYPE BoxModelState
#define VALUE_NAME box_model_state

#include "tempbuffer.inl"

size_t current_box_model_state_index = 0;
size_t result_parser_node_index;

static void push_state();

static BoxModelState *get_current_state()
{
  if (current_box_model_state_index == 0)
    {
      /* ensure that the state stack is not empty */
      push_state();
    }
  return get_box_model_state(current_box_model_state_index);
}

static void push_state()
{
  BoxModelState new_state;
  if (current_box_model_state_index)
    {
      new_state = *get_current_state();
    }
  else
    {
      new_state.index = 0;
      new_state.previous_state_index = 0;
      new_state.fontsize = font_size;
      new_state.font = FV_IT;
      new_state.dpi = 72;
    }
  new_state.previous_state_index = current_box_model_state_index;
  current_box_model_state_index = copy_box_model_state(new_state);
}

static void pop_state()
{
  if (current_box_model_state_index == 0)
    {
      /* the state stack is already empty */
      return;
    }
  if (current_box_model_state_index == box_model_state_next_index_)
    {
      /* if possible, mark this state as unused in the tempbuffer */
      box_model_state_next_index_ -= 1;
    }
  current_box_model_state_index = get_current_state()->previous_state_index;
}

static double get_em_width(BoxModelState bm_state)
{
  double advance;
  double bearing;
  if (gks_ft_get_metrics(MATH_FONT, bm_state.fontsize * 1.16, 'm', bm_state.dpi, NULL, NULL, NULL, &advance, &bearing,
                         NULL, NULL, NULL, NULL))
    {
      return advance - bearing;
    }
  else
    {
      return 0.0;
    }
}

static double get_underline_thickness(FontVariant font, double fontsize, double dpi)
{
  return ((0.75 / 12.0) * fontsize * dpi) / 72.0;
}

static size_t make_kern(double width)
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_KERN;
  bm_node.size = 0;
  bm_node.u.kern.is_auto = 0;
  bm_node.u.kern.width = width;
  return copy_box_model_node(bm_node);
}

static size_t make_space(double percentage)
{
  return make_kern(get_em_width(*get_current_state()) * percentage);
}


static size_t make_char(unsigned int codepoint)
{
  if (codepoint == 0)
    {
      return 0;
    }
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_CHAR;
  bm_node.u.character.codepoint = codepoint;
  bm_node.size = 0;
  bm_node.u.character.state = *get_current_state();
  double width, height, depth, advance, bearing;
  if (gks_ft_get_metrics(
          MATH_FONT, bm_node.u.character.state.fontsize * 1.16,
          get_codepoint_for_character_variant(bm_node.u.character.codepoint, bm_node.u.character.state.font),
          bm_node.u.character.state.dpi, &width, &height, &depth, &advance, &bearing, NULL, NULL, NULL, NULL))
    {
      if (codepoint == ' ')
        {
          bm_node.u.character.width = advance;
        }
      else
        {
          bm_node.u.character.width = width;
        }
      bm_node.u.character.height = height;
      bm_node.u.character.depth = depth;
      bm_node.u.character.advance = advance;
      bm_node.u.character.bearing = 0.0;
      if (codepoint == '|')
        {
          bm_node.u.character.advance = advance * 1.5;
        }
      else if (codepoint == 215)
        {
          bm_node.u.character.advance = width * 1.25;
        }
    }
  else
    {
      bm_node.u.character.width = 0.0;
      bm_node.u.character.height = 0.0;
      bm_node.u.character.depth = 0.0;
      bm_node.u.character.advance = 0.0;
      bm_node.u.character.bearing = 0.0;
      fprintf(stderr, "Missing character %c / %u\n", codepoint, codepoint);
    }
  bm_node.u.character.shift_amount = 0.0;
  return copy_box_model_node(bm_node);
}

static void shrink(size_t node_index);

static size_t make_accent(unsigned int c, double bearing_factor)
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_CHAR;
  bm_node.u.character.codepoint = c;
  bm_node.size = 0;
  bm_node.u.character.state = *get_current_state();
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  double bearing;
  if (gks_ft_get_metrics(
          MATH_FONT, bm_node.u.character.state.fontsize * 1.16,
          get_codepoint_for_character_variant(bm_node.u.character.codepoint, bm_node.u.character.state.font),
          bm_node.u.character.state.dpi, NULL, NULL, NULL, NULL, &bearing, &xmin, &xmax, &ymin, &ymax))
    {
      bm_node.u.character.width = xmax - xmin;
      bm_node.u.character.height = ymax - ymin;
      bm_node.u.character.depth = 0.0;
      bm_node.u.character.advance = bm_node.u.character.width;
      bm_node.u.character.bearing = bearing_factor * bearing;
      bm_node.u.character.shift_amount = -ymin;
    }
  else
    {
      bm_node.u.character.width = 0.0;
      bm_node.u.character.height = 0.0;
      bm_node.u.character.depth = 0.0;
      bm_node.u.character.advance = 0.0;
      bm_node.u.character.bearing = 0.0;
      bm_node.u.character.shift_amount = 0.0;
      fprintf(stderr, "Missing character %c / %u\n", c, c);
    }
  size_t bm_node_index = copy_box_model_node(bm_node);
  shrink(bm_node_index);
  return bm_node_index;
}
static size_t make_hrule_with_width(double thickness, double width)
{
  BoxModelNode bm_node;
  if (isnan(thickness))
    {
      BoxModelState *bm_state = get_current_state();
      thickness = get_underline_thickness(bm_state->font, bm_state->fontsize, bm_state->dpi);
    }
  bm_node.index = 0;
  bm_node.type = BT_HRULE;
  bm_node.size = 0;
  bm_node.u.hrule.width = width;
  bm_node.u.hrule.height = thickness * 0.5;
  bm_node.u.hrule.depth = thickness * 0.5;
  return copy_box_model_node(bm_node);
}


static size_t make_hrule(double thickness)
{
  return make_hrule_with_width(thickness, INFINITY);
}

static size_t make_hbox(double width)
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_HBOX;
  bm_node.size = 0;
  bm_node.u.hbox.width = width;
  return copy_box_model_node(bm_node);
}

static size_t make_vbox(double height, double depth)
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_VBOX;
  bm_node.size = 0;
  bm_node.u.vbox.height = height;
  bm_node.u.vbox.depth = depth;
  return copy_box_model_node(bm_node);
}

static size_t make_glue(GlueType type)
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_GLUE;
  bm_node.size = 0;
  bm_node.u.glue.factor = 1;
  bm_node.u.glue.spec = &glue_specs[type];
  return copy_box_model_node(bm_node);
}

static size_t make_vlist()
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_VLIST;
  bm_node.size = 0;
  bm_node.u.vlist.width = 0.0;
  bm_node.u.vlist.height = 0.0;
  bm_node.u.vlist.depth = 0.0;
  bm_node.u.vlist.shift_amount = 0.0;
  bm_node.u.vlist.glue_set = 0.0;
  bm_node.u.vlist.glue_sign = 0;
  bm_node.u.vlist.glue_order = 0;
  bm_node.u.vlist.first_entry_index = 0;
  bm_node.u.vlist.function_name_start = NULL;
  bm_node.u.vlist.function_name_length = 0;
  return copy_box_model_node(bm_node);
}

static void append_to_vlist(size_t vlist_index, size_t bm_node_index)
{
  if (bm_node_index == 0)
    {
      return;
    }
  BoxModelNode bm_entry_node;
  bm_entry_node.index = 0;
  bm_entry_node.type = BT_VLIST_ENTRY;
  bm_entry_node.u.vlist_entry.next_entry_index = 0;
  bm_entry_node.u.vlist_entry.node_index = bm_node_index;
  BoxModelNode *bm_vlist_node = get_box_model_node(vlist_index);
  size_t last_entry_index = bm_vlist_node->u.vlist.first_entry_index;
  BoxModelNode *bm_existing_entry_node = NULL;
  while (last_entry_index)
    {
      bm_existing_entry_node = get_box_model_node(last_entry_index);
      last_entry_index = bm_existing_entry_node->u.vlist_entry.next_entry_index;
    }
  if (bm_existing_entry_node)
    {
      bm_existing_entry_node->u.vlist_entry.next_entry_index = copy_box_model_node(bm_entry_node);
    }
  else
    {
      bm_vlist_node->u.vlist.first_entry_index = copy_box_model_node(bm_entry_node);
    }
}

static void vlist_set_glue_(BoxModelNode *vlist_node, double x, int sign, const double *totals, const char *error_type)
{

  int o;
  for (o = 0; o < 4 && totals[o] == 0.0; o++)
    ;
  o = o % 4;
  vlist_node->u.vlist.glue_order = o;
  vlist_node->u.vlist.glue_sign = sign;
  if (totals[o] != 0.0)
    {
      vlist_node->u.vlist.glue_set = x / totals[o];
    }
  else
    {
      vlist_node->u.vlist.glue_sign = 0;
    }
  if (o == 0 && vlist_node->u.vlist.first_entry_index)
    {
      fprintf(stderr, "%s\n", error_type);
    }
}

static void pack_vlist(size_t vlist_index, double h, int m, double l)
{
  double w = 0;
  double d = 0;
  double x = 0;

  double total_stretch[4] = {0.0, 0.0, 0.0, 0.0};
  double total_shrink[4] = {0.0, 0.0, 0.0, 0.0};

  BoxModelNode *vlist_node = get_box_model_node(vlist_index);
  BoxModelNode *entry_node;
  for (entry_node = get_box_model_node(vlist_node->u.vlist.first_entry_index); entry_node;
       entry_node = get_box_model_node(entry_node->u.vlist_entry.next_entry_index))
    {
      size_t node_index = entry_node->u.vlist_entry.node_index;
      BoxModelNode *node = get_box_model_node(node_index);
      if (!node)
        {
          fprintf(stderr, "empty vlist entry!\n");
          continue;
        }
      switch (node->type)
        {
        case BT_HBOX:
          x += d;
          d = 0;
          if (!isinf(node->u.hbox.width))
            {
              w = max(w, node->u.hbox.width);
            }
          break;
        case BT_VBOX:
          x += d + node->u.vbox.height;
          d = node->u.vbox.depth;
          w = max(w, 0.0);
          break;
        case BT_HLIST:
          x += d + node->u.hlist.height;
          d = node->u.hlist.depth;
          if (!isinf(node->u.hlist.width))
            {
              w = max(w, node->u.hlist.width + node->u.hlist.shift_amount);
            }
          break;
        case BT_VLIST:
          x += d + node->u.vlist.height;
          d = node->u.vlist.depth;
          if (!isinf(node->u.vlist.width))
            {
              w = max(w, node->u.vlist.width + node->u.vlist.shift_amount);
            }
          break;
        case BT_HRULE:
          x += d + node->u.hrule.height;
          d = node->u.hrule.depth;
          if (!isinf(node->u.hrule.width))
            {
              w = max(w, node->u.hrule.width);
            }
          break;
        case BT_KERN:
          x += d + node->u.kern.width;
          d = 0.0;
          break;
        case BT_CHAR:
          fprintf(stderr, "error: char in vlist\n");
          break;
        case BT_GLUE:
          x += d;
          d = 0.0;
          x += node->u.glue.spec->width * node->u.glue.factor;
          total_stretch[node->u.glue.spec->stretch_order] += node->u.glue.spec->stretch;
          total_shrink[node->u.glue.spec->shrink_order] += node->u.glue.spec->shrink;
          break;
        default:
          fprintf(stderr, "error: unhandled type in vlist: %d\n", node->type);
          break;
        }
    }

  vlist_node->u.vlist.width = w;
  if (d > l)
    {
      x += d - l;
      vlist_node->u.vlist.depth = l;
    }
  else
    {
      vlist_node->u.vlist.depth = d;
    }

  if (m)
    {
      h += x;
    }
  vlist_node->u.vlist.height = h;
  x = h - x;

  if (x == 0.0)
    {
      vlist_node->u.hlist.glue_sign = 0;
      vlist_node->u.hlist.glue_order = 0;
    }
  else if (x > 0.0)
    {
      vlist_set_glue_(vlist_node, x, 1, total_stretch, "Overfull vbox");
    }
  else
    {
      vlist_set_glue_(vlist_node, x, -1, total_shrink, "Underfull vbox");
    }
}

static size_t make_hlist()
{
  BoxModelNode bm_node;
  bm_node.index = 0;
  bm_node.type = BT_HLIST;
  bm_node.size = 0;
  bm_node.u.hlist.width = 0.0;
  bm_node.u.hlist.height = 0.0;
  bm_node.u.hlist.depth = 0.0;
  bm_node.u.hlist.shift_amount = 0.0;
  bm_node.u.hlist.glue_set = 0.0;
  bm_node.u.hlist.glue_sign = 0;
  bm_node.u.hlist.glue_order = 0;
  bm_node.u.hlist.first_entry_index = 0;
  bm_node.u.hlist.subsuper_width = 0.0;
  bm_node.u.hlist.subsuper_height = NAN;
  bm_node.u.hlist.subsuper_depth = NAN;
  bm_node.u.hlist.subsuper_hlist_index = 0;
  bm_node.u.hlist.subsuper_kern_index = 0;
  bm_node.u.hlist.subsuper_is_overunder = 0;
  bm_node.u.hlist.function_name_start = NULL;
  bm_node.u.hlist.function_name_length = 0;
  return copy_box_model_node(bm_node);
}

static void append_to_hlist(size_t hlist_index, size_t bm_node_index)
{
  if (bm_node_index == 0)
    {
      return;
    }
  BoxModelNode bm_entry_node;
  bm_entry_node.index = 0;
  bm_entry_node.type = BT_HLIST_ENTRY;
  bm_entry_node.u.hlist_entry.next_entry_index = 0;
  bm_entry_node.u.hlist_entry.node_index = bm_node_index;
  BoxModelNode *bm_hlist_node = get_box_model_node(hlist_index);
  size_t last_entry_index = bm_hlist_node->u.hlist.first_entry_index;
  BoxModelNode *bm_existing_entry_node = NULL;
  while (last_entry_index)
    {
      bm_existing_entry_node = get_box_model_node(last_entry_index);
      last_entry_index = bm_existing_entry_node->u.hlist_entry.next_entry_index;
    }
  if (bm_existing_entry_node)
    {
      bm_existing_entry_node->u.hlist_entry.next_entry_index = copy_box_model_node(bm_entry_node);
    }
  else
    {
      bm_hlist_node->u.hlist.first_entry_index = copy_box_model_node(bm_entry_node);
    }
}

static void prepend_to_hlist(size_t hlist_index, size_t bm_node_index)
{
  if (bm_node_index == 0)
    {
      return;
    }
  BoxModelNode *bm_hlist_node = get_box_model_node(hlist_index);
  BoxModelNode bm_entry_node;
  bm_entry_node.index = 0;
  bm_entry_node.type = BT_HLIST_ENTRY;
  bm_entry_node.u.hlist_entry.next_entry_index = bm_hlist_node->u.hlist.first_entry_index;
  bm_entry_node.u.hlist_entry.node_index = bm_node_index;
  bm_hlist_node->u.hlist.first_entry_index = copy_box_model_node(bm_entry_node);
}

static void hlist_set_glue_(BoxModelNode *hlist_node, double x, int sign, const double *totals, const char *error_type)
{

  int o;
  for (o = 0; o < 4 && totals[o] == 0.0; o++)
    ;
  o = o % 4;
  hlist_node->u.hlist.glue_order = o;
  hlist_node->u.hlist.glue_sign = sign;
  if (totals[o] != 0.0)
    {
      hlist_node->u.hlist.glue_set = x / totals[o];
    }
  else
    {
      hlist_node->u.hlist.glue_sign = 0;
    }
  if (o == 0 && hlist_node->u.hlist.first_entry_index)
    {
      fprintf(stderr, "%s\n", error_type);
    }
}


static void pack_hlist(size_t hlist_index, double w, int m)
{
  double h = 0;
  double d = 0;
  double x = 0;

  double total_stretch[4] = {0.0, 0.0, 0.0, 0.0};
  double total_shrink[4] = {0.0, 0.0, 0.0, 0.0};

  BoxModelNode *hlist_node = get_box_model_node(hlist_index);
  BoxModelNode *entry_node;
  for (entry_node = get_box_model_node(hlist_node->u.hlist.first_entry_index); entry_node;
       entry_node = get_box_model_node(entry_node->u.hlist_entry.next_entry_index))
    {
      size_t node_index = entry_node->u.hlist_entry.node_index;
      BoxModelNode *node = get_box_model_node(node_index);
      if (!node)
        {
          fprintf(stderr, "empty hlist entry!\n");
          continue;
        }
      switch (node->type)
        {
        case BT_CHAR:
          x += node->u.character.width;
          h = max(h, node->u.character.height);
          d = max(d, node->u.character.depth);
          break;
        case BT_HBOX:
          x += node->u.hbox.width;
          h = max(h, 0.0);
          d = max(d, 0.0);
          break;
        case BT_VBOX:
          if (!isinf(node->u.vbox.height) && !isinf(node->u.vbox.depth))
            {
              h = max(h, node->u.vbox.height);
              d = max(d, node->u.vbox.depth);
            }
          break;
        case BT_HLIST:
          x += node->u.hlist.width;
          if (!isinf(node->u.hlist.height) && !isinf(node->u.hlist.depth))
            {
              h = max(h, node->u.hlist.height - node->u.hlist.shift_amount);
              d = max(d, node->u.hlist.depth + node->u.hlist.shift_amount);
            }
          break;
        case BT_VLIST:
          x += node->u.vlist.width;
          if (!isinf(node->u.vlist.height) && !isinf(node->u.vlist.depth))
            {
              h = max(h, node->u.vlist.height - node->u.vlist.shift_amount);
              d = max(d, node->u.vlist.depth + node->u.vlist.shift_amount);
            }
          break;
        case BT_HRULE:
          x += node->u.hrule.width;
          if (!isinf(node->u.hrule.height) && !isinf(node->u.hrule.depth))
            {
              h = max(h, node->u.hrule.height);
              d = max(d, node->u.hrule.depth);
            }
          break;
        case BT_GLUE:
          x += node->u.glue.spec->width * node->u.glue.factor;
          total_stretch[node->u.glue.spec->stretch_order] += node->u.glue.spec->stretch;
          total_shrink[node->u.glue.spec->shrink_order] += node->u.glue.spec->shrink;
        case BT_KERN:
          x += node->u.kern.width;
          break;
        default:
          fprintf(stderr, "error: unhandled type in hlist: %d\n", node->type);
          break;
        }
    }
  hlist_node->u.hlist.height = h;
  hlist_node->u.hlist.depth = d;
  if (m)
    {
      w += x;
    }
  hlist_node->u.hlist.width = w;
  x = w - x;

  if (x == 0.0)
    {
      hlist_node->u.hlist.glue_sign = 0;
      hlist_node->u.hlist.glue_order = 0;
    }
  else if (x > 0.0)
    {
      hlist_set_glue_(hlist_node, x, 1, total_stretch, "Overfull hbox");
    }
  else
    {
      hlist_set_glue_(hlist_node, x, -1, total_shrink, "Underfull hbox");
    }
}


static void kern_hlist(size_t hlist_index)
{

  BoxModelNode *hlist_node = get_box_model_node(hlist_index);
  BoxModelNode *entry_node;

  for (entry_node = get_box_model_node(hlist_node->u.hlist.first_entry_index); entry_node;
       entry_node = get_box_model_node(entry_node->u.hlist_entry.next_entry_index))
    {
      if (entry_node->u.hlist_entry.node_index)
        {
          BoxModelNode *child = get_box_model_node(entry_node->u.hlist_entry.node_index);
          if (child && child->type == BT_CHAR)
            {
              double kerning = 0.0;
              BoxModelNode *next = NULL;
              if (entry_node->u.hlist_entry.next_entry_index)
                {
                  next = get_box_model_node(
                      get_box_model_node(entry_node->u.hlist_entry.next_entry_index)->u.hlist_entry.node_index);
                }
              if (next && next->type == BT_CHAR && child->u.character.state.font == next->u.character.state.font &&
                  child->u.character.state.fontsize == next->u.character.state.fontsize)
                {
                  kerning = gks_ft_get_kerning(child->u.character.state.font, child->u.character.state.fontsize,
                                               child->u.character.state.dpi, child->u.character.codepoint,
                                               next->u.character.codepoint);
                }
              kerning += child->u.character.advance - child->u.character.width;
              if (kerning != 0)
                {
                  BoxModelNode kerning_node;
                  kerning_node.type = BT_KERN;
                  kerning_node.index = 0;
                  kerning_node.size = 0;
                  kerning_node.u.kern.is_auto = 1;
                  kerning_node.u.kern.width = kerning;
                  BoxModelNode kerning_hlist_entry;
                  kerning_hlist_entry.type = BT_HLIST_ENTRY;
                  kerning_hlist_entry.index = 0;
                  kerning_hlist_entry.u.hlist_entry.node_index = copy_box_model_node(kerning_node);
                  kerning_hlist_entry.u.hlist_entry.next_entry_index = entry_node->u.hlist_entry.next_entry_index;
                  entry_node->u.hlist_entry.next_entry_index = copy_box_model_node(kerning_hlist_entry);
                }
            }
        }
    }
}


static size_t convert_to_box_model(size_t parser_node_index, size_t previous_box_model_node_index);

static size_t convert_space_to_box_model(ParserNode *node)
{
  const struct
  {
    const char *symbol;
    double width;
  } space_widths[] = {{"\\,", 0.16667}, {"\\thinspace", 0.16667}, {"\\/", 0.16667},
                      {"\\>", 0.22222}, {"\\:", 0.22222},         {"\\;", 0.27778},
                      {"\\ ", 0.33333}, {"~", 0.33333},           {"\\enspace", 0.5},
                      {"\\quad", 1},    {"\\qquad", 2},           {"\\!", -0.16667}};
  int i;
  for (i = 0; i < sizeof(space_widths) / sizeof(space_widths[0]); i++)
    {
      if (strncmp(space_widths[i].symbol, node->source, node->length) == 0)
        {
          size_t bm_node_index = make_space(space_widths[i].width);
          return bm_node_index;
        }
    }
  return 0;
}

static size_t convert_custom_space_to_box_model(ParserNode *node)
{
  double width = 0;
  int n = sscanf(node->source, "\\hspace{%lf}", &width);
  if (n != 1)
    {
      fprintf(stderr, "unable to parse custom space\n");
      width = 0;
    }
  size_t bm_node_index = make_space(width);
  return bm_node_index;
}


static size_t convert_math_to_box_model_helper(ParserNode *node, size_t *inner_node_index_ptr)
{
  size_t hlist_index = 0;
  size_t previous_inner_node_index = 0;
  if (node->u.math.previous != 0)
    {
      hlist_index =
          convert_math_to_box_model_helper(get_parser_node(node->u.math.previous), &previous_inner_node_index);
    }
  else
    {
      hlist_index = make_hlist();
    }
  *inner_node_index_ptr = convert_to_box_model(node->u.math.token, previous_inner_node_index);
  append_to_hlist(hlist_index, *inner_node_index_ptr);
  return hlist_index;
}

static size_t convert_math_to_box_model(ParserNode *node)
{
  size_t previous_inner_bm_index = 0;
  return convert_math_to_box_model_helper(node, &previous_inner_bm_index);
}

static size_t convert_function_to_box_model(ParserNode *node)
{
  push_state();
  get_current_state()->font = FV_RM;
  size_t bm_node_index = make_hlist();
  size_t i;
  for (i = 1; i < node->length; i++)
    {
      append_to_hlist(bm_node_index, make_char((unsigned int)(node->source[i])));
    }
  kern_hlist(bm_node_index);
  pack_hlist(bm_node_index, 0, 1);
  pop_state();
  get_box_model_node(bm_node_index)->u.hlist.function_name_start = node->source;
  get_box_model_node(bm_node_index)->u.hlist.function_name_length = node->length;
  return bm_node_index;
}

static size_t convert_operatorname_to_box_model(ParserNode *node)
{
  push_state();
  get_current_state()->font = FV_RM;
  size_t bm_node_index = make_hlist();
  ParserNode *inner_node;
  for (inner_node = node; inner_node; inner_node = get_parser_node(inner_node->u.operatorname.previous))
    {
      append_to_hlist(bm_node_index, convert_to_box_model(inner_node->u.operatorname.token, 0));
    }
  pop_state();
  kern_hlist(bm_node_index);
  pack_hlist(bm_node_index, 0, 1);
  return bm_node_index;
}


static size_t find_in_sorted_string_list(const char *string, size_t string_length, const char **sorted_list,
                                         size_t list_length)
{
  size_t lower = 0;
  size_t upper = list_length - 1;
  while (lower <= upper)
    {
      size_t middle = (lower + upper) / 2;
      int cmp = strncmp(sorted_list[middle], string, string_length);
      if (cmp == 0 && sorted_list[middle][string_length] == 0)
        {
          return middle;
        }
      if (lower == upper)
        {
          break;
        }
      if (cmp >= 0)
        {
          upper = middle - 1;
        }
      else
        {
          lower = middle + 1;
        }
    }
  return list_length;
}


static unsigned int symbol_to_codepoint(const unsigned char *utf8_str, size_t length)
{
  if (utf8_str[0] == '\\' && length != 1)
    {
      {
        /* tex symbols */
        size_t num_symbols = sizeof(symbol_names) / sizeof(symbol_names[0]);
        size_t pos = find_in_sorted_string_list((const char *)utf8_str, length, symbol_names, num_symbols);

        if (pos < num_symbols)
          {
            return symbol_codepoints[pos];
          }
      }
      {
        /* binary operators */
        size_t num_symbols = sizeof(binary_operators) / sizeof(binary_operators[0]);
        size_t pos = find_in_sorted_string_list((const char *)utf8_str, length, binary_operators, num_symbols);

        if (pos < num_symbols)
          {
            return binary_operator_codepoints[pos];
          }
      }
      return (unsigned int)'?';
    }

  unsigned int codepoint = 0;
  if ((utf8_str[0] & 0x80U) == 0x00U)
    {
      if (length != 1)
        {
          return (unsigned int)'?';
        }
      codepoint = utf8_str[0U];
    }
  else if ((utf8_str[0] & 0xe0U) == 0xc0U && (utf8_str[1] & 0xc0U) == 0x80U)
    {
      if (length != 2)
        {
          return (unsigned int)'?';
        }
      codepoint = ((utf8_str[0] & 0x1fU) << 6U) + (utf8_str[1] & 0x3fU);
    }
  else if ((utf8_str[0] & 0xf0U) == 0xe0U && (utf8_str[1] & 0xc0U) == 0x80U && (utf8_str[2] & 0xc0U) == 0x80U)
    {
      if (length != 3)
        {
          return (unsigned int)'?';
        }
      codepoint = ((utf8_str[0] & 0xfU) << 12U) + ((utf8_str[1] & 0x3fU) << 6U) + (utf8_str[1] & 0x3fU);
    }
  else if ((utf8_str[0] & 0xf8U) == 0xf0U && (utf8_str[1] & 0xc0U) == 0x80U && (utf8_str[2] & 0xc0U) == 0x80U &&
           (utf8_str[3] & 0xc0U) == 0x80U)
    {
      if (length != 4)
        {
          return (unsigned int)'?';
        }
      codepoint = ((utf8_str[0] & 0x7U) << 18U) + ((utf8_str[1] & 0x3fU) << 12U) + ((utf8_str[2] & 0x3fU) << 6U) +
                  (utf8_str[3] & 0x3fU);
    }
  else
    {
      /* invalid byte combination */
      return (unsigned int)'?';
    }

  if (codepoint == '-')
    {
      return 0x2212;
    }
  if (codepoint < 128)
    {
      return codepoint;
    }
  return (unsigned int)'?';
}

static size_t convert_symbol_to_box_model(ParserNode *node)
{
  size_t num_binary_operators = sizeof(binary_operators) / sizeof(binary_operators[0]);
  size_t pos = find_in_sorted_string_list(node->source, node->length, binary_operators, num_binary_operators);
  int is_binary_operator = pos < num_binary_operators;
  int is_spaced_symbol = is_binary_operator;
  if (!is_spaced_symbol)
    {
      size_t num_spaced_symbols = sizeof(spaced_symbols) / sizeof(spaced_symbols[0]);
      pos = find_in_sorted_string_list(node->source, node->length, spaced_symbols, num_spaced_symbols);
      is_spaced_symbol = pos < num_spaced_symbols;
    }

  if (is_binary_operator)
    {
      char previous_char;
      const char *cursor;
      for (cursor = node->source - 1; cursor >= input && isspace(*cursor); cursor--)
        ;
      if (cursor >= input)
        {
          previous_char = *cursor;
        }
      else
        {
          previous_char = 0;
        }
      if (previous_char == 0 || strchr("{[<(", previous_char) != NULL)
        {
          is_spaced_symbol = 0;
        }
    }

  if (is_spaced_symbol)
    {
      size_t char_node_index = make_char(symbol_to_codepoint((const unsigned char *)node->source, node->length));
      BoxModelCharNode *char_node = &get_box_model_node(char_node_index)->u.character;
      size_t hlist_index = make_hlist();
      double space;
      switch (char_node->codepoint)
        {
        case 215:
          space = 0.45;
          break;
        case 8712:
          space = 0.4;
          break;
        default:
          space = 0.35;
          break;
        }
      append_to_hlist(hlist_index, make_space(space));
      append_to_hlist(hlist_index, char_node_index);
      append_to_hlist(hlist_index, make_space(space));
      kern_hlist(hlist_index);
      pack_hlist(hlist_index, 0, 1);
      if (node->source[0] == '*' && node->length == 1)
        {
          get_box_model_node(hlist_index)->u.hlist.shift_amount = get_current_state()->fontsize * 0.3;
        }
      return hlist_index;
    }
  int is_punctuation_symbol =
      (node->length == 1 && strchr(",;.!", node->source[0]) != NULL) ||
      (node->length == 6 && (strncmp(node->source, "\\ldotp", 6) == 0 || strncmp(node->source, "\\cdotp", 6) == 0));
  if (is_punctuation_symbol)
    {
      if (node->length == 1 && node->source[0] == ',')
        {
          const char *cursor;
          char previous_char;
          for (cursor = node->source - 1; cursor >= input && isspace(*cursor); cursor--)
            ;
          if (cursor >= input)
            {
              previous_char = *cursor;
            }
          else
            {
              previous_char = 0;
            }
          if (previous_char == '{')
            {
              char next_char;
              for (cursor = node->source + 1; isspace(*cursor); cursor++)
                ;
              next_char = *cursor;
              if (next_char == '}')
                {
                  is_punctuation_symbol = 0;
                }
            }
        }
    }
  if (is_punctuation_symbol)
    {
      if (node->length == 1 && node->source[0] == '.')
        {
          if (isdigit(node->source[1]) && node->source > input && isdigit(node->source[-1]))
            {
              is_punctuation_symbol = 0;
            }
        }
    }
  if (is_punctuation_symbol)
    {
      size_t hlist_index = make_hlist();
      append_to_hlist(hlist_index, make_char(symbol_to_codepoint((const unsigned char *)node->source, node->length)));
      append_to_hlist(hlist_index, make_space(0.2));
      kern_hlist(hlist_index);
      pack_hlist(hlist_index, 0, 1);
      return hlist_index;
    }
  if (node->source[0] == '*' && node->length == 1)
    {
      size_t hlist_index = make_hlist();
      append_to_hlist(hlist_index, make_char(symbol_to_codepoint((const unsigned char *)node->source, node->length)));
      kern_hlist(hlist_index);
      pack_hlist(hlist_index, 0, 1);
      get_box_model_node(hlist_index)->u.hlist.shift_amount = get_current_state()->fontsize * 0.3;
      return hlist_index;
    }
  if (node->source[0] == '\'' && node->length == 1)
    {
      const char *cursor;
      for (cursor = node->source - 1; cursor >= input && isspace(*cursor); cursor--)
        ;
      if (cursor >= input && cursor[0] != '\'')
        {
          size_t hlist_index = make_hlist();
          append_to_hlist(hlist_index, make_space(0.1));
          append_to_hlist(hlist_index,
                          make_char(symbol_to_codepoint((const unsigned char *)node->source, node->length)));
          kern_hlist(hlist_index);
          pack_hlist(hlist_index, 0, 1);
          return hlist_index;
        }
    }
  return make_char(symbol_to_codepoint((const unsigned char *)node->source, node->length));
}

static size_t convert_overline_to_box_model(ParserNode *node)
{

  size_t body_node_index = convert_to_box_model(node->u.overline.body, 0);
  if (!body_node_index)
    {
      return 0;
    }
  BoxModelNode *body_node = get_box_model_node(body_node_index);

  BoxModelState *state = get_current_state();
  double thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);

  double height = body_node->u.hlist.height - body_node->u.hlist.shift_amount + thickness * 3.0;
  double depth = body_node->u.hlist.depth + body_node->u.hlist.shift_amount;

  size_t stack_index = make_vlist();
  append_to_vlist(stack_index, make_hrule(thickness));
  append_to_vlist(stack_index, make_glue(GT_FILL));
  append_to_vlist(stack_index, body_node_index);
  pack_vlist(stack_index, height + (state->fontsize * state->dpi) / (100.0 * 12.0), 0, depth);

  size_t bm_node_index = make_hlist();
  append_to_hlist(bm_node_index, stack_index);
  kern_hlist(bm_node_index);
  pack_hlist(bm_node_index, 0, 1);
  return bm_node_index;
}


static size_t convert_latextext_to_box_model(ParserNode *node)
{
  const char *font_str = node->source + 5;
  size_t font_str_length = strchr(font_str, '{') - font_str;
  const unsigned char *text_str = (const unsigned char *)font_str + font_str_length + 1;
  size_t text_str_length = strchr((const char *)text_str, '}') - (const char *)text_str;
  size_t i;
  size_t bm_node_index;
  push_state();
  if (strncmp("it", font_str, font_str_length) == 0 && strlen("it") == font_str_length)
    {
      get_current_state()->font = FV_IT;
    }
  else if (strncmp("rm", font_str, font_str_length) == 0 && strlen("rm") == font_str_length)
    {
      get_current_state()->font = FV_RM;
    }
  else if (strncmp("tt", font_str, font_str_length) == 0 && strlen("tt") == font_str_length)
    {
      get_current_state()->font = FV_TT;
    }
  else if (strncmp("bf", font_str, font_str_length) == 0 && strlen("bf") == font_str_length)
    {
      get_current_state()->font = FV_BF;
    }
  else
    {
      fprintf(stderr, "Error: unknown font variant %.*s\n", (int)font_str_length, font_str);
    }
  bm_node_index = make_hlist();

  for (i = 0; i < text_str_length; i++)
    {
      int j;
      int offset;
      int following_bytes;
      unsigned int codepoint = 0;

      if (text_str[i] < 128)
        {
          offset = 0;
          following_bytes = 0;
        }
      else if (text_str[i] < 128 + 64 + 32)
        {
          offset = 128 + 64;
          following_bytes = 1;
        }
      else if (text_str[i] < 128 + 64 + 32 + 16)
        {
          offset = 128 + 64 + 32;
          following_bytes = 2;
        }
      else if (text_str[i] < 128 + 64 + 32 + 16 + 8)
        {
          offset = 128 + 64 + 32 + 16;
          following_bytes = 3;
        }
      else
        {
          continue;
        }
      codepoint = text_str[i] - offset;
      for (j = 0; j < following_bytes; j++)
        {
          codepoint = codepoint * 64;
          i++;
          if (text_str[i] < 128 || text_str[i] >= 128 + 64)
            {
              continue;
            }
          codepoint += text_str[i] - 128;
        }
      append_to_hlist(bm_node_index, make_char(codepoint));
    }
  pop_state();
  kern_hlist(bm_node_index);
  pack_hlist(bm_node_index, 0, 1);
  return bm_node_index;
}

static size_t convert_group_to_box_model(ParserNode *node)
{
  const char *font_str = NULL;
  if (strncmp(node->source, "\\math", 5) == 0)
    {
      font_str = node->source + 5;
    }
  if (font_str)
    {
      push_state();
      size_t font_str_length = strchr(font_str, '{') - font_str;
      if (font_str_length != (size_t)font_str)
        {
          if (strncmp("cal", font_str, font_str_length) == 0 && strlen("cal") == font_str_length)
            {
              get_current_state()->font = FV_CAL;
            }
          else if (strncmp("frak", font_str, font_str_length) == 0 && strlen("frak") == font_str_length)
            {
              get_current_state()->font = FV_FRAK;
            }
          else if (strncmp("it", font_str, font_str_length) == 0 && strlen("it") == font_str_length)
            {
              get_current_state()->font = FV_IT;
            }
          else if (strncmp("rm", font_str, font_str_length) == 0 && strlen("rm") == font_str_length)
            {
              get_current_state()->font = FV_RM;
            }
          else if (strncmp("tt", font_str, font_str_length) == 0 && strlen("tt") == font_str_length)
            {
              get_current_state()->font = FV_TT;
            }
          else if (strncmp("bf", font_str, font_str_length) == 0 && strlen("bf") == font_str_length)
            {
              get_current_state()->font = FV_BF;
            }
          else if (strncmp("bb", font_str, font_str_length) == 0 && strlen("bb") == font_str_length)
            {
              get_current_state()->font = FV_BB;
            }
          else
            {
              fprintf(stderr, "Error: unknown font variant %.*s\n", (int)font_str_length, font_str);
            }
        }
    }
  size_t bm_node_index = make_hlist();
  ParserNode *inner_node;
  size_t previous_inner_bm_index = 0;
  for (inner_node = node; inner_node; inner_node = get_parser_node(inner_node->u.operatorname.previous))
    {
      previous_inner_bm_index = convert_to_box_model(inner_node->u.operatorname.token, previous_inner_bm_index);
      append_to_hlist(bm_node_index, previous_inner_bm_index);
    }
  if (font_str)
    {
      pop_state();
    }
  kern_hlist(bm_node_index);
  pack_hlist(bm_node_index, 0, 1);
  return bm_node_index;
}

static void shrink(size_t node_index)
{
  if (!node_index)
    {
      return;
    }
  BoxModelNode *node = get_box_model_node(node_index);
  if (!node)
    {
      return;
    }
  switch (node->type)
    {
    case BT_CHAR:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.character.state.fontsize *= SHRINK_FACTOR;
            node->u.character.width *= SHRINK_FACTOR;
            node->u.character.height *= SHRINK_FACTOR;
            node->u.character.depth *= SHRINK_FACTOR;
            node->u.character.advance *= SHRINK_FACTOR;
            node->u.character.bearing *= SHRINK_FACTOR;
            node->u.character.shift_amount *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_GLUE:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
          }
        break;
      }
    case BT_KERN:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.kern.width *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_HBOX:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.hbox.width *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_HRULE:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.hrule.width *= SHRINK_FACTOR;
            node->u.hrule.height *= SHRINK_FACTOR;
            node->u.hrule.depth *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_VBOX:
      {
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.vbox.height *= SHRINK_FACTOR;
            node->u.vbox.depth *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_HLIST:
      {
        shrink(node->u.hlist.first_entry_index);
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.hlist.shift_amount *= SHRINK_FACTOR;
            node->u.hlist.glue_set *= SHRINK_FACTOR;
            node->u.hlist.width *= SHRINK_FACTOR;
            node->u.hlist.height *= SHRINK_FACTOR;
            node->u.hlist.depth *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_VLIST:
      {
        shrink(node->u.vlist.first_entry_index);
        if (node->size < NUM_SIZE_LEVELS)
          {
            node->size += 1;
            node->u.vlist.shift_amount *= SHRINK_FACTOR;
            node->u.vlist.glue_set *= SHRINK_FACTOR;
            node->u.vlist.width *= SHRINK_FACTOR;
            node->u.vlist.height *= SHRINK_FACTOR;
            node->u.vlist.depth *= SHRINK_FACTOR;
          }
        break;
      }
    case BT_HLIST_ENTRY:
      shrink(node->u.hlist_entry.node_index);
      shrink(node->u.hlist_entry.next_entry_index);
      break;
    case BT_VLIST_ENTRY:
      shrink(node->u.vlist_entry.node_index);
      shrink(node->u.vlist_entry.next_entry_index);
      break;
    }
}

static size_t make_auto_height_char(unsigned int codepoint, double height, double depth, double factor)
{
  if (codepoint == 0)
    {
      return 0;
    }
  double target_total = height + depth;
  size_t simple_character_index = make_char(codepoint);
  BoxModelNode *simple_character = get_box_model_node(simple_character_index);
  if (isnan(factor))
    {
      factor = target_total / (simple_character->u.character.height + simple_character->u.character.depth);
    }
  push_state();
  get_current_state()->fontsize *= factor;
  size_t scaled_character_index = make_char(codepoint);
  BoxModelNode *scaled_character = get_box_model_node(scaled_character_index);
  pop_state();
  size_t hlist_index = make_hlist();
  append_to_hlist(hlist_index, scaled_character_index);
  kern_hlist(hlist_index);
  pack_hlist(hlist_index, 0, 1);
  get_box_model_node(hlist_index)->u.hlist.shift_amount = (depth - scaled_character->u.character.depth);
  return hlist_index;
}

static size_t make_auto_width_accent(unsigned int codepoint, double width, double factor, double bearing_factor)
{
  if (codepoint == 0)
    {
      return 0;
    }
  double target_total = width;
  size_t simple_character_index = make_accent(codepoint, bearing_factor);
  BoxModelNode *simple_character = get_box_model_node(simple_character_index);
  if (isnan(factor))
    {
      factor = target_total / simple_character->u.character.width;
    }
  push_state();
  get_current_state()->fontsize *= factor;
  size_t scaled_character_index = make_accent(codepoint, bearing_factor);
  pop_state();
  size_t hlist_index = make_hlist();
  append_to_hlist(hlist_index, scaled_character_index);
  kern_hlist(hlist_index);
  pack_hlist(hlist_index, 0, 1);
  return hlist_index;
}

static void remove_auto_space(size_t hlist_index)
{
  BoxModelNode *hlist_node = get_box_model_node(hlist_index);
  if (!hlist_node)
    {
      return;
    }
  if (!hlist_node->u.hlist.first_entry_index)
    {
      return;
    }
  size_t entry_index;
  size_t last_non_autokern_index = 0;
  BoxModelNode *node;
  for (entry_index = hlist_node->u.hlist.first_entry_index; entry_index;)
    {
      BoxModelNode *entry = get_box_model_node(entry_index);
      if (!entry)
        {
          break;
        }
      if (entry->u.hlist_entry.node_index)
        {
          node = get_box_model_node(entry->u.hlist_entry.node_index);
          if (node && (node->type != BT_KERN || !node->u.kern.is_auto))
            {
              last_non_autokern_index = entry_index;
            }
        }
      entry_index = entry->u.hlist_entry.next_entry_index;
    }
  if (last_non_autokern_index)
    {
      get_box_model_node(last_non_autokern_index)->u.hlist_entry.next_entry_index = 0;
    }
  pack_hlist(hlist_index, 0, 1);
}

static size_t make_auto_sized_delim(const char *left_delim_start, size_t left_delim_length, size_t middle_node_index,
                                    const char *right_delim_start, size_t right_delim_length)
{
  double height = NAN;
  double depth = 0.0;
  double factor = 1.0;
  remove_auto_space(middle_node_index);
  if (middle_node_index)
    {
      BoxModelNode *middle_node = get_box_model_node(middle_node_index);
      if (middle_node)
        {
          height = middle_node->u.hlist.height;
          depth = middle_node->u.hlist.depth;
          factor = NAN;
        }
    }
  if (height == 0 && depth == 0)
    {
      height = NAN;
    }

  unsigned int left_delim_codepoint = 0;
  if (left_delim_start != NULL && left_delim_length != 0)
    {
      left_delim_codepoint = ((unsigned char *)left_delim_start)[0];
    }
  if (left_delim_codepoint == '\\' && left_delim_length > 1)
    {
      if (left_delim_length == 2)
        {
          left_delim_codepoint = ((unsigned char *)left_delim_start)[1];
        }
      else
        {
          size_t symbol_index = find_in_sorted_string_list(left_delim_start, left_delim_length, symbol_names,
                                                           sizeof(symbol_names) / sizeof(symbol_names[0]));
          left_delim_codepoint = symbol_codepoints[symbol_index];
        }
    }
  unsigned int right_delim_codepoint = 0;
  if (right_delim_start != NULL && right_delim_length != 0)
    {
      right_delim_codepoint = ((unsigned char *)right_delim_start)[0];
    }
  if (right_delim_codepoint == '\\')
    {
      if (right_delim_length == 2)
        {
          right_delim_codepoint = ((unsigned char *)right_delim_start)[1];
        }
      else
        {

          size_t symbol_index = find_in_sorted_string_list(right_delim_start, right_delim_length, symbol_names,
                                                           sizeof(symbol_names) / sizeof(symbol_names[0]));
          right_delim_codepoint = symbol_codepoints[symbol_index];
        }
    }

  size_t hlist_index = make_hlist();
  if (left_delim_codepoint && left_delim_codepoint != '.')
    {
      if (isnan(height))
        {
          append_to_hlist(hlist_index, make_char(left_delim_codepoint));
        }
      else
        {
          append_to_hlist(hlist_index, make_auto_height_char(left_delim_codepoint, height, depth, factor));
        }
    }
  BoxModelState *state = get_current_state();
  double default_thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);
  append_to_hlist(hlist_index, make_kern(2 * default_thickness));
  append_to_hlist(hlist_index, middle_node_index);
  append_to_hlist(hlist_index, make_kern(2 * default_thickness));
  if (right_delim_codepoint && right_delim_codepoint != '.')
    {
      if (isnan(height))
        {
          append_to_hlist(hlist_index, make_char(right_delim_codepoint));
        }
      else
        {
          append_to_hlist(hlist_index, make_auto_height_char(right_delim_codepoint, height, depth, factor));
        }
    }
  kern_hlist(hlist_index);
  pack_hlist(hlist_index, 0, 1);
  return hlist_index;
}

static size_t convert_genfrac_to_box_model(ParserNode *node)
{
  BoxModelState *state = get_current_state();
  double default_thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);
  double rule_thickness = node->u.genfrac.thickness;
  if (isnan(rule_thickness))
    {
      rule_thickness = default_thickness;
    }

  size_t numerator_node_index = convert_to_box_model(node->u.genfrac.numerator_group, 0);
  size_t denominator_node_index = convert_to_box_model(node->u.genfrac.denominator_group, 0);


  int style = 0;
  if (node->u.genfrac.style_text_length == 3)
    {
      char style_char = node->u.genfrac.style_text_start[1];
      if ('0' <= style_char && style_char <= '3')
        {
          style = style_char - '0';
        }
    }
  if (style != 0)
    {
      shrink(numerator_node_index);
      shrink(denominator_node_index);
    }
  remove_auto_space(numerator_node_index);
  remove_auto_space(denominator_node_index);

  size_t centered_numerator_node_index = make_hlist();
  append_to_hlist(centered_numerator_node_index, make_glue(GT_SS));
  append_to_hlist(centered_numerator_node_index, numerator_node_index);
  append_to_hlist(centered_numerator_node_index, make_glue(GT_SS));
  pack_hlist(centered_numerator_node_index, 0, 1);

  size_t centered_denominator_node_index = make_hlist();
  append_to_hlist(centered_denominator_node_index, make_glue(GT_SS));
  append_to_hlist(centered_denominator_node_index, denominator_node_index);
  append_to_hlist(centered_denominator_node_index, make_glue(GT_SS));
  pack_hlist(centered_denominator_node_index, 0, 1);

  BoxModelNode *centered_numerator_node = get_box_model_node(centered_numerator_node_index);
  BoxModelNode *centered_denominator_node = get_box_model_node(centered_denominator_node_index);

  double max_width = max(centered_numerator_node->u.hlist.width, centered_denominator_node->u.hlist.width);
  pack_hlist(centered_numerator_node_index, max_width, 0);
  pack_hlist(centered_denominator_node_index, max_width, 0);

  size_t vlist_index = make_vlist();
  append_to_vlist(vlist_index, centered_numerator_node_index);
  append_to_vlist(vlist_index, make_vbox(0, default_thickness * 4.0));
  append_to_vlist(vlist_index, make_hrule_with_width(rule_thickness, max_width + default_thickness * 2));
  append_to_vlist(vlist_index, make_vbox(0, default_thickness * 4.0));
  append_to_vlist(vlist_index, centered_denominator_node_index);
  pack_vlist(vlist_index, 0, 1, INFINITY);

  double ymax, ymin;
  gks_ft_get_metrics(MATH_FONT, state->fontsize * 1.16, '=', state->dpi, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                     &ymin, &ymax);

  double shift =
      centered_denominator_node->u.hlist.height + default_thickness * 4 + rule_thickness / 2 - (ymax + ymin) / 2;

  get_box_model_node(vlist_index)->u.vlist.shift_amount = shift;

  size_t hlist_index = make_hlist();
  append_to_hlist(hlist_index, make_hbox(default_thickness * 2.0));
  append_to_hlist(hlist_index, vlist_index);
  pack_hlist(hlist_index, 0, 1);

  return make_auto_sized_delim(node->u.genfrac.left_delim_start, node->u.genfrac.left_delim_length, hlist_index,
                               node->u.genfrac.right_delim_start, node->u.genfrac.right_delim_length);
}

static size_t convert_subsuper_to_box_model(ParserNode *node, size_t previous_box_model_node_index)
{
  int is_overunder = 0;
  BoxModelNode *previous_node = get_box_model_node(previous_box_model_node_index);
  size_t previous_node_kern_index = 0;
  size_t previous_node_hlist_index = 0;
  if (previous_node)
    {
      if (previous_node->type == BT_HLIST)
        {
          remove_auto_space(previous_box_model_node_index);
          previous_node = get_box_model_node(previous_box_model_node_index);
          previous_node_hlist_index = previous_node->u.hlist.subsuper_hlist_index;
          previous_node_kern_index = previous_node->u.hlist.subsuper_kern_index;
        }
      else if (previous_node->type == BT_CHAR)
        {
          switch (previous_node->u.character.codepoint)
            {
            case 8719:
            case 8720:
            case 8721:
            case 8747:
            case 8750:
            case 8896:
            case 8897:
            case 8898:
            case 8899:
            case 10752:
            case 10753:
            case 10754:
            case 10756:
              is_overunder = 1;
              break;
            default:
              break;
            }
          /* Swap nodes */
          size_t hlist_wrapper_index = make_hlist();
          BoxModelNode tmp_node = *get_box_model_node(hlist_wrapper_index);
          previous_node->index = hlist_wrapper_index;
          tmp_node.index = previous_box_model_node_index;
          *get_box_model_node(hlist_wrapper_index) = *previous_node;
          *previous_node = tmp_node;
          previous_node_hlist_index = previous_box_model_node_index;
          previous_node_kern_index = make_kern(0);
          append_to_hlist(previous_box_model_node_index, previous_node_kern_index);
          append_to_hlist(previous_box_model_node_index, hlist_wrapper_index);
          pack_hlist(previous_box_model_node_index, 0, 1);
          previous_box_model_node_index = hlist_wrapper_index;
          previous_node = get_box_model_node(previous_box_model_node_index);
        }
    }

  size_t body_node_index = convert_to_box_model(node->u.subsuper.token, 0);
  shrink(body_node_index);

  size_t body_hlist_index = make_hlist();
  append_to_hlist(body_hlist_index, body_node_index);
  pack_hlist(body_hlist_index, 0, 1);
  body_node_index = body_hlist_index;
  double body_width = get_box_model_node(body_node_index)->u.hlist.width;

  size_t hlist_index = make_hlist();
  int sub_and_super_filled = 0;

  BoxModelState *state = get_current_state();

  double default_thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);
  double padding = 2 * default_thickness;
  if (previous_node && previous_node->type == BT_HLIST)
    {
      if (node->u.subsuper.operator== '_' && previous_node->u.hlist.subsuper_width<0)
        {
          padding += previous_node->u.hlist.subsuper_width;
          is_overunder = previous_node->u.hlist.subsuper_is_overunder;
          sub_and_super_filled = 1;
        }
      else if (node->u.subsuper.operator== '^' && previous_node->u.hlist.subsuper_width> 0)
        {
          padding -= previous_node->u.hlist.subsuper_width;
          is_overunder = previous_node->u.hlist.subsuper_is_overunder;
          sub_and_super_filled = 1;
        }
    }
  /* TODO: better default height? */
  double previous_node_height = state->fontsize;
  double previous_node_depth = 0.0;
  double previous_node_width = 0.0;
  double previous_node_kern_width = 0.0;
  if (previous_node)
    {
      if (previous_node->type != BT_HLIST)
        {
          size_t tmp_hlist_index = make_hlist();
          append_to_hlist(tmp_hlist_index, previous_box_model_node_index);
          pack_hlist(tmp_hlist_index, 0, 1);
          BoxModelNode *tmp_hlist = get_box_model_node(tmp_hlist_index);
          previous_node_height = tmp_hlist->u.hlist.height - tmp_hlist->u.hlist.shift_amount;
          previous_node_depth = tmp_hlist->u.hlist.depth + tmp_hlist->u.hlist.shift_amount;
          if (previous_node->type == BT_CHAR)
            {
              previous_node_width = previous_node->u.character.advance;
            }
          else
            {
              previous_node_width = tmp_hlist->u.hlist.width;
            }
          if (is_overunder)
            {
              padding = -previous_node_width / 2 - body_width / 2;
              previous_node_kern_width = body_width / 2 - default_thickness * 2;
            }
        }
      else if (!isnan(previous_node->u.hlist.subsuper_height))
        {
          previous_node_height = previous_node->u.hlist.subsuper_height;
          previous_node_depth = previous_node->u.hlist.subsuper_depth;
          previous_node_width = previous_node->u.hlist.subsuper_width;
          if (previous_node_width < 0)
            {
              previous_node_width = -previous_node_width;
            }
          if (is_overunder)
            {
              padding = -previous_node_width / 2 - body_width / 2;
              previous_node_kern_width = body_width / 2 - default_thickness * 2;
            }
        }
    }
  if (previous_node_kern_index)
    {
      BoxModelNode *previous_node_kern = get_box_model_node(previous_node_kern_index);
      if (previous_node_kern->u.kern.width < previous_node_kern_width)
        {
          previous_node_kern->u.kern.width = previous_node_kern_width;
          pack_hlist(previous_node_hlist_index, 0, 1);
        }
    }
  append_to_hlist(hlist_index, make_kern(padding));
  append_to_hlist(hlist_index, body_node_index);
  pack_hlist(hlist_index, 0, 1);
  BoxModelNode *hlist_node = get_box_model_node(hlist_index);
  if (hlist_node->u.hlist.width < 0)
    {
      append_to_hlist(hlist_index, make_kern(-hlist_node->u.hlist.width));
    }
  append_to_hlist(hlist_index, make_kern(2 * default_thickness));
  pack_hlist(hlist_index, 0, 1);
  hlist_node = get_box_model_node(hlist_index);
  if (!sub_and_super_filled)
    {
      hlist_node->u.hlist.subsuper_width = hlist_node->u.hlist.width;
      if (is_overunder)
        {
          hlist_node->u.hlist.subsuper_width =
              max(previous_node_width + 4 * default_thickness, body_width + 4 * default_thickness);
        }
      if (node->u.subsuper.operator== '^')
        {
          hlist_node->u.hlist.subsuper_width = -hlist_node->u.hlist.subsuper_width;
        }
    }
  hlist_node = get_box_model_node(hlist_index);
  hlist_node->u.hlist.subsuper_height = previous_node_height;
  hlist_node->u.hlist.subsuper_depth = previous_node_depth;
  hlist_node->u.hlist.subsuper_is_overunder = is_overunder;
  hlist_node->u.hlist.subsuper_kern_index = previous_node_kern_index;
  hlist_node->u.hlist.subsuper_hlist_index = previous_node_hlist_index;
  if (is_overunder)
    {
      if (node->u.subsuper.operator== '_')
        {
          hlist_node->u.hlist.shift_amount = previous_node_depth + 1.5 * hlist_node->u.hlist.height;
        }
      else
        {
          hlist_node->u.hlist.shift_amount = -previous_node_height - 0.75 * hlist_node->u.hlist.height;
        }
    }
  else
    {
      if (node->u.subsuper.operator== '_')
        {
          hlist_node->u.hlist.shift_amount = previous_node_depth + 0.6 * hlist_node->u.hlist.height;
        }
      else
        {
          hlist_node->u.hlist.shift_amount =
              -previous_node_height - hlist_node->u.hlist.depth + 0.3 * hlist_node->u.hlist.height;
        }
    }

  return hlist_index;
}

static size_t convert_auto_delim_to_box_model(ParserNode *node)
{
  size_t hlist_index = make_hlist();
  ParserNode *inner_node;
  size_t previous_bm_node_index = 0;
  for (inner_node = get_parser_node(node->u.autodelim.inner_node_index); inner_node;
       inner_node = get_parser_node(inner_node->u.autodeliminner.previous))
    {
      previous_bm_node_index = convert_to_box_model(inner_node->u.autodeliminner.token, previous_bm_node_index);
      append_to_hlist(hlist_index, previous_bm_node_index);
    }
  kern_hlist(hlist_index);
  pack_hlist(hlist_index, 0, 1);
  return make_auto_sized_delim(node->u.autodelim.left_delim_start, node->u.autodelim.left_delim_length, hlist_index,
                               node->u.autodelim.right_delim_start, node->u.autodelim.right_delim_length);
}

static size_t convert_c_over_c_to_box_model(ParserNode *node)
{
  if (node->length == 3 && strncmp(node->source, "\\AA", 3) == 0)
    {
      /* Use actual unicode angstrom sign instead of faking it */
      return make_char(8491);
    }
  fprintf(stderr, "Unknown character-over-character combination");
  return 0;
}

static size_t convert_accent_to_box_model(ParserNode *node)
{

  const char *accent_symbols[] = {
      "\"",
      "'",
      ".",
      "^",
      "`",
      "acute",
      "bar",
      "breve",
      "ddot",
      "dot",
      "grave",
      "hat",
      "mathring",
      "overleftarrow",
      "overrightarrow",
      "tilde",
      "vec",
      "widebar",
      "widehat",
      "widetilde",
      "~",
  };
  const unsigned int accent_codepoints[] = {
      776,
      769,
      775,
      770,
      768,
      769,
      772,
      774,
      776,
      775,
      768,
      770,
      8728,
      8592,
      8594,
      771,
      8407,
      772, /* 773,*/
      770,
      /* 770, */ /* current implementation of wide symbols looks awful */
      771,       /* 771, */
      771,
  };
  const int accent_is_wide[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 1, */
      0,                                                    /* 1, */
      0,                                                    /* 1, */
      0,
  };
  const double accent_bearing_factor[] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, /* 1, */
      1,                                                    /* 1, */
      1,                                                    /* 1, */
      1,
  };
  size_t inner_node_index = convert_to_box_model(node->u.accent.token, 0);
  if (!inner_node_index)
    {
      fprintf(stderr, "Error: Empty accent");
      return 0;
    }
  if (get_box_model_node(inner_node_index)->type == BT_HLIST)
    {
      remove_auto_space(inner_node_index);
    }
  ParserNode *token_node = get_parser_node(node->u.accent.token);
  size_t accent_length = node->length - token_node->length;
  size_t accent_index = find_in_sorted_string_list(node->source + 1, accent_length - 1, accent_symbols,
                                                   sizeof(accent_symbols) / sizeof(accent_symbols[0]));
  if (accent_index == sizeof(accent_symbols) / sizeof(accent_symbols[0]))
    {
      fprintf(stderr, "Error: Unknown accent");
      return inner_node_index;
    }
  unsigned int accent_codepoint = accent_codepoints[accent_index];
  int is_wide = accent_is_wide[accent_index];
  size_t accent_node_index = 0;

  size_t centered_denominator_node_index = make_hlist();
  append_to_hlist(centered_denominator_node_index, make_glue(GT_SS));
  append_to_hlist(centered_denominator_node_index, inner_node_index);
  append_to_hlist(centered_denominator_node_index, make_glue(GT_SS));
  pack_hlist(centered_denominator_node_index, 0, 1);

  if (is_wide)
    {
      accent_node_index =
          make_auto_width_accent(accent_codepoint, get_box_model_node(centered_denominator_node_index)->u.hlist.width,
                                 NAN, accent_bearing_factor[accent_index]);
    }
  else
    {
      accent_node_index = make_accent(accent_codepoint, accent_bearing_factor[accent_index]);
    }
  if (accent_codepoint == 8728)
    {
      shrink(accent_node_index);
      shrink(accent_node_index);
    }
  size_t centered_numerator_node_index = make_hlist();
  append_to_hlist(centered_numerator_node_index, make_glue(GT_SS));
  append_to_hlist(centered_numerator_node_index, make_kern(get_box_model_node(accent_node_index)->u.character.width));
  append_to_hlist(centered_numerator_node_index, accent_node_index);
  append_to_hlist(centered_numerator_node_index, make_glue(GT_SS));
  pack_hlist(centered_numerator_node_index, 0, 1);

  BoxModelNode *centered_numerator_node = get_box_model_node(centered_numerator_node_index);
  BoxModelNode *centered_denominator_node = get_box_model_node(centered_denominator_node_index);

  double max_width = max(centered_numerator_node->u.hlist.width, centered_denominator_node->u.hlist.width);
  pack_hlist(centered_numerator_node_index, max_width, 0);
  pack_hlist(centered_denominator_node_index, max_width, 0);

  BoxModelState *state = get_current_state();
  double default_thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);
  size_t vlist_index = make_vlist();
  append_to_vlist(vlist_index, centered_numerator_node_index);
  append_to_vlist(vlist_index, make_vbox(0, default_thickness * 2.0));
  append_to_vlist(vlist_index, centered_denominator_node_index);
  pack_vlist(vlist_index, 0, 1, INFINITY);
  return vlist_index;
}

static size_t convert_sqrt_to_box_model(ParserNode *node)
{
  size_t inner_node_index = convert_to_box_model(node->u.sqrt.token, 0);
  if (!inner_node_index)
    {
      fprintf(stderr, "Error: Empty root");
      return 0;
    }
  remove_auto_space(inner_node_index);
  BoxModelNode *inner_node = get_box_model_node(inner_node_index);
  BoxModelState *state = get_current_state();
  double default_thickness = get_underline_thickness(state->font, state->fontsize, state->dpi);
  double inner_height = inner_node->u.hlist.height - inner_node->u.hlist.shift_amount + default_thickness * 5;
  double inner_depth = inner_node->u.hlist.depth + inner_node->u.hlist.shift_amount;

  size_t padded_inner_index = make_hlist();
  append_to_hlist(padded_inner_index, make_hbox(default_thickness * 2.0));
  append_to_hlist(padded_inner_index, inner_node_index);
  append_to_hlist(padded_inner_index, make_hbox(default_thickness * 2.0));
  kern_hlist(padded_inner_index);
  pack_hlist(padded_inner_index, 0, 1);

  size_t hlist_index = make_hlist();

  if (node->u.sqrt.index_length)
    {
      size_t index_hlist_index = make_hlist();
      size_t i;
      for (i = 0; i < node->u.sqrt.index_length; i++)
        {
          append_to_hlist(index_hlist_index, make_char((unsigned int)node->u.sqrt.index_start[i]));
        }
      kern_hlist(index_hlist_index);
      pack_hlist(index_hlist_index, 0, 1);
      shrink(index_hlist_index);
      shrink(index_hlist_index);
      BoxModelNode *inner_hlist_node = get_box_model_node(index_hlist_index);
      inner_hlist_node->u.hlist.shift_amount = -inner_height * 0.5;
      double negative_space = -min(inner_hlist_node->u.hlist.width, (inner_height + inner_depth) * 0.6);
      append_to_hlist(hlist_index, index_hlist_index);
      append_to_hlist(hlist_index, make_kern(negative_space));
    }
  float scaling_factor = (inner_height + inner_depth) / state->fontsize * 0.8;
  size_t sqrt_sign_index = make_auto_height_char(8730, inner_height, inner_depth, NAN);
  append_to_hlist(hlist_index, sqrt_sign_index);
  size_t right_side_index = make_vlist();
  append_to_vlist(right_side_index, make_hrule(default_thickness * scaling_factor));
  append_to_vlist(right_side_index, make_glue(GT_FILL));
  append_to_vlist(right_side_index, padded_inner_index);
  pack_vlist(right_side_index, inner_height + scaling_factor * 0.2 * default_thickness, 0, INFINITY);

  append_to_hlist(hlist_index, right_side_index);
  kern_hlist(hlist_index);
  pack_hlist(hlist_index, 0, 1);


  return hlist_index;
}

static size_t convert_to_box_model(size_t parser_node_index, size_t previous_box_model_node_index)
{
  if (parser_node_index == 0)
    {
      return 0;
    }
  ParserNode *node = get_parser_node(parser_node_index);
  switch (node->type)
    {
    case NT_MATH:
      return convert_math_to_box_model(node);
    case NT_OTHER:
      break;
    case NT_SPACE:
      return convert_space_to_box_model(node);
    case NT_CUSTOMSPACE:
      return convert_custom_space_to_box_model(node);
    case NT_FUNCTION:
      return convert_function_to_box_model(node);
    case NT_OPERATORNAME:
      return convert_operatorname_to_box_model(node);
    case NT_SYMBOL:
      return convert_symbol_to_box_model(node);
    case NT_OVERLINE:
      return convert_overline_to_box_model(node);
    case NT_GROUP:
      return convert_group_to_box_model(node);
    case NT_GENFRAC:
      return convert_genfrac_to_box_model(node);
    case NT_SUBSUPER:
      return convert_subsuper_to_box_model(node, previous_box_model_node_index);
    case NT_AUTO_DELIM:
      return convert_auto_delim_to_box_model(node);
    case NT_C_OVER_C:
      return convert_c_over_c_to_box_model(node);
    case NT_ACCENT:
      return convert_accent_to_box_model(node);
    case NT_SQRT:
      return convert_sqrt_to_box_model(node);
    case NT_LATEXTEXT:
      return convert_latextext_to_box_model(node);
    default:
      break;
    }
  return 0;
}

static void apply_transformation(double *x, double *y)
{
  double x2 = *x;
  double y2 = *y;
  *x = transformation[0] * x2 + transformation[1] * y2 + transformation[4];
  *y = transformation[2] * x2 + transformation[3] * y2 + transformation[5];
}

static void render_character(BoxModelNode *node, double x, double y)
{
  unsigned int codepoint = node->u.character.codepoint;
  codepoint = get_codepoint_for_character_variant(codepoint, node->u.character.state.font);
  y = canvas_height - y;
  unsigned char utf8_str[5] = {0, 0, 0, 0, 0};
  if (codepoint == ' ')
    {
      return;
    }
  if (codepoint > 0 && codepoint < (1U << 7U))
    {
      utf8_str[0] = (unsigned char)codepoint;
    }
  else if (codepoint < (1U << 11U))
    {
      utf8_str[0] = (unsigned char)(((codepoint >> 6U) & 0x3fU) | 0xc0U);
      utf8_str[1] = (unsigned char)(((codepoint >> 0U) & 0x3fU) | 0x80U);
    }
  else if (codepoint < (1U << 16U))
    {
      utf8_str[0] = (unsigned char)(((codepoint >> 12U) & 0x3fU) | 0xe0U);
      utf8_str[1] = (unsigned char)(((codepoint >> 6U) & 0x3fU) | 0x80U);
      utf8_str[2] = (unsigned char)(((codepoint >> 0U) & 0x3fU) | 0x80U);
    }
  else if (codepoint < (1U << 21U))
    {
      utf8_str[0] = (unsigned char)(((codepoint >> 18U) & 0x3fU) | 0xf0U);
      utf8_str[1] = (unsigned char)(((codepoint >> 12U) & 0x3fU) | 0x80U);
      utf8_str[2] = (unsigned char)(((codepoint >> 6U) & 0x3fU) | 0x80U);
      utf8_str[3] = (unsigned char)(((codepoint >> 0U) & 0x3fU) | 0x80U);
    }
  if (!utf8_str[0])
    {
      return;
    }
  /* TODO: inquire current workstation window height? */
  int window_height = 2400;
  gks_set_text_height(node->u.character.state.fontsize / 15.0 * 12 / window_height);
  gks_set_text_fontprec(MATH_FONT, 3);
  gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_BASE);
  if (node->u.character.bearing < 0)
    {
      x += node->u.character.bearing;
      x += node->u.character.advance;
    }
  y += node->u.character.shift_amount;
  apply_transformation(&x, &y);
  gks_select_xform(0);
  x = (x - window[0]) / (window[1] - window[0]);
  y = (y - window[2]) / (window[3] - window[2]);
  gks_text(x, y, (char *)utf8_str);
}

typedef struct Ship_
{
  int max_push;
  int cur_s;
  double cur_h;
  double cur_v;
  double off_h;
  double off_v;
} Ship;


static double clamp(double x)
{
  if (x < -1000000000)
    {
      return -1000000000;
    }
  if (x > 1000000000)
    {
      return 1000000000;
    }
  return x;
}

static void render_rect(double x, double y, double width, double height)
{
  y = canvas_height - y;
  {
    double xs[] = {x, x + width, x + width, x};
    double ys[] = {y, y, y + height, y + height};
    int i;
    for (i = 0; i < 4; i++)
      {
        apply_transformation(xs + i, ys + i);
        xs[i] = (xs[i] - window[0]) / (window[1] - window[0]);
        ys[i] = (ys[i] - window[2]) / (window[3] - window[2]);
      }
    gks_select_xform(0);
    gks_fillarea(4, xs, ys);
  }
}

static void ship_hlist_out(Ship *this, size_t bm_node_index);

static void ship_vlist_out(Ship *this, size_t bm_node_index)
{
  BoxModelNode *box = get_box_model_node(bm_node_index);
  if (!box) return;

  int cur_g = 0;
  double cur_glue = 0.0;
  int glue_order = box->u.vlist.glue_order;
  int glue_sign = box->u.vlist.glue_sign;
  double top_edge = this->cur_v;
  double left_edge = this->cur_h;
  this->cur_s += 1;
  if (this->cur_s > this->max_push)
    {
      this->max_push = this->cur_s;
    }
  this->cur_v -= box->u.vlist.height;

  BoxModelNode *node = box;
  BoxModelNode *vlist_entry;
  size_t vlist_entry_index;
  for (vlist_entry_index = node->u.vlist.first_entry_index; vlist_entry_index;
       vlist_entry_index = get_box_model_node(vlist_entry_index)->u.vlist_entry.next_entry_index)
    {
      vlist_entry = get_box_model_node(vlist_entry_index);
      BoxModelNode *child = get_box_model_node(vlist_entry->u.vlist_entry.node_index);
      if (!child) continue;

      switch (child->type)
        {
        case BT_KERN:
          this->cur_v += child->u.kern.width;
          break;
        case BT_HLIST:
          {
            if (child->u.hlist.first_entry_index == 0)
              {
                this->cur_v += child->u.hlist.height + child->u.hlist.depth;
              }
            else
              {
                this->cur_v += child->u.hlist.height;
                this->cur_h = left_edge + child->u.hlist.shift_amount;
                double save_v = this->cur_v;
                child->u.hlist.width = box->u.vlist.width;
                ship_hlist_out(this, child->index);
                this->cur_v = save_v + child->u.hlist.depth;
                this->cur_h = left_edge;
              }
            break;
          }
        case BT_VLIST:
          {
            if (child->u.vlist.first_entry_index == 0)
              {
                this->cur_v += child->u.vlist.height + child->u.vlist.depth;
              }
            else
              {
                this->cur_v += child->u.vlist.height;
                this->cur_h = left_edge + child->u.vlist.shift_amount;
                double save_v = this->cur_v;
                child->u.vlist.width = box->u.vlist.width;
                ship_vlist_out(this, child->index);
                this->cur_v = save_v + child->u.vlist.depth;
                this->cur_h = left_edge;
              }
            break;
          }
        case BT_HLIST_ENTRY:
          break;
        case BT_VLIST_ENTRY:
          break;
        case BT_HRULE:
          {
            double rule_height = child->u.hrule.height;
            double rule_depth = child->u.hrule.depth;
            double rule_width = child->u.hrule.width;
            if (isinf(rule_width))
              {
                rule_width = node->u.hlist.width;
              }
            rule_height += rule_depth;
            if (rule_height > 0 && rule_depth > 0)
              {
                this->cur_v += rule_height;
                render_rect(this->cur_h + this->off_h, this->cur_v + this->off_v, rule_width, rule_height);
              }
            break;
          }
        case BT_GLUE:
          {
            double rule_height = child->u.glue.spec->width * child->u.glue.factor - cur_g;
            if (glue_sign != 0)
              {
                if (glue_sign == 1 && child->u.glue.spec->stretch_order == glue_order)
                  {
                    cur_glue += child->u.glue.spec->stretch;
                    cur_g = (int)round(clamp(box->u.vlist.glue_set * cur_glue));
                  }
                else if (glue_sign == -1 && child->u.glue.spec->shrink_order == glue_order)
                  {
                    cur_glue += child->u.glue.spec->shrink;
                    cur_g = (int)round(clamp(box->u.vlist.glue_set * cur_glue));
                  }
              }
            rule_height += cur_g;
            this->cur_v += rule_height;
            break;
          }
        case BT_HBOX:
          break;
        case BT_VBOX:
          {
            this->cur_v += child->u.vbox.depth;
            break;
          }
        case BT_CHAR:
          fprintf(stderr, "Error: Char in vlist\n");
          break;
        }
    }


  this->cur_s -= 1;
}

static void ship_hlist_out(Ship *this, size_t bm_node_index)
{
  BoxModelNode *box = get_box_model_node(bm_node_index);
  if (!box) return;
  int cur_g = 0;
  double cur_glue = 0.0;
  int glue_order = box->u.hlist.glue_order;
  int glue_sign = box->u.hlist.glue_sign;
  double base_line = this->cur_v;
  double left_edge = this->cur_h;
  this->cur_s += 1;
  if (this->cur_s > this->max_push)
    {
      this->max_push = this->cur_s;
    }

  BoxModelNode *node = box;
  BoxModelNode *hlist_entry;
  for (hlist_entry = get_box_model_node(node->u.hlist.first_entry_index); hlist_entry;
       hlist_entry = get_box_model_node(hlist_entry->u.hlist_entry.next_entry_index))
    {
      BoxModelNode *child = get_box_model_node(hlist_entry->u.hlist_entry.node_index);
      if (!child) continue;

      switch (child->type)
        {
        case BT_CHAR:
          render_character(child, this->cur_h + this->off_h, this->cur_v + this->off_v);
          this->cur_h += child->u.character.width;
          break;
        case BT_KERN:
          this->cur_h += child->u.kern.width;
          break;
        case BT_HLIST:
          if (child->u.hlist.first_entry_index == 0)
            {
              this->cur_h += child->u.hlist.width;
            }
          else
            {
              double edge = this->cur_h;
              this->cur_v = base_line + child->u.hlist.shift_amount;
              ship_hlist_out(this, child->index);
              this->cur_h = edge + child->u.hlist.width;
              this->cur_v = base_line;
            }
          break;
        case BT_VLIST:
          if (child->u.vlist.first_entry_index == 0)
            {
              this->cur_h += child->u.vlist.width;
            }
          else
            {
              double edge = this->cur_h;
              this->cur_v = base_line + child->u.vlist.shift_amount;
              ship_vlist_out(this, child->index);
              this->cur_h = edge + child->u.vlist.width;
              this->cur_v = base_line;
            }
          break;
        case BT_HLIST_ENTRY:
          break;
        case BT_VLIST_ENTRY:
          break;
        case BT_HRULE:
          {
            double rule_height = child->u.hrule.height;
            double rule_depth = child->u.hrule.depth;
            double rule_width = child->u.hrule.width;
            if (isinf(rule_height))
              {
                rule_height = node->u.hlist.height;
              }
            if (isinf(rule_depth))
              {
                rule_depth = node->u.hlist.depth;
              }
            if (rule_height > 0 && rule_width > 0)
              {
                this->cur_v = base_line + rule_depth;
                render_rect(this->cur_h + this->off_h, this->cur_v + this->off_v, rule_width, rule_height);
                this->cur_v = base_line;
              }
            this->cur_h += rule_width;
            break;
          }
        case BT_GLUE:
          {
            double rule_width = child->u.glue.spec->width * child->u.glue.factor - cur_g;
            if (glue_sign != 0)
              {
                if (glue_sign == 1 && child->u.glue.spec->stretch_order == glue_order)
                  {
                    cur_glue += child->u.glue.spec->stretch;
                    cur_g = (int)round(clamp(box->u.hlist.glue_set * cur_glue));
                  }
                else if (glue_sign == -1 && child->u.glue.spec->shrink_order == glue_order)
                  {
                    cur_glue += child->u.glue.spec->shrink;
                    cur_g = (int)round(clamp(box->u.hlist.glue_set * cur_glue));
                  }
              }
            rule_width += cur_g;
            this->cur_h += rule_width;
            break;
          }
        case BT_HBOX:
          {
            this->cur_h += child->u.hbox.width;
            break;
          }
        case BT_VBOX:
          break;
        }
    }

  this->cur_s -= 1;
}


static void ship(double ox, double oy, size_t bm_node_index)
{
  BoxModelNode *box = get_box_model_node(bm_node_index);
  if (!box) return;
  assert(box->type == BT_HLIST);
  Ship ship;
  ship.max_push = 0;
  ship.cur_s = 0;
  ship.cur_v = 0.0;
  ship.cur_h = 0.0;
  ship.off_h = ox;
  ship.off_v = oy + box->u.hlist.height;
  ship_hlist_out(&ship, bm_node_index);
}

static void get_results(size_t bm_node_index, double *width, double *height, double *depth)
{
  ship(0, 0, bm_node_index);
  BoxModelNode *box = get_box_model_node(bm_node_index);
  assert(box->type == BT_HLIST);
  *width = ceil(box->u.hlist.width);
  *height = ceil(ceil(box->u.hlist.height) + ceil(box->u.hlist.depth));
  *depth = ceil(box->u.hlist.depth);
}


static void mathtex_to_box_model(const char *mathtex, double *width, double *height, double *depth)
{
  state = OUTSIDE_SYMBOL;
  symbol_start = NULL;
  ignore_whitespace = 0;
  input = mathtex;
  cursor = input;
  yyparse();
  if (has_parser_error)
    {
      return;
    }
  result_box_model_node_index = convert_to_box_model(result_parser_node_index, 0);
  kern_hlist(result_box_model_node_index);
  pack_hlist(result_box_model_node_index, 0.0, 1);
  BoxModelNode *result_node = get_box_model_node(result_box_model_node_index);
  assert(get_box_model_node(result_box_model_node_index)->type == BT_HLIST);
  canvas_height = result_node->u.hlist.height + result_node->u.hlist.depth;
  canvas_width = result_node->u.hlist.width;
  if (width)
    {
      *width = result_node->u.hlist.width;
    }
  if (height)
    {
      *height = result_node->u.hlist.height;
    }
  if (depth)
    {
      *depth = result_node->u.hlist.depth;
    }
}

static void calculate_alignment_offsets(int horizontal_alignment, int vertical_alignment, double *x_offset,
                                        double *y_offset)
{
  /* TODO: use actual window height */
  int window_width = 2400;
  int window_height = 2400;
  switch (horizontal_alignment)
    {
    case GKS_K_TEXT_HALIGN_RIGHT:
      *x_offset = -canvas_width / window_width;
      break;
    case GKS_K_TEXT_HALIGN_CENTER:
      *x_offset = -canvas_width / window_width / 2;
      break;
    case GKS_K_TEXT_HALIGN_NORMAL:
    case GKS_K_TEXT_HALIGN_LEFT:
    default:
      *x_offset = 0;
      break;
    }
  switch (vertical_alignment)
    {
    case GKS_K_TEXT_VALIGN_TOP:
      *y_offset = -1.1 * canvas_height / window_height;
      break;
    case GKS_K_TEXT_VALIGN_CAP:
      *y_offset = -canvas_height / window_height;
      break;
    case GKS_K_TEXT_VALIGN_HALF:
      *y_offset = -canvas_height / window_height / 2;
      break;
    case GKS_K_TEXT_VALIGN_BOTTOM:
      *y_offset = 0.1 * canvas_height / window_height;
      break;
    case GKS_K_TEXT_VALIGN_NORMAL:
    case GKS_K_TEXT_VALIGN_BASE:
    default:
      *y_offset = 0;
      break;
    }
}

static void render_box_model(double x, double y, int horizontal_alignment, int vertical_alignment)
{
  int unused;
  int fillcolorind = 1;
  double width, height, depth;
  /* TODO: use actual window height */
  int window_width = 2400;
  int window_height = 2400;
  double x_offset = 0;
  double y_offset = 0;
  gks_set_viewport(1, 0, 1, 0, 1);
  gks_inq_text_color_index(&unused, &fillcolorind);
  gks_set_fill_color_index(fillcolorind);
  gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);
  calculate_alignment_offsets(horizontal_alignment, vertical_alignment, &x_offset, &y_offset);
  transformation[4] += x_offset * window_width * transformation[0] + y_offset * window_height * transformation[1];
  transformation[5] += x_offset * window_width * transformation[2] + y_offset * window_height * transformation[3];
  window[0] = -x * window_width;
  window[1] = (1 - x) * window_width;
  window[2] = -y * window_height;
  window[3] = (1 - y) * window_height;
  get_results(result_box_model_node_index, &width, &height, &depth);
}


static unsigned int get_codepoint_for_character_variant(unsigned int codepoint, FontVariant variant)
{
  switch (variant)
    {
    case FV_RM:
      if (codepoint == 981)
        {
          return 0x1D6FC + 21;
        }
      if (codepoint == 966)
        {
          return 0x1D6FC + 29;
        }
      if (945 <= codepoint && codepoint <= 969)
        {
          return codepoint + 0x1D6FC - 945;
        }
      if (codepoint == 8706)
        {
          return 0x1D715;
        }
      return codepoint;
    case FV_IT:
      if (codepoint == 'h')
        {
          return 0x210E;
        }
      if ('A' <= codepoint && codepoint <= 'Z')
        {
          return codepoint + 0x1D434 - 'A';
        }
      if ('a' <= codepoint && codepoint <= 'z')
        {
          return codepoint + 0x1D44E - 'a';
        }
      if (codepoint == 981)
        {
          return 0x1D6FC + 21;
        }
      if (codepoint == 966)
        {
          return 0x1D6FC + 29;
        }
      if (945 <= codepoint && codepoint <= 969)
        {
          return codepoint + 0x1D6FC - 945;
        }
      if (codepoint == 8706)
        {
          return 0x1D715;
        }
      return codepoint;
    case FV_BF:
      if ('A' <= codepoint && codepoint <= 'Z')
        {
          return codepoint + 0x1D400 - 'A';
        }
      if ('a' <= codepoint && codepoint <= 'z')
        {
          return codepoint + 0x1D41A - 'a';
        }
      if ('0' <= codepoint && codepoint <= '9')
        {
          return codepoint + 0x1D7CE - '0';
        }
      if (codepoint == 981)
        {
          return 0x1D6FC + 21;
        }
      if (codepoint == 966)
        {
          return 0x1D6FC + 29;
        }
      if (945 <= codepoint && codepoint <= 969)
        {
          return codepoint + 0x1D6FC - 945;
        }
      if (913 <= codepoint && codepoint <= 937)
        {
          return codepoint + 0x1D6A8 - 913;
        }
      if (codepoint == 8706)
        {
          return 0x1D715;
        }
      return codepoint;
    case FV_TT:
      if ('A' <= codepoint && codepoint <= 'Z')
        {
          return codepoint + 0x1D670 - 'A';
        }
      if ('a' <= codepoint && codepoint <= 'z')
        {
          return codepoint + 0x1D68A - 'a';
        }
      if ('0' <= codepoint && codepoint <= '9')
        {
          return codepoint + 0x1D7F6 - '0';
        }
      return codepoint;
    case FV_FRAK:
      switch (codepoint)
        {
        case 'C':
          return 0x212D;
        case 'H':
          return 0x210C;
        case 'I':
          return 0x2111;
        case 'R':
          return 0x211C;
        case 'Z':
          return 0x2128;
        default:
          if ('A' <= codepoint && codepoint <= 'Z')
            {
              return codepoint + 0x1D504 - 'A';
            }
          if ('a' <= codepoint && codepoint <= 'z')
            {
              return codepoint + 0x1D51E - 'a';
            }
          return codepoint;
        }
    case FV_CAL:
      switch (codepoint)
        {
        case 'B':
          return 0x212C;
        case 'E':
          return 0x2130;
        case 'F':
          return 0x2131;
        case 'H':
          return 0x210B;
        case 'I':
          return 0x2110;
        case 'L':
          return 0x2112;
        case 'M':
          return 0x2133;
        case 'R':
          return 0x211B;
        default:
          if ('A' <= codepoint && codepoint <= 'Z')
            {
              return codepoint + 0x1D49C - 'A';
            }
          /* italic instead of caligraphic, as lowercase script glyphs are missing. */
          if (codepoint == 'h')
            {
              return 0x210E;
            }
          if ('a' <= codepoint && codepoint <= 'z')
            {
              return codepoint + 0x1D44E - 'a';
            }
          return codepoint;
        }
    case FV_BB:
      switch (codepoint)
        {
        case 'C':
          return 0x2102;
        case 'H':
          return 0x210D;
        case 'N':
          return 0x2115;
        case 'P':
          return 0x2119;
        case 'Q':
          return 0x211A;
        case 'R':
          return 0x211D;
        case 'Z':
          return 0x2124;
        default:
          if ('A' <= codepoint && codepoint <= 'Z')
            {
              return codepoint + 0x1D538 - 'A';
            }
          if ('a' <= codepoint && codepoint <= 'z')
            {
              return codepoint + 0x1D552 - 'a';
            }
          if ('0' <= codepoint && codepoint <= '9')
            {
              return codepoint + 0x1D7D8 - '0';
            }
          return codepoint;
        }
    default:
      return codepoint;
    }
}

void mathtex2(double x, double y, const char *formula, int inquire, double *tbx, double *tby)
{
  int unused;
  int previous_bearing_x_direction;
  double previous_char_height;
  double chupx = 0;
  double chupy = 0;
  int previous_tnr;
  int previous_fill_int_style;
  int previous_fill_color_index = 0;
  int previous_encoding = ENCODING_LATIN1;
  int horizontal_alignment = GKS_K_TEXT_HALIGN_NORMAL;
  int vertical_alignment = GKS_K_TEXT_VALIGN_NORMAL;
  int font;
  int prec;
  double previous_viewport_xmin, previous_viewport_xmax, previous_viewport_ymin, previous_viewport_ymax;
  /* TODO: inquire current workstation window height? */
  int window_width = 2400;
  int window_height = 2400;

  double tbx_fallback[4];
  double tby_fallback[4];
  /* use fallback arrays to simplify handling of tbx and tby */
  if (!tbx)
    {
      tbx = tbx_fallback;
    }
  if (!tby)
    {
      tby = tby_fallback;
    }

  has_parser_error = 0;
  gks_ft_inq_bearing_x_direction(&previous_bearing_x_direction);
  gks_ft_set_bearing_x_direction(1);
  /* gr call first to ensure autoinit has run */
  gr_inqviewport(&previous_viewport_xmin, &previous_viewport_xmax, &previous_viewport_ymin, &previous_viewport_ymax);
  gks_inq_current_xformno(&unused, &previous_tnr);
  gks_inq_text_fontprec(&unused, &font, &prec);
  gks_inq_text_align(&unused, &horizontal_alignment, &vertical_alignment);
  gks_inq_fill_color_index(&unused, &previous_fill_color_index);
  gks_inq_fill_int_style(&unused, &previous_fill_int_style);
  gks_inq_encoding(&previous_encoding);
  gks_set_encoding(ENCODING_UTF8);
  gks_inq_text_height(&unused, &previous_char_height);
  gks_inq_text_upvec(&unused, &chupx, &chupy);
  if (chupx * chupx + chupy * chupy == 0)
    {
      chupx = 0;
      chupy = 1;
    }
  else
    {
      double chup_length = sqrt(chupx * chupx + chupy * chupy);
      chupx /= chup_length;
      chupy /= chup_length;
    }
  transformation[0] = chupy;
  transformation[1] = chupx;
  transformation[2] = -chupx;
  transformation[3] = chupy;
  /* transformation offsets depend on the canvas size */
  transformation[4] = 0;
  transformation[5] = 0;
  font_size = 16.0 * previous_char_height / 0.027 * window_height / 500;
  mathtex_to_box_model(formula, NULL, NULL, NULL);
  if (!has_parser_error)
    {
      double x_offset = 0;
      double y_offset = 0;
      if (!inquire)
        {
          render_box_model(x, y, horizontal_alignment, vertical_alignment);
        }
      else
        {
          double xmin, xmax, ymin, ymax;
          double angle;
          int i;
          calculate_alignment_offsets(horizontal_alignment, vertical_alignment, &x_offset, &y_offset);
          xmin = x + x_offset;
          xmax = x + x_offset + canvas_width / window_width;
          ymin = y + y_offset;
          ymax = y + y_offset + canvas_height / window_height;
          tbx[0] = xmin;
          tbx[1] = xmax;
          tbx[2] = xmax;
          tbx[3] = xmin;
          tby[0] = ymin;
          tby[1] = ymin;
          tby[2] = ymax;
          tby[3] = ymax;
          angle = -atan2(chupx, chupy);
          for (i = 0; i < 4; i++)
            {
              double rx, ry;
              rx = tbx[i] - x;
              ry = tby[i] - y;
              tbx[i] = x + rx * cos(angle) - ry * sin(angle);
              tby[i] = y + rx * sin(angle) + ry * cos(angle);
            }
        }
    }
  else if (inquire)
    {
      tbx[0] = x;
      tbx[1] = x;
      tbx[2] = x;
      tbx[3] = x;

      tby[0] = y;
      tby[1] = y;
      tby[2] = y;
      tby[3] = y;
    }

  free_parser_node_buffer();
  free_box_model_node_buffer();
  free_box_model_state_buffer();
  current_box_model_state_index = 0;

  gks_ft_set_bearing_x_direction(previous_bearing_x_direction);
  gks_set_text_height(previous_char_height);
  gks_set_encoding(previous_encoding);
  gks_set_text_fontprec(font, prec);
  gks_set_text_align(horizontal_alignment, vertical_alignment);
  gks_set_fill_color_index(previous_fill_color_index);
  gks_set_fill_int_style(previous_fill_int_style);
  gks_set_viewport(1, previous_viewport_xmin, previous_viewport_xmax, previous_viewport_ymin, previous_viewport_ymax);
  gks_select_xform(previous_tnr);

  if (inquire && previous_tnr != 0)
    {
      int i;
      for (i = 0; i < 4; i++)
        {
          gr_ndctowc(tbx + i, tby + i);
        }
    }
}
