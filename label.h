#ifndef GL_LABEL_H_INCLUDED
#define GL_LABEL_H_INCLUDED

#include "frame.h"
#include "font/font.h"

struct gl_label {
	struct gl_frame frame;
	font_t font;
	int border;
	int color;
	const char *text;
};

#define GL_LABEL(frame)   ((struct gl_label *)frame)

extern int gl_label_create(struct gl_frame **out,
			   int x, int y, int w, int h, int color);

static inline void gl_label_set_text(struct gl_label *label, const char *text)
{
	label->text = text;
}

static inline void gl_label_set_color(struct gl_label *label, int color)
{
	label->color = color;
}

static inline void gl_label_set_font(struct gl_label *label, font_t font)
{
	label->font = font;
}

static inline void gl_label_set_border_width(struct gl_label *label, int border)
{
	label->border = border;
}

#endif  /* GL_LABEL_H_INCLUDED */
