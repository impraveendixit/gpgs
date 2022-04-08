#ifndef SVGALIB_PRIVATE_H_INCLUDED
#define SVGALIB_PRIVATE_H_INCLUDED

#ifndef VGAMODE
#define VGAMODE		18
#endif
#ifndef MAXX_PIXELS
#define MAXX_PIXELS	640
#endif
#ifndef MAXY_PIXELS
#define MAXY_PIXELS	480
#endif

#define CTX_HEIGHT	(MAXY_PIXELS)
#define CTX_WIDTH	(MAXX_PIXELS)

#ifdef CONFIG_SVGALIB_GRAPHICS_MODE
#include "svgalib-graphics.h"
#else
#include "svgalib-text.h"
#endif

#endif  /* SVGALIB_PRIVATE_H_INCLUDED */
