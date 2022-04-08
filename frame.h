#ifndef GL_FRAME_H_INCLUDED
#define GL_FRAME_H_INCLUDED

#include "list.h"
#include "internals.h"

struct gl_frame;

typedef void (*gl_frame_callback_t)(struct gl_frame *, const void *);

struct gl_frame {
	int xb, yb;
	int width, height;
	int color;
	gl_frame_callback_t callback;
	const void *callback_data;
	struct list_head list;
	rc_t rc;
	void (*draw)(struct gl_frame *frm);
	void (*destroy)(struct gl_frame *frm);
};

#define GL_FRAME(frm)   ((struct gl_frame *)frm)

extern void gl_frame_init(struct gl_frame *frm,
			  int x, int y, int w, int h, int color);

extern void gl_frame_add_callback(struct gl_frame *frm, rc_t rc,
				  gl_frame_callback_t callback,
				  const void *data);

extern void gl_frame_draw(struct gl_frame *frm, unsigned int no_border);

extern void gl_frame_destroy(struct gl_frame *frm);

static inline void gl_frame_set_color(struct gl_frame *frm, int color)
{
	frm->color = color;
}

#endif	/* GL_FRAME_H_INCLUDED */
