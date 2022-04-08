#ifndef GL_COMPASS_H_INCLUDED
#define GL_COMPASS_H_INCLUDED

#include "frame.h"

struct gl_compass {
	struct gl_frame frame;
	int center_x, center_y;
	int radius;
	int bg_color;
	double sinfi, cosfi;
};

#define GL_COMPASS(frame) ((struct gl_compass *)frame)

extern int gl_compass_create(struct gl_frame **out,
			     int x, int y, int w, int h, int color);

static inline void gl_compass_set_orientation(struct gl_compass *comp,
					      double sinfi, double cosfi)
{
	comp->sinfi = sinfi;
	comp->cosfi = cosfi;
}

static inline void gl_compass_set_bgcolor(struct gl_compass *comp, int color)
{
	comp->bg_color = color;
}

#endif	/* GL_COMPASS_H_INCLUDED */
