#ifndef PLAY_H
#define PLAY_H
struct keyword
{
  char *name;
  int type;
  void (*callback) (void);
  void *userdata;
};
typedef void (*funcptr) (void);
#define GENERIC 0
#define PARAM_INTEGER 1
#define PARAM_BOOL 2
#define PARAM_NSTRING 3
#define PARAM_FLOAT 4
#define PARAM_KEYSTRING 5
#define PARAM_COORD 6
#define PARAM_STRING 7
extern CONST char *CONST save_fastmode[];
extern CONST char *CONST uih_colornames[];
void uih_play_formula (struct uih_context *uih, char *name);
void uih_playfilter (struct uih_context *uih, dialogparam * p);
void uih_zoomcenter (struct uih_context *uih, number_t x, number_t y);
void uih_playpalette (struct uih_context *uih);
void uih_playdefpalette (struct uih_context *uih, int shift);
void uih_playusleep (struct uih_context *uih, int time);
void uih_playtextsleep (struct uih_context *uih);
void uih_playwait (struct uih_context *uih);
void uih_playjulia (struct uih_context *uih, int julia);
void uih_playzoom (struct uih_context *uih);
void uih_playunzoom (struct uih_context *uih);
void uih_playstop (struct uih_context *uih);
void uih_playmorph (struct uih_context *uih, dialogparam * p);
void uih_playmove (struct uih_context *uih, number_t x, number_t y);
void uih_playtextpos (struct uih_context *uih, dialogparam * p);
void uih_playcalculate (struct uih_context *uih);
void uih_playmorphjulia (struct uih_context *uih, number_t x, number_t y);
void uih_playmorphangle (struct uih_context *uih, number_t angle);
void uih_playautorotate (struct uih_context *uih, int mode);
void uih_playmessage (struct uih_context *uih, char *message);
void uih_playload (struct uih_context *uih, char *message);
void uih_playinit (struct uih_context *uih);

void uih_line (uih_context * c, dialogparam * d);
void uih_morphline (uih_context * c, dialogparam * d);
void uih_morphlastline (uih_context * c, dialogparam * d);
void uih_setkey (uih_context * c, int line);
void uih_clear_line (uih_context * c);
void uih_clear_lines (uih_context * c);

#endif
