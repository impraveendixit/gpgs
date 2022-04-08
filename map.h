#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

struct gl_frame;
struct map_context;
struct flight_data;
struct course;

typedef enum map_scale_t {
	MAP_SCALE_UP,
	MAP_SCALE_DN,
	MAP_SCALE_AUTO,
} map_scale_t;

extern int map_context_create(struct gl_frame **out, int x, int y,
			      int w, int h, int color, const struct course *cp,
			      const struct flight_data *flt);

extern void map_context_adjust_scale(struct gl_frame *frm, map_scale_t scale);

extern void icons_create(void);

#endif	/* MAP_H_INCLUDED */
