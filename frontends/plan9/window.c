#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <draw.h>
#include <event.h>
#include <cursor.h>
#include "utils/errors.h"
#include "utils/nsurl.h"
#include "netsurf/window.h"
#include "netsurf/inttypes.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/mouse.h"
#include "netsurf/window.h"
#include "netsurf/misc.h"
#include "netsurf/form.h"
#include "netsurf/keypress.h"
#include "plan9/window.h"
#include "plan9/gui.h"
#include "plan9/plotter.h"
#include "plan9/utils.h"
#include "plan9/drawui/window.h"
#include "plan9/drawui/data.h"

/**
 * Create and open a gui window for a browsing context.
 *
 * The implementing front end must create a context suitable
 *  for it to display a window referred to as the "gui window".
 *
 * The frontend will be expected to request the core redraw
 *  areas of the gui window which have become invalidated
 *  either from toolkit expose events or as a result of a
 *  invalidate() call.
 *
 * Most core operations used by the frontend concerning browser
 *  windows require passing the browser window context therefor
 *  the gui window must include a reference to the browser
 *  window passed here.
 *
 * If GW_CREATE_CLONE flag is set existing is non-NULL.
 *
 * \param bw The core browsing context associated with the gui window
 * \param existing An existing gui_window, may be NULL.
 * \param flags flags to control the gui window creation.
 * \return gui window, or NULL on error.
 */
struct gui_window *
window_create(struct browser_window *bw,
			  struct gui_window *existing,
			  gui_window_create_flags flags)
{
	return gui_window_create(bw);
}


/**
 * Destroy previously created gui window
 *
 * \param gw The gui window to destroy.
 */
void
window_destroy(struct gui_window *gw)
{
	gui_window_destroy(gw);
}


/**
 * Invalidate an area of a window.
 *
 * The specified area of the window should now be considered
 *  out of date. If the area is NULL the entire window must be
 *  invalidated. It is expected that the windowing system will
 *  then subsequently cause redraw/expose operations as
 *  necessary.
 *
 * \note the frontend should not attempt to actually start the
 *  redraw operations as a result of this callback because the
 *  core redraw functions may already be threaded.
 *
 * \param gw The gui window to invalidate.
 * \param rect area to redraw or NULL for the entire window area
 * \return NSERROR_OK on success or appropriate error code
 */
nserror
window_invalidate(struct gui_window *gw, const struct rect *rect)
{
	Rectangle clipr;
	int sy;

	if (rect == NULL) {
		clipr = dwindow_get_view_rect(gw->dw);
	} else {
		sy = dwindow_get_scroll_y(gw->dw);
		clipr = Rect(rect->x0, rect->y0 - sy, rect->x1, rect->y1 - sy);
		clipr = dwindow_rect_in_view_rect(gw->dw, clipr);
	}
//DBG("window_invalidate (rect:[%d %d %d %d])", clipr.min.x, clipr.min.y, clipr.max.x, clipr.max.y);
	gui_window_redraw(gw, clipr);
	return NSERROR_OK;
}


/**
 * Get the scroll position of a browser window.
 *
 * \param gw The gui window to obtain the scroll position from.
 * \param sx receives x ordinate of point at top-left of window
 * \param sy receives y ordinate of point at top-left of window
 * \return true iff successful
 */
bool
window_get_scroll(struct gui_window *gw, int *sx, int *sy)
{
	*sx = dwindow_get_scroll_x(gw->dw);
	*sy = dwindow_get_scroll_y(gw->dw);
	return 1;
}


/**
 * Set the scroll position of a browser window.
 *
 * scrolls the viewport to ensure the specified rectangle of
 *   the content is shown.
 * If the rectangle is of zero size i.e. x0 == x1 and y0 == y1
 *   the contents will be scrolled so the specified point in the
 *   content is at the top of the viewport.
 * If the size of the rectangle is non zero the frontend may
 *   add padding or centre the defined area or it may simply
 *   align as in the zero size rectangle
 *
 * \param gw The gui window to scroll.
 * \param rect The rectangle to ensure is shown.
 * \return NSERROR_OK on success or appropriate error code.
 */
nserror
window_set_scroll(struct gui_window *gw, const struct rect *rect)
{
	if(rect != NULL && rect->x0 == rect->x1 && rect->y0 == rect->y1) {
		dwindow_set_scroll(gw->dw, rect->x0, rect->y0);
	}
	return NSERROR_OK;
}

