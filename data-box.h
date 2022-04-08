#ifndef DATA_BOX_H_INCLUDED
#define DATA_BOX_H_INCLUDED

#include "frame.h"

typedef enum box_split_t {
	SPLIT_NONE,
	SPLIT_VERTICAL,
	SPLIT_HORIZONTAL,
} box_split_t;

struct data_box {
	struct gl_frame frame;
	box_split_t split;
	float split_ratio;
	char caption[16];
	char text[10];
	int text_color;
};

#define DATA_BOX(frame)    ((struct data_box *)frame)

extern int data_box_create(struct gl_frame **out,
			   int x, int y, int w, int h, int color);

extern void data_box_set_text(struct data_box *dbp,
			      const char *text, int size);

extern void data_box_set_caption(struct data_box *dbp,
				 const char *caption);

static inline void data_box_set_text_color(struct data_box *dbp, int color)
{
	dbp->text_color = color;
}

static inline void data_box_set_split(struct data_box *dbp,
				      box_split_t type, float ratio)
{
	dbp->split = type;
	dbp->split_ratio = ratio;
}

#endif	/* DATA_BOX_H_INCLUDED */
