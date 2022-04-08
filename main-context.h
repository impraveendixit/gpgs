#ifndef MAIN_CONTEXT_H_INCLUDED
#define MAIN_CONTEXT_H_INCLUDED

#include "internals.h"

struct graphics_context;
struct course;
struct flight_data;
struct gps_data;
struct mag_data;

struct gl_frame;

extern int file_chooser_create(struct gl_frame **out,
			       int x, int y, int w, int h, int color);

extern int file_chooser_for_each(struct gl_frame *frm, unsigned int next);

extern int file_chooser_get_file(struct gl_frame *frm, char *buff, int size);

extern int gps_context_create(struct gl_frame **out,
			      int x, int y, int w, int h, int color);

extern void gps_context_add_entry(struct gl_frame *frm,
				  const char *txt, int color);

extern void gps_context_change_header(struct gl_frame *frm,
				      const char *header, int color);

extern int graphics_context_init(struct graphics_context **out,
				 const struct course *cp,
				 const struct flight_data *flt,
				 const struct gps_data *gps,
				 const struct mag_data *mag);

extern void graphics_update(struct graphics_context *gc, rc_t rc);

extern void graphics_context_destroy(struct graphics_context *gc);

extern rc_t graphics_controls(struct graphics_context *gc,
			      struct course *cp,
			      struct flight_data *flt,
			      int key);

#endif	/* MAIN_CONTEXT_H_INCLUDED */
