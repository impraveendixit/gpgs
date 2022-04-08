#ifndef GL_CLOCK_H_INCLUDED
#define GL_CLOCK_H_INCLUDED

#include "frame.h"

struct gl_clock {
	struct gl_frame frame;
	int xb, yb;
	int radius;
	int bg_color;
	char text[8];
};

#define GL_CLOCK(frame)   ((struct gl_clock *)frame)

extern void gl_clock_set_text(struct gl_clock *clk,
			      const char *text, int size);

extern int gl_clock_create(struct gl_frame **out,
			   int x, int y, int w, int h, int color);

static inline void gl_clock_set_bgcolor(struct gl_clock *clk, int color)
{
	clk->bg_color = color;
}

#endif	/* GL_CLOCK_H_INCLUDED */