/**
 * Find the current dimensions of a browser window's content area.
 *
 * This is used to determine the actual available drawing size
 * in pixels. This allows contents that can be dynamically
 * reformatted, such as HTML, to better use the available
 * space.
 *
 * \param gw The gui window to measure content area of.
 * \param width receives width of window
 * \param height receives height of window
 * \return NSERROR_OK on success and width and height updated
 *          else error code.
 */
nserror
window_get_dimensions(struct gui_window *gw, int *width, int *height)
{
	Rectangle r;

	r = dwindow_get_view_rect(gw->dw);
	*width = Dx(r);
	*height = Dy(r);
	return NSERROR_OK;
}


/**
 * Miscellaneous event occurred for a window
 *
 * This is used to inform the frontend of window events which
 *   require no additional parameters.
 *
 * \param gw The gui window the event occurred for
 * \param event Which event has occurred.
 * \return NSERROR_OK if the event was processed else error code.
 */
nserror
window_event(struct gui_window *gw, enum gui_window_event event)
{
	Rectangle r;
	int w, h;

	//DBG("IN window_event - event=%d", event);
	switch (event) {
	case GW_EVENT_UPDATE_EXTENT:
		if (browser_window_get_extents(gw->bw, true, &w, &h) == NSERROR_OK) {
			dwindow_set_extents(gw->dw, w, h);
		}
		break;

	case GW_EVENT_REMOVE_CARET:
		if (gw->caret_height != -1) {
			gw->caret = ZP;
			gw->caret_height = -1;
		}
		break;

	case GW_EVENT_START_SELECTION:

		break;

	case GW_EVENT_START_THROBBER:

		break;

	case GW_EVENT_STOP_THROBBER:

		break;

	case GW_EVENT_PAGE_INFO_CHANGE:

		break;

	default:
		break;
	}
	return NSERROR_OK;
}

/* Optional entries */

/**
 * Set the title of a window.
 *
 * \param gw The gui window to set title of.
 * \param title new window title
 */
void
window_set_title(struct gui_window *gw, const char *title)
{
	dwindow_set_title(gw->dw, title);
}

/**
 * Set the navigation url.
 *
 * \param gw window to update.
 * \param url The url to use as icon.
 */
nserror
window_set_url(struct gui_window *gw, struct nsurl *url)
{
	dwindow_set_url(gw->dw, nsurl_access(url));
	return NSERROR_OK;
}

/**
 * Set a favicon for a gui window.
 *
 * \param gw window to update.
 * \param icon handle to object to use as icon.
 */
void
window_set_icon(struct gui_window *gw, struct hlcache_handle *icon)
{
	Image *i;
	struct bitmap *b;

	i = NULL;
	if (content_get_type(icon) == CONTENT_IMAGE) {
		b = content_get_bitmap(icon);
		i = getimage(b);
	}
	dwindow_set_icon(gw->dw, i);
}

/**
 * Set the status bar message of a browser window.
 *
 * \param gw gui_window to update
 * \param text new status text
 */
void
window_set_status(struct gui_window *gw, const char *text)
{
	dwindow_set_status(gw->dw, text);
}

/**
 * Change mouse pointer shape
 *
 * \param g The gui window to change pointer shape in.
 * \param shape The new shape to change to.
 */
void
window_set_pointer(struct gui_window *g, enum gui_pointer_shape shape)
{
	Cursor *c;

	c = NULL;
	switch (shape) {
	case GUI_POINTER_POINT:
		c = &linkcursor;
		break;
	case GUI_POINTER_CARET:
		c = &caretcursor;
		break;
	case GUI_POINTER_WAIT:
	case GUI_POINTER_PROGRESS:
	case GUI_POINTER_MENU:
		c = &waitcursor;
		break;
	case GUI_POINTER_CROSS:
		c = &crosscursor;
		break;
	case GUI_POINTER_HELP:
		c = &helpcursor;
		break;
	case GUI_POINTER_LU:
		c = cornercursors[0];
		break;
	case GUI_POINTER_UP:
		c = cornercursors[1];
		break;
	case GUI_POINTER_RU:
		c = cornercursors[2];
		break;
	case GUI_POINTER_LEFT:
		c = cornercursors[3];
		break;
	case GUI_POINTER_RIGHT:
		c = cornercursors[5];
		break;
	case GUI_POINTER_LD:
		c = cornercursors[6];
		break;
	case GUI_POINTER_DOWN:
		c = cornercursors[7];
		break;
	case GUI_POINTER_RD:
		c = cornercursors[8];
		break;
	/* not handled */
	case GUI_POINTER_MOVE:
	case GUI_POINTER_NO_DROP:
	case GUI_POINTER_NOT_ALLOWED:
	case GUI_POINTER_DEFAULT:
		break;	
	}
	esetcursor(c);
}

