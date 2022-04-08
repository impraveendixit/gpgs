#include <stdlib.h>

#include "icon-private.h"
#include "../svgalib-private.h"
#include "internals.h"
#include "geometry.h"

static const struct icon *icon_lookup(icon_t name)
{
	switch (name) {
	case ICON_HELITOP:
		return &icon_helitop;
	case ICON_HELIFRONT:
		return &icon_helifront;
	case ICON_BARREL:
		return &icon_barrel;
	case ICON_CAMP:
		return &icon_camp;
	case ICON_HOME:
		return &icon_home;
	case ICON_AIRCRAFT:
		return &icon_aircraft;
	case ICON_FLAG:
		return &icon_flag;
	case ICON_POWERLINE:
		return &icon_powerline;
	}
	return NULL;
}

void icons_initialize(void)
{
	register int i, k;
	int color_black = svgalib_get_color(0, 0, 0);
	int color_brown = svgalib_get_color(15, 8, 0);
	int color_red = svgalib_get_color(15, 0, 0);
	int color_yellow = svgalib_get_color(31, 31, 0);
	int color_blue = svgalib_get_color(0, 0, 31);
	int color_green = svgalib_get_color(0, 0, 0);
	int color_light_green = svgalib_get_color(0, 31, 0);
	int color_light_red = svgalib_get_color(31, 0, 0);
	int color_white = svgalib_get_color(31, 31, 31);
	int color_cyan = svgalib_get_color(10, 21, 21);
	int *icon_data[] = {
		icon_helitop.data,
		icon_helifront.data,
		icon_barrel.data,
		icon_camp.data,
		icon_home.data,
		icon_aircraft.data,
		icon_flag.data,
		icon_powerline.data
	};

	for (i = 0; i < ARRAY_SIZE(icon_data); i++) {
		for (k = 0; k < ICON_SIZE; k++) {
			switch (icon_data[i][k]) {
			default:
			case 0:
				icon_data[i][k] = color_black;
				break;
			case 1:
				icon_data[i][k] = color_brown;
				break;
			case 2:
				icon_data[i][k] = color_red;
				break;
			case 3:
				icon_data[i][k] = color_yellow;
				break;
			case 4:
				icon_data[i][k] = color_blue;
				break;
			case 5:
				icon_data[i][k] = color_green;
				break;
			case 6:
				icon_data[i][k] = color_light_green;
				break;
			case 7:
				icon_data[i][k] = color_light_red;
				break;
			case 8:
				icon_data[i][k] = color_white;
				break;
			case 9:
				icon_data[i][k] = color_cyan;
				break;
			}
		}
	}
}

void icon_plot(const struct boundary *b, const struct point *pos, icon_t name)
{
	register int i, k;
	struct point p;
	int color;
	const struct icon *icp = NULL;
	const int *data;

	icp = icon_lookup(name);
	if (icp == NULL)
		return;

	data = (const int *)icp->data;

	for (i = 0; i < ICON_WIDTH; i++) {
		p.x = i + pos->x - 10;

		for (k = 0; k < ICON_HEIGHT; k++) {
			p.y = k + pos->y - 10;
			color = data[i + ICON_HEIGHT * k];
			if (!is_point_inside_boundary(b, &p) || !color)
				continue;
			svgalib_set_pixel(p.x, p.y, color);
		}
	}
}
