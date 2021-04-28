/*
 * This code is derived from matplotlib's mathtext module.
 *
 * The code was rewritten in C and yacc, adapted to the GR framework's text and path rendering, and modified so that
 * its results resemble those of the previous gr_mathtex implementation which uses LaTeX directly.
 *
 * The original mathtext module can be found at:
 * https://github.com/matplotlib/matplotlib/blob/baaec371364deac71de24d8f564fb43f70db7297/lib/matplotlib/mathtext.py
 */

%{
#include "mathtex2.h"


#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

int yylex(void);
void yyerror(char const *);
%}

%define api.value.type {ParserNode}
%token SINGLE_SYMBOL
%token SUBSUPEROP
%token APOSTROPHE
%token FRAC
%token DFRAC
%token STACKREL
%token BINOM
%token GENFRAC
%token SNOWFLAKE
%token ACCENT
%token UNKNOWN_SYMBOL
%token FONT
%token LATEXFONT
%token LATEXTEXT
%token FUNCTION
%token C_OVER_C
%token SPACE
%token HSPACE
%token LEFT
%token RIGHT
%token LEFT_DELIM
%token AMBI_DELIM
%token RIGHT_DELIM
%token LBRACE
%token RBRACE
%token PIPE
%token OPERATORNAME
%token OVERLINE
%token SQRT
%token DIGIT
%token PLUSMINUS

%%

result:
math {
    result_parser_node_index = copy_parser_node($1);
}

math:
token {
    $$ = $1;
    $$.u.math.previous = 0;
    $$.u.math.token = copy_parser_node($1);
    $$.type = NT_MATH;
}
| math token {
    $$ = $1;
    $$.u.math.previous = copy_parser_node($1);
    $$.u.math.token = copy_parser_node($2);
    $$.length += $2.length;
}
;


single_symbol:
SINGLE_SYMBOL { $$ = $1; }
| LEFT_DELIM { $$ = $1; }
| AMBI_DELIM { $$ = $1; }
| RIGHT_DELIM { $$ = $1; }
| '[' { $$ = $1; }
| ']' { $$ = $1; }
| '(' { $$ = $1; }
| ')' { $$ = $1; }
| '|' { $$ = $1; }
| '<' { $$ = $1; }
| '>' { $$ = $1; }
| '/' { $$ = $1; }
| '.' { $$ = $1; }
| LBRACE { $$ = $1; }
| RBRACE { $$ = $1; }
| PIPE { $$ = $1; }
| DIGIT { $$ = $1; }
| PLUSMINUS { $$ = $1; }
;


token:
auto_delim {
    $$ = $1;
}
| simple {
    $$ = $1;
}
| UNKNOWN_SYMBOL {
    $$ = $1;
    $$.type = NT_SYMBOL;
}
;

simple:
SPACE {
    $$ = $1;
    $$.type = NT_SPACE;
}
| customspace {
    $$ = $1;
}
| FONT {
    $$ = $1;
    $$.type = NT_FONT;
}
| subsuper {
    $$ = $1;
}
| placeable {
    $$ = $1;
}
;

customspace:
HSPACE '{' NT_FLOAT '}' {
    $$ = $1;
    $$.length += $2.length + $3.length + $4.length;
    $$.type = NT_CUSTOMSPACE;
}
;

subsuper:
APOSTROPHE {
    $$ = $1;
    $$.index = 0;
    $$.type = NT_SUBSUPER;
    $$.u.subsuper.operator = '\'';
    $$.u.subsuper.token = 0;
}
| SUBSUPEROP placeable {
    $$ = $1;
    $$.index = 0;
    $$.length += $2.length;
    $$.type = NT_SUBSUPER;
    $$.u.subsuper.operator = $1.source[0];
    $$.u.subsuper.token = copy_parser_node($2);
}
;

auto_delim:
left_delim auto_delim_inner right_delim {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_AUTO_DELIM;
    $$.u.autodelim.left_delim_start = $1.source + 5;
    $$.u.autodelim.left_delim_length = $1.length - 5;
    $$.u.autodelim.right_delim_start = $3.source + 6;
    $$.u.autodelim.right_delim_length = $3.length - 6;
    $$.u.autodelim.inner_node_index = copy_parser_node($2);
}
;

ambi_delim_symbol:
'|' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| '/' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| '.' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| AMBI_DELIM {
    $$ = $1;
    $$.type = NT_OTHER;
}
| PIPE {
    $$ = $1;
    $$.type = NT_OTHER;
}
;

