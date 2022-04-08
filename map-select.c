#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "main-context.h"
#include "svgalib-private.h"
#include "frame.h"
#include "config.h"
#include "debug.h"
#include "list.h"

#define MAP_FILE_EXTENSION \
	stringify(.pgn)

#define EXIT_STRING \
	stringify(- EXIT -)

#define	FOOTER_TEXT \
	stringify(Navigate using <Arrow> key and press <Enter> to choose.)

struct file_entry {
	char *name;
	struct list_head list;
};

struct file_chooser {
	struct gl_frame frame;
	struct list_head file_list;
	const struct list_head *curr_entry;
};

static int __add_entry(struct file_chooser *fch, const char *name)
{
	struct file_entry *entry = NULL;

	if (name == NULL)
		goto exit;

	entry = malloc(sizeof(struct file_entry));
	if (entry == NULL) {
		SYSERR("Failed to allocate for map file entry");
		goto exit;
	}

	if ((entry->name = strdup(name)) == NULL) {
		SYSERR("Failed to duplicate string.");
		goto exit_free;
	}
	INIT_LIST_HEAD(&entry->list);

	list_add(&entry->list, &fch->file_list);
	fch->curr_entry = &entry->list;
	return 0;

 exit_free:
	free(entry);
 exit:
	return -1;
}

static int check_file_extension(const char *filename, const char *ext)
{
	const char *chp = NULL;

	if (filename == NULL)
		return -1;

	for (chp = filename; *chp != '\0' && *chp != '.'; ++chp)
	;

	/* Ignoring case of file names */
	if (strcasecmp(chp, ext) != 0) {
		DEBUG("File extension doesnot match");
		return -1;
	}
	return 0;
}

static int fetch_directory_entry(const char *dirpath, struct file_chooser *fch)
{
	struct dirent *ep = NULL;
	DIR *dirp = NULL;

	if (dirpath == NULL)
		return -1;

	dirp = opendir(dirpath);
	if (dirp == NULL) {
		SYSERR("Failed to open directory '%s'", dirpath);
		return -1;
	}

	while (((ep = readdir(dirp)) != NULL)) {
		if (!check_file_extension(ep->d_name, MAP_FILE_EXTENSION))
			__add_entry(fch, ep->d_name);
	}
	closedir(dirp);
	return 0;
}

static void file_chooser_destroy(struct gl_frame *frm)
{
	struct file_chooser *fch = (struct file_chooser *)frm;

	if (fch == NULL)
		return;

	if (!list_empty(&fch->file_list)) {
		struct list_head *pos, *temp;
		list_for_each_safe(pos, temp, &fch->file_list) {
			struct file_entry *entry = NULL;
			if (!(entry = list_entry(pos, struct file_entry, list)))
				continue;
			list_del(&entry->list);
			free(entry->name);
			free(entry);
			entry = NULL;
		}
	} else {
		DEBUG("List is empty already.!!");
	}
	free(fch);
	fch = NULL;
}

