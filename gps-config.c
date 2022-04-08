#include <stdlib.h>

#include "main-context.h"
#include "debug.h"
#include "frame.h"
#include "nmea.h"
#include "svgalib-private.h"

#define FOOTER_TEXT	stringify(Press <Enter> to Proceed...)

struct gps_context_entry {
	char txt[NMEA_STRING_LEN + 1];
	int color;
};

struct gps_context {
	struct gl_frame frame;
	struct gps_context_entry **entries;
	const char *header;
	int hdr_color;
	int nr_entry;
	int curr_index;
};

static void gps_context_draw(struct gl_frame *frm)
{
	struct gps_context *ctx = (struct gps_context *)frm;
	register int i;
	int x = frm->xb + (frm->width >> 1);

	/* Header */
	svgalib_display_text_wrapped(x, frm->yb + 10, ctx->header,
				     FONT_SUN8x16, frm->color, ctx->hdr_color);

	INFO("%s", ctx->header);

	/* Footer */
	svgalib_display_text_wrapped(x, frm->yb + frm->height - 20,
				     FOOTER_TEXT,
				     FONT_SUN8x16, frm->color,
				     svgalib_get_color(20, 20, 20));

	/* Since index is incremented after add up and normally
	 * display is called after each entry, so equality
	 * condition is prohibited here..!!
	 */
	for (i = 0; i < ctx->curr_index; ++i) {
		struct gps_context_entry *entry = ctx->entries[i];
		svgalib_display_text(frm->xb + 8, frm->yb + (i << 4) + 40,
				     entry->txt, FONT_ACORN8x8,
				     frm->color, entry->color);
		INFO("%s", entry->txt);
	}

	INFO("%s\n", FOOTER_TEXT);
}

static void gps_context_destroy(struct gl_frame *frm)
{
	struct gps_context *ctx = (struct gps_context *)frm;
	register int i;

	if (ctx == NULL)
		return;

	for (i = 0; i < ctx->nr_entry; i++) {
		struct gps_context_entry *entry = ctx->entries[i];
		if (entry)
			free(entry);
	}
	free(ctx->entries);
	free(ctx);
	ctx = NULL;
}

int gps_context_create(struct gl_frame **out,
		       int x, int y, int w, int h, int color)
{
	register int i;
	struct gps_context *ctx = NULL;

	if (out == NULL)
		goto exit;

	ctx = calloc(1, sizeof(struct gps_context));
	if (ctx == NULL) {
		SYSERR("Out of memory.");
		goto exit;
	}

	/* Initialize frame */
	gl_frame_init(&ctx->frame, x, y, w, h, color);
	ctx->frame.draw = gps_context_draw;
	ctx->frame.destroy = gps_context_destroy;

	ctx->curr_index = 0;
	ctx->header = "Configuring GPS...";
	ctx->hdr_color = svgalib_get_color(15, 15, 0);
	ctx->nr_entry = ((h - 80) >> 4);
	ctx->entries = calloc(ctx->nr_entry,
			      sizeof(struct gps_context_entry *));
	if (ctx->entries == NULL) {
		SYSERR("Failed to allocate for gps context entry array");
		goto exit_free;
	}

	for (i = 0; i < ctx->nr_entry; i++) {
		ctx->entries[i] = calloc(1, sizeof(struct gps_context_entry));
		if (ctx->entries[i] == NULL) {
			SYSERR("Failed to allocate for gps entry");
			goto exit_free_entry;
		}
		ctx->entries[i]->color = svgalib_get_color(0, 0, 0);
	}

	*out = GL_FRAME(ctx);
	return 0;

 exit_free_entry:
	for (i = 0; i < ctx->nr_entry; i++) {
		struct gps_context_entry *entry = ctx->entries[i];
		if (entry)
			free(entry);
	}
 exit_free:
	free(ctx);
 exit:
	return -1;
}

void gps_context_change_header(struct gl_frame *frm,
			       const char *header, int color)
{
	struct gps_context *ctx = (struct gps_context *)frm;
	ctx->header = header;
	ctx->hdr_color = color;
}

void gps_context_add_entry(struct gl_frame *frm, const char *txt, int color)
{
	struct gps_context *ctx = (struct gps_context *)frm;
	int index = ctx->curr_index;
	register int i;
	int nr_chars = (frm->width - 8) >> 3;

	/* append new text at the bottom */
	if (index >= ctx->nr_entry) {
		index = ctx->nr_entry - 1;
		for (i = 1; i <= index; i++)
			memmove(ctx->entries[i - 1], ctx->entries[i],
				sizeof(struct gps_context_entry));
	}

	strncpy(ctx->entries[index]->txt, txt, NMEA_STRING_LEN);
	ctx->entries[index]->color = color;

	if (nr_chars > NMEA_STRING_LEN)
		nr_chars = NMEA_STRING_LEN - 1;

	/* Truncate string otherwise disvplay goes out of screen. */
	ctx->entries[index]->txt[nr_chars] = '\0';
	ctx->entries[index]->txt[nr_chars - 1] = '~';
	ctx->curr_index = ++index;
}