left_delim_symbol:
'(' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| '[' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| '<' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| ambi_delim_symbol {
    $$ = $1;
    $$.type = NT_OTHER;
}
| LEFT_DELIM {
    $$ = $1;
    $$.type = NT_OTHER;
}
| LBRACE {
    $$ = $1;
    $$.type = NT_OTHER;
}
;

right_delim_symbol:
')' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| ']' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| '>' {
    $$ = $1;
    $$.type = NT_OTHER;
}
| ambi_delim_symbol {
    $$ = $1;
    $$.type = NT_OTHER;
}
| RIGHT_DELIM {
    $$ = $1;
    $$.type = NT_OTHER;
}
| RBRACE {
    $$ = $1;
    $$.type = NT_OTHER;
}
;

left_delim:
LEFT left_delim_symbol {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length+$2.length;
    $$.type = NT_OTHER;
}
;

right_delim:
RIGHT right_delim_symbol {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length+$2.length;
    $$.type = NT_OTHER;
}
;

auto_delim_inner:
%empty {
    $$.index = 0;
    $$.source = NULL;
    $$.length = 0;
    $$.u.autodeliminner.previous = 0;
    $$.u.autodeliminner.token = 0;
    $$.type = NT_AUTO_DELIM_INNER;
}
| simple auto_delim_inner {
    $$ = $1;
    $$.index = 0;
    $$.type = NT_AUTO_DELIM_INNER;
    $$.u.autodeliminner.previous = copy_parser_node($2);
    $$.u.autodeliminner.token = copy_parser_node($1);
    $$.length += $2.length;
}
| auto_delim auto_delim_inner {
    $$ = $1;
    $$.index = 0;
    $$.type = NT_AUTO_DELIM_INNER;
    $$.u.autodeliminner.previous = copy_parser_node($2);
    $$.u.autodeliminner.token = copy_parser_node($1);
    $$.length += $2.length;
}
;

accent:
ACCENT placeable {
    $$.index = 0;
    $$.type = NT_ACCENT;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.u.accent.token = copy_parser_node($2);
}

placeable:
SNOWFLAKE {
    $$ = $1;
    $$.type = NT_SYMBOL;
}
| accent {
    $$ = $1;
}
| single_symbol {
    $$ = $1;
    $$.type = NT_SYMBOL;
}
| C_OVER_C {
    $$ = $1;
    $$.type = NT_C_OVER_C;
}
| FUNCTION {
    $$ = $1;
    $$.type = NT_FUNCTION;
}
| group {
    $$ = $1;
}
| LATEXTEXT {
    $$ = $1;
    $$.type = NT_LATEXTEXT;
}
| frac {
    $$ = $1;
}
| dfrac {
    $$ = $1;
}
| stackrel {
   $$ = $1;
}
| binom {
    $$ = $1;
}
| genfrac {
    $$ = $1;
}
| sqrt {
    $$ = $1;
}
| overline {
    $$ = $1;
}
| operatorname {
    $$ = $1;
}
;

genfrac:
GENFRAC '{' left_delim_symbol '}' '{' right_delim_symbol '}' '{' NT_FLOAT '}' simple_group required_group required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length + $4.length + $5.length + $6.length + $7.length + $8.length + $9.length + $10.length + $11.length + $12.length + $13.length;
    $$.type = NT_GENFRAC;
    $$.u.genfrac.left_delim_start = $3.source;
    $$.u.genfrac.left_delim_length = $3.length;
    $$.u.genfrac.right_delim_start = $6.source;
    $$.u.genfrac.right_delim_length = $6.length;
    double thickness = 0;
    int n = sscanf($9.source, "\\hspace{%lf}", &thickness);
    if (n != 1) {
        thickness = NAN;
    }
    $$.u.genfrac.thickness = thickness;
    $$.u.genfrac.style_text_start = $11.source;
    $$.u.genfrac.style_text_length = $11.length;
    $$.u.genfrac.numerator_group = copy_parser_node($12);
    $$.u.genfrac.denominator_group = copy_parser_node($13);
}
;

overline:
OVERLINE required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OVERLINE;
    $$.u.overline.body = copy_parser_node($2);
}
;

operatorname:
OPERATORNAME '{' operatorname_inner '}' {
    $$ = $3;
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length + $4.length;
    $$.type = NT_OPERATORNAME;
}
;

operatorname_inner:
%empty {
    $$.index = 0;
    $$.source = NULL;
    $$.length = 0;
    $$.type = NT_OTHER;
    $$.u.operatorname.previous = 0;
    $$.u.operatorname.token = 0;
}
| simple operatorname_inner {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
    $$.u.operatorname.previous = copy_parser_node($2);
    $$.u.operatorname.token = copy_parser_node($1);
}
| UNKNOWN_SYMBOL operatorname_inner {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
    $$.u.operatorname.previous = copy_parser_node($2);
    $$.u.operatorname.token = copy_parser_node($1);
}
;

