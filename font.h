#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

typedef enum font_t {
	FONT_NONE,
	FONT_MINI4x6,
	FONT_6x10,
	FONT_VGA6x11,
	FONT_7x14,
	FONT_VGA8x8,
	FONT_PEARL8x8,
	FONT_ACORN8x8,
	FONT_VGA8x16,
	FONT_SUN8x16,
	FONT_10x18,
	FONT_SUN12x22,
	FONT_ACORN16x16,
} font_t;

struct font {
	font_t type;
	int width;
	int height;
	void *data;
};

extern const struct font *get_font(font_t type);

static inline int font_width(const struct font *f)
{
	return f->width;
}

static inline int font_height(const struct font *f)
{
	return f->height;
}

static inline void *font_data(const struct font *f)
{
	return f->data;
}

#endif /* FONT_H INCLUDED */
