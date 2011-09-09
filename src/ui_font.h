/*********************************************************************

  ui_font.h


*********************************************************************/

#ifndef UI_FONT_H
#define UI_FONT_H

int uifont_buildfont(void);
int uifont_need_font_warning(void);
void uifont_freefont(void);
int uifont_decodechar(unsigned char *s, unsigned short *code);
int uifont_drawfont(struct mame_bitmap *bitmap, const char *s,
                    int sx, int sy, int color);

#ifdef UI_COLOR_DISPLAY
void convert_command_move(char *buf);
#endif /* UI_COLOR_DISPLAY */

enum {
	UI_LANG_EN_US = 0,
	UI_LANG_MAX
};

struct ui_lang_info_t
{
	const char *name;
	const char *shortname;
	const char *description;
	int         codepage;
	int         numchars;
};


extern struct ui_lang_info_t ui_lang_info[UI_LANG_MAX];

extern int lang_find_langname(const char *name);
extern int lang_find_codepage(int cp);

extern void set_langcode(int langcode);

#endif