sqrt:
SQRT required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_SQRT;
    $$.u.sqrt.index_start = "";
    $$.u.sqrt.index_length = 0;
    $$.u.sqrt.token = copy_parser_node($2);
}
| SQRT '[' int ']' required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length + $4.length + $5.length;
    $$.type = NT_SQRT;
    $$.u.sqrt.index_start = $3.source;
    $$.u.sqrt.index_length = $3.length;
    $$.u.sqrt.token = copy_parser_node($5);
}
;

group:
start_group group_inner '}' {
    $$ = $2;
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GROUP;
}
;

simple_group:
'{' group_inner '}' {
    $$ = $2;
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GROUP;
}
;

required_group:
'{' required_group_inner '}' {
    $$ = $2;
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GROUP;
}
;

required_group_inner:
token {
    $$ = $1;
    $$.type = NT_OTHER;
    $$.u.group.previous = 0;
    $$.u.group.token = copy_parser_node($1);
}
| token required_group_inner {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
    $$.u.group.previous = copy_parser_node($2);
    $$.u.group.token = copy_parser_node($1);
}
;

start_group:
LATEXFONT '{' {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
}
| '{' {
    $$ = $1;
    $$.type = NT_OTHER;
}
;

group_inner:
%empty {
    $$.index = 0;
    $$.source = NULL;
    $$.length = 0;
    $$.type = NT_OTHER;
    $$.u.group.previous = 0;
    $$.u.group.token = 0;
}
| token group_inner {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
    $$.u.group.previous = copy_parser_node($2);
    $$.u.group.token = copy_parser_node($1);
}
;

frac:
FRAC required_group required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GENFRAC;
    $$.u.genfrac.left_delim_start = "";
    $$.u.genfrac.left_delim_length = 0;
    $$.u.genfrac.right_delim_start = "";
    $$.u.genfrac.right_delim_length = 0;
    $$.u.genfrac.thickness = NAN;
    $$.u.genfrac.style_text_start = "{0}";
    $$.u.genfrac.style_text_length = 3;
    $$.u.genfrac.numerator_group = copy_parser_node($2);
    $$.u.genfrac.denominator_group = copy_parser_node($3);
}
;

dfrac:
DFRAC required_group required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GENFRAC;
    $$.u.genfrac.left_delim_start = "";
    $$.u.genfrac.left_delim_length = 0;
    $$.u.genfrac.right_delim_start = "";
    $$.u.genfrac.right_delim_length = 0;
    $$.u.genfrac.thickness = NAN;
    $$.u.genfrac.style_text_start = "{1}";
    $$.u.genfrac.style_text_length = 3;
    $$.u.genfrac.numerator_group = copy_parser_node($2);
    $$.u.genfrac.denominator_group = copy_parser_node($3);
}
;

stackrel:
STACKREL required_group required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GENFRAC;
    $$.u.genfrac.left_delim_start = "";
    $$.u.genfrac.left_delim_length = 0;
    $$.u.genfrac.right_delim_start = "";
    $$.u.genfrac.right_delim_length = 0;
    $$.u.genfrac.thickness = 0.0;
    $$.u.genfrac.style_text_start = "{1}";
    $$.u.genfrac.style_text_length = 3;
    $$.u.genfrac.numerator_group = copy_parser_node($2);
    $$.u.genfrac.denominator_group = copy_parser_node($3);
}
;

binom:
BINOM required_group required_group {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_GENFRAC;
    $$.u.genfrac.left_delim_start = "(";
    $$.u.genfrac.left_delim_length = 1;
    $$.u.genfrac.right_delim_start = ")";
    $$.u.genfrac.right_delim_length = 1;
    $$.u.genfrac.thickness = 0.0;
    $$.u.genfrac.style_text_start = "{1}";
    $$.u.genfrac.style_text_length = 3;
    $$.u.genfrac.numerator_group = copy_parser_node($2);
    $$.u.genfrac.denominator_group = copy_parser_node($3);
}
;


NT_FLOAT:
int { $$ = $1; }
| '.' uint {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_FLOAT;
}
| PLUSMINUS '.' uint {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_FLOAT;
}
| int '.' {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_FLOAT;
}
| int '.' uint {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length + $3.length;
    $$.type = NT_FLOAT;
}
;

int:
uint {
    $$ = $1;
    $$.type = NT_INTEGER;
}
| PLUSMINUS uint {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_INTEGER;
}

