#ifndef GL_SCALE_BAR_H_INCLUDED
#define GL_SCALE_BAR_H_INCLUDED

#include "frame.h"

typedef int (*gl_scale_bar_color_callback_t)(int, int, const void *);

/* Scale pointer direction */
typedef enum gl_scale_pointer_t {
	GL_SCALE_POINTER_NONE,
	GL_SCALE_POINTER_LEFT,
	GL_SCALE_POINTER_RIGHT,
	GL_SCALE_POINTER_TOP,
	GL_SCALE_POINTER_BOTTOM,
} gl_scale_pointer_t;

struct gl_scale_bar {
	struct gl_frame frame;
	gl_scale_pointer_t pointer;
	char pointer_text[8];
	int txt_color;
	int txt_xb, txt_yb;
	int pointer_color;
	gl_scale_bar_color_callback_t callback;
	const void *callback_data;
};

#define GL_SCALE_BAR(frame)   ((struct gl_scale_bar *)frame)

extern int gl_scale_bar_create(struct gl_frame **out,
			       int x, int y, int w, int h, int color);

extern void gl_scale_bar_set_text(struct gl_scale_bar *sbar,
				  const char *txt, int size);

extern void gl_scale_bar_set_pointer(struct gl_scale_bar *sbar,
				     gl_scale_pointer_t pointer);

extern void gl_scale_bar_set_color_callback(struct gl_scale_bar *sbar,
		gl_scale_bar_color_callback_t callback, const void *userdata);

static inline void gl_scale_bar_set_text_color(struct gl_scale_bar *sbar,
					       int color)
{
	sbar->txt_color = color;
}

static inline void gl_scale_bar_set_pointer_color(struct gl_scale_bar *sbar,
						  int color)
{
	sbar->pointer_color = color;
}

#endif	/* GL_SCALE_BAR_H_INCLUDED */
