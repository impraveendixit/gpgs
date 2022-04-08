#ifndef ICON_PRIVATE_H_INCLUDED
#define ICON_PRIVATE_H_INCLUDED

#include "icon.h"

#define ICON_WIDTH	20
#define ICON_HEIGHT	20

#define ICON_SIZE	(ICON_WIDTH * ICON_HEIGHT)

struct icon {
	icon_t	name;
	int	width;
	int	height;
	void *	data;
};

extern struct icon	icon_aircraft,
			icon_helifront,
			icon_helitop,
			icon_flag,
			icon_powerline,
			icon_barrel,
			icon_camp,
			icon_home;

#endif	/* ICON_PRIVATE_H_INCLUDED */