/**
 * Place the caret in a browser window.
 *
 * \param  g	   window with caret
 * \param  x	   coordinates of caret
 * \param  y	   coordinates of caret
 * \param  height  height of caret
 * \param  clip	   clip rectangle, or NULL if none
 */
void
window_place_caret(struct gui_window *g, int x, int y, int height, const struct rect *clip)
{
	Rectangle r;
	Point p0, p1;
	int sx, sy;

	sx = dwindow_get_scroll_x(g->dw);
	sy = dwindow_get_scroll_y(g->dw);
	p0 = dwindow_point_in_view_rect(g->dw, Pt(x - sx, y - sy));
	p1 = dwindow_point_in_view_rect(g->dw, Pt(x - sx, y - sy + height));
	if (clip != NULL) {
		r = Rect(clip->x0, clip->x1, clip->y0, clip->y1);
	} else {
		r = Rect(x - 1, y - 1, x + 1, y + height + 1);
	}
	r = rectaddpt(r, Pt(-sx, -sy));
	r = dwindow_rect_in_view_rect(g->dw, r);
	replclipr(screen, 0, screen->r);
	line(screen, p0, p1, 1, 1, 0, display->black, ZP);
	g->caret = addpt(p0, Pt(sx, sy));
	g->caret_height = height;
}

/**
 * start a drag operation within a window
 *
 * \param g window to start drag from.
 * \param type Type of drag to start
 * \param rect Confining rectangle of drag operation.
 * \return true if drag started else false.
 */
bool
window_drag_start(struct gui_window *g, gui_drag_type type, const struct rect *rect)
{
	DBG("IN window_drag_start");
	return 0;
}

/**
 * save link operation
 *
 * \param g window to save link from.
 * \param url The link url.
 * \param title The title of the link.
 * \return NSERROR_OK on success else appropriate error code.
 */
nserror
window_save_link(struct gui_window *g, struct nsurl *url, const char *title)
{
	DBG("IN window_save_link");
	return NSERROR_OK;
}

/**
 * create a form select menu
 *
 * \param gw The gui window to open select menu form gadget in.
 * \param control The form control gadget handle.
 */
void
window_create_form_select_menu(struct gui_window *gw, struct form_control *control)
{
	DBG("IN window_create_form_select_menu");
}

/**
 * Called when file chooser gadget is activated
 *
 * \param gw The gui window to open file chooser in.
 * \param hl The content of the object.
 * \param gadget The form control gadget handle.
 */
void
window_file_gadget_open(struct gui_window *gw, struct hlcache_handle *hl, struct form_control *gadget)
{
	DBG("IN window_file_gadget_open");
}

/**
 * object dragged to window
 *
 * \param gw The gui window to save dragged object of.
 * \param c The content of the object.
 * \param type the type of save.
 */
void
window_drag_save_object(struct gui_window *gw, struct hlcache_handle *c, gui_save_type type)
{
	DBG("IN window_drag_save_object");
}

/**
 * drag selection save
 *
 * \param gw The gui window to save dragged selection of.
 * \param selection The selection to save.
 */
void
window_drag_save_selection(struct gui_window *gw, const char *selection)
{
	DBG("IN window_drag_save_selection");
}

/**
 * console logging happening.
 *
 * See \ref browser_window_console_log
 *
 * \param gw The gui window receiving the logging.
 * \param src The source of the logging message
 * \param msg The text of the logging message
 * \param msglen The length of the text of the logging message
 * \param flags Flags associated with the logging.
 */
void 
window_console_log(struct gui_window *gw,
		    	   browser_window_console_source src,
		    	   const char *msg,
		    	   size_t msglen,
		    	   browser_window_console_flags flags)
{
	DBG("IN window_console_log");
}

static struct gui_window_table window_table = {
	.create = window_create,
	.destroy = window_destroy,
	.invalidate = window_invalidate,
	.get_scroll = window_get_scroll,
	.set_scroll = window_set_scroll,
	.get_dimensions = window_get_dimensions,
	.event = window_event,

	.set_icon = window_set_icon,
	.set_title = window_set_title,
	.set_status = window_set_status,
	.set_pointer = window_set_pointer,
	.place_caret = window_place_caret,
	.create_form_select_menu = window_create_form_select_menu,
	.file_gadget_open = window_file_gadget_open,
	.set_url = window_set_url,


};

struct gui_window_table *plan9_window_table = &window_table;