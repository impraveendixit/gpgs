#ifndef ICON_H_INCLUDED
#define ICON_H_INCLUDED

typedef enum icon_t {
	ICON_AIRCRAFT,
	ICON_HELIFRONT,
	ICON_HELITOP,
	ICON_FLAG,
	ICON_POWERLINE,
	ICON_BARREL,
	ICON_CAMP,
	ICON_HOME,
} icon_t;

struct point;
struct boundary;

void icons_initialize(void);

void icon_plot(const struct boundary *b, const struct point *pos, icon_t name);

#endif	/* ICON_H_INCLUDED */