static void file_chooser_draw(struct gl_frame *frm)
{
	struct file_chooser *fch = (struct file_chooser *)frm;
	struct file_entry *entry = NULL;
	int i = 0, nr_display = (frm->height - 80) >> 5;
	int bg_color = svgalib_get_color(20, 20, 20);
	int txt_color = svgalib_get_color(1, 1, 10);
	char buff[64] = "";
	int yb, ye, ym;
	int xb, xe, xm;

	xb = frm->xb + 100;
	xe = frm->xb + frm->width - 100;
	xm = (xb + xe) >> 1;

	/* Header */
	svgalib_draw_frame(frm->xb, frm->yb, frm->width, 30, bg_color);
	snprintf(buff, 64, "GPS-aided Pilot Guidance System [GPGS %s]",
		 GPGS_VERSION);
	svgalib_display_text_wrapped(frm->xb + (frm->width >> 1), frm->yb + 15,
				     buff, FONT_SUN8x16, bg_color, txt_color);

	INFO("%s", buff);

	/* Footer */
	svgalib_display_text_wrapped(frm->xb + (frm->width >> 1),
				     (frm->yb + frm->height) - 30,
				     FOOTER_TEXT,
				     FONT_ACORN8x8, frm->color, bg_color);

	if (list_empty(&fch->file_list)) {
		DEBUG("list is empty.");
		return;
	}

	list_for_each_entry(entry, &fch->file_list, list) {
		if (i < nr_display) {
			yb = frm->yb + 40 + (i++ << 5);
			ye = yb + 32;
			ym = (yb + ye) >> 1;

			/* current selection in green background and bigger font. */
			if (fch->curr_entry == &entry->list) {
				bg_color = svgalib_get_color(0, 10, 0);
				svgalib_draw_frame(xb, yb, xe - xb,
						   ye - yb, bg_color);
				svgalib_display_text_wrapped(xm, ym,
					entry->name, FONT_SUN8x16, bg_color,
					svgalib_get_color(31, 31, 31));

				INFO("--->%d: %s", i, entry->name);
			} else {
				bg_color = svgalib_get_color(20, 20, 20);
				svgalib_draw_frame(xb, yb, xe - xb,
						   ye - yb, bg_color);
				svgalib_display_text_wrapped(xm, ym,
					    entry->name, FONT_ACORN8x8,
					    bg_color, txt_color);

				INFO("%d: %s", i, entry->name);
			}
		} else {
			/* If more files than which can be fit on screen, then at
			* least current selection shown at the bottom which can
			* simulate graphics spinner type of action.
			*/
			if (fch->curr_entry == &entry->list) {
				yb = frm->yb + 40 + ((nr_display - 1) << 5);
				ye = yb + 32;
				ym = (yb + ye) >> 1;
				bg_color = svgalib_get_color(0, 10, 0);
				svgalib_draw_frame(xb, yb, xe - xb,
						   ye - yb, bg_color);
				svgalib_display_text_wrapped(xm, ym,
					entry->name, FONT_SUN8x16, bg_color,
					svgalib_get_color(31, 31, 31));

				INFO("--->%d: %s", i, entry->name);
			}
		}
	}

	INFO("%s\n", FOOTER_TEXT);
}

int file_chooser_create(struct gl_frame **out,
			int x, int y, int w, int h, int color)
{
	struct file_chooser *fch = NULL;

	if (out == NULL)
		goto exit;

	if ((fch = malloc(sizeof(struct file_chooser))) == NULL) {
		SYSERR("Out of memory.");
		goto exit;
	}

	gl_frame_init(&fch->frame, x, y, w, h, color);
	fch->frame.draw = file_chooser_draw;
	fch->frame.destroy = file_chooser_destroy;

	INIT_LIST_HEAD(&fch->file_list);
	fch->curr_entry = NULL;

	/* Add 'EXIT' action at bottom */
	if (__add_entry(fch, EXIT_STRING) != 0) {
		DEBUG("Failed to add '%s' entry in file chooser", EXIT_STRING);
		goto exit_free;
	}

	if (fetch_directory_entry(map_directory, fch) != 0)
		DEBUG("Failed to fetch directory '%s' entry", map_directory);

	*out = GL_FRAME(fch);
	return 0;

 exit_free:
	free(fch);
 exit:
	return -1;
}

int file_chooser_for_each(struct gl_frame *frm, unsigned int next)
{
    struct file_chooser *fch = (struct file_chooser *)frm;

	if (fch == NULL)
		return -1;

	if (next) {
		if (fch->curr_entry->next != &fch->file_list) {
			fch->curr_entry = fch->curr_entry->next;
			return 0;
		}
	} else {
		if (fch->curr_entry->prev != &fch->file_list) {
			fch->curr_entry = fch->curr_entry->prev;
			return 0;
		}
	}
	return -1;
}

int file_chooser_get_file(struct gl_frame *frm, char *buff, int size)
{
	struct file_chooser *fch = (struct file_chooser *)frm;

	if (fch == NULL)
		return -1;

	if (!list_is_last(fch->curr_entry, &fch->file_list)) {
		struct file_entry *entry = list_entry(fch->curr_entry,
						      struct file_entry, list);
		snprintf(buff, size, "%s%s", map_directory, entry->name);
		INFO("Chosen file=%s", buff);
		return 0;
	}
	return -1;
}