uint:
DIGIT {
    $$ = $1;
    $$.type = NT_OTHER;
}
| uint DIGIT {
    $$.index = 0;
    $$.source = $1.source;
    $$.length = $1.length + $2.length;
    $$.type = NT_OTHER;
}
;

%%



const char *snowflake_symbols[] = {"\\doteqdot", "\\doteq", "\\dotminus", "\\barleftarrow", "\\ddots", "\\dotplus", "\\dots", "\\barwedge"};
const char *accent_symbols[] = {"\\hat", "\\breve", "\\bar", "\\grave", "\\acute", "\\tilde", "\\dot", "\\ddot", "\\vec", "\\overrightarrow", "\\overleftarrow", "\\mathring", "\\widebar", "\\widehat", "\\widetilde"};
const char *font_symbols[] = {"\\rm", "\\cal", "\\it", "\\tt", "\\sf", "\\bf", "\\default", "\\bb", "\\frak", "\\circled", "\\scr", "\\regular"};
const char *latexfont_symbols[] = {"\\mathrm", "\\mathcal", "\\mathit", "\\mathtt", "\\mathsf", "\\mathbf", "\\mathdefault", "\\mathbb", "\\mathfrak", "\\mathcircled", "\\mathscr", "\\mathregular", "\\textrm", "\\textit", "\\textbf", "\\texttt"};
const char *c_over_c_symbols[] = {"\\AA"};
const char *space_symbols[] = {"\\thinspace", "\\enspace", "\\quad", "\\qquad"};
const char *left_delim_symbols[] = {"\\int", "\\lfloor", "\\langle", "\\lceil", "\\sum"};
const char *ambi_delim_symbols[] = {"\\backslash", "\\uparrow", "\\downarrow", "\\updownarrow", "\\Uparrow", "\\Downarrow", "\\Updownarrow", "\\vert", "\\Vert"};
const char *right_delim_symbols[] = {"\\rfloor", "\\rangle", "\\rceil"};
const char *function_symbols[] = {"\\arccos", "\\csc", "\\ker", "\\min", "\\arcsin", "\\deg", "\\lg", "\\Pr", "\\arctan", "\\det", "\\lim", "\\sec", "\\arg", "\\dim", "\\liminf", "\\sin", "\\cos", "\\exp", "\\limsup", "\\sinh", "\\cosh", "\\gcd", "\\ln", "\\sup", "\\cot", "\\hom", "\\log", "\\tan", "\\coth", "\\inf", "\\max", "\\tanh"};

int symbol_in_symbol_list(const char *symbol, size_t length, const char **symbol_list, size_t num_symbols) {
  int i;
  for (i = 0; i < num_symbols; i++) {
    if (strncmp(symbol, symbol_list[i], length) == 0 && symbol_list[i][length] == 0) {
      return 1;
    }
  }
  return 0;
}

int symbol_is_snowflake(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, snowflake_symbols, sizeof(snowflake_symbols) / sizeof(const char *));
}

int symbol_is_accent(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, accent_symbols, sizeof(accent_symbols) / sizeof(const char *));
}

int symbol_is_font(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, font_symbols, sizeof(font_symbols) / sizeof(const char *));
}

int symbol_is_latexfont(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, latexfont_symbols, sizeof(latexfont_symbols) / sizeof(const char *));
}

int symbol_is_c_over_c(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, c_over_c_symbols, sizeof(c_over_c_symbols) / sizeof(const char *));
}

int symbol_is_space(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, space_symbols, sizeof(space_symbols) / sizeof(const char *));
}

int symbol_is_left_delim(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, left_delim_symbols, sizeof(left_delim_symbols) / sizeof(const char *));
}

int symbol_is_ambi_delim(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, ambi_delim_symbols, sizeof(ambi_delim_symbols) / sizeof(const char *));
}

int symbol_is_right_delim(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, right_delim_symbols, sizeof(right_delim_symbols) / sizeof(const char *));
}
int symbol_is_function(const char *symbol, size_t length) {
  return symbol_in_symbol_list(symbol, length, function_symbols, sizeof(function_symbols) / sizeof(const char *));
}



const char *cursor;
enum State state;
const char *symbol_start;
int ignore_whitespace;

int yylex(void) {
  yylval.index = 0;
  for (; *cursor != 0 || state == INSIDE_SYMBOL; cursor++) {
    int c = *cursor;
    if (c == ' ' && ignore_whitespace) {
        ignore_whitespace = 0;
        continue;
    }
    ignore_whitespace = 0;
    switch (state) {
    case OUTSIDE_SYMBOL:
    {
      if ('0' <= c && c <= '9') {
        yylval.source = cursor;
        yylval.length = 1;
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return DIGIT;
      } else if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (0x80 <= c && c <= 0x1ffff) || strchr(" *,=:;!?&'@", c) != NULL) {
        if (c == ' ') {
            break;
        }
        yylval.source = cursor;
        yylval.length = 1;
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return SINGLE_SYMBOL;
      } else if (strchr("()[]<>./{}|", c) != NULL) {
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          return c;
      } else {
        switch(c) {
        case '\\':
          state = INSIDE_SYMBOL;
          symbol_start = cursor;
          break;
        case '_':
        case '^':
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          return SUBSUPEROP;
        case '\'':
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          while (*cursor == '\'') {
            yylval.length = 1;
            cursor += 1;
          }
          return APOSTROPHE;
        case '~':
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          return SPACE;
        case '-':
        case '+':
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          return PLUSMINUS;
        default:
          yylval.source = cursor;
          yylval.length = 1;
          yylval.type = NT_TERMINAL_SYMBOL;
          cursor += 1;
          return c;
        }
      }
    }
      break;
    case INSIDE_SYMBOL:
    {
      if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
        /* valid part of symbol */
      } else if (c == '{' && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return LBRACE;
      } else if (c == '}' && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return RBRACE;
      } else if (c == '|' && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return PIPE;
      } else if (strchr(",/>:; !", c) != NULL && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return SPACE;
      } else if (strchr("%$[]_#", c) != NULL && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return SINGLE_SYMBOL;
      } else if (strchr("\"`'~.^", c) != NULL && cursor == symbol_start+1) {
        state = OUTSIDE_SYMBOL;
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start + 1);
        yylval.type = NT_TERMINAL_SYMBOL;
        cursor += 1;
        return ACCENT;
      } else {
        state = OUTSIDE_SYMBOL;
        int result;
        yylval.type = NT_TERMINAL_SYMBOL;
        if (strncmp("\\frac", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = FRAC;
        } else if (strncmp("\\dfrac", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = DFRAC;
        } else if (strncmp("\\stackrel", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = STACKREL;
        } else if (strncmp("\\binom", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = BINOM;
        } else if (strncmp("\\genfrac", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = GENFRAC;
        } else if (strncmp("\\operatorname", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = OPERATORNAME;
        } else if (strncmp("\\overline", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = OVERLINE;
        } else if (strncmp("\\sqrt", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = SQRT;
        } else if (strncmp("\\hspace", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = HSPACE;
        } else if (strncmp("\\left", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = LEFT;
        } else if (strncmp("\\right", symbol_start, (int)(cursor - symbol_start)) == 0) {
          result = RIGHT;
        } else if (symbol_is_snowflake(symbol_start, (int)(cursor - symbol_start))) {
          result = SNOWFLAKE;
        } else if (symbol_is_accent(symbol_start, (int)(cursor - symbol_start))) {
          result = ACCENT;
        } else if (symbol_is_font(symbol_start, (int)(cursor - symbol_start))) {
          result = FONT;
        } else if (symbol_is_latexfont(symbol_start, (int)(cursor - symbol_start))) {
          result = LATEXFONT;
        } else if (symbol_is_function(symbol_start, (int)(cursor - symbol_start))) {
          result = FUNCTION;
        } else if (symbol_is_c_over_c(symbol_start, (int)(cursor - symbol_start))) {
          result = C_OVER_C;
        } else if (symbol_is_space(symbol_start, (int)(cursor - symbol_start))) {
          result = SPACE;
        } else if (symbol_is_left_delim(symbol_start, (int)(cursor - symbol_start))) {
          result = LEFT_DELIM;
        } else if (symbol_is_ambi_delim(symbol_start, (int)(cursor - symbol_start))) {
          result = AMBI_DELIM;
        } else if (symbol_is_right_delim(symbol_start, (int)(cursor - symbol_start))) {
          result = RIGHT_DELIM;
        } else {
          yylval.type = NT_SYMBOL;
          result = SINGLE_SYMBOL;
          ignore_whitespace = 1;
        }
        yylval.source = symbol_start;
        yylval.length = (int)(cursor - symbol_start);
        if (result == LATEXFONT && *cursor == '{' && strncmp(symbol_start, "\\text", 5) == 0) {
            const char *text_end = strchr(symbol_start, '}');
            if (text_end && (strrchr(text_end, '{') == cursor)) {
                yylval.length = (int)(text_end - symbol_start);
                cursor = text_end+1;
                result = LATEXTEXT;
            }
        }
        return result;
      }
    }
      break;
    default:
      break;
    }
  }
  return 0;
}
