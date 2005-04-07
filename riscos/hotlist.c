/*
 * This file is part of NetSurf, http://netsurf.sourceforge.net/
 * Licensed under the GNU General Public License,
 *		  http://www.opensource.org/licenses/gpl-license
 * Copyright 2004, 2005 Richard Wilson <info@tinct.net>
 */

/** \file
 * Hotlist (implementation).
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <swis.h>
#include "libxml/HTMLparser.h"
#include "libxml/HTMLtree.h"
#include "oslib/colourtrans.h"
#include "oslib/dragasprite.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"
#include "netsurf/content/content.h"
#include "netsurf/desktop/tree.h"
#include "netsurf/riscos/gui.h"
#include "netsurf/riscos/menus.h"
#include "netsurf/riscos/theme.h"
#include "netsurf/riscos/tinct.h"
#include "netsurf/riscos/treeview.h"
#include "netsurf/riscos/wimp.h"
#include "netsurf/utils/log.h"
#include "netsurf/utils/messages.h"
#include "netsurf/utils/utils.h"
#include "netsurf/utils/url.h"


static void ro_gui_hotlist_visited(struct content *content, struct tree *tree,
		struct node *node);

/*	A basic window for the hotlist
*/
static wimp_window hotlist_window_definition = {
	{0, 0, 600, 800},
	0,
	0,
	wimp_TOP,
	wimp_WINDOW_NEW_FORMAT | wimp_WINDOW_MOVEABLE | wimp_WINDOW_BACK_ICON |
			wimp_WINDOW_CLOSE_ICON | wimp_WINDOW_TITLE_ICON |
			wimp_WINDOW_TOGGLE_ICON | wimp_WINDOW_SIZE_ICON |
			wimp_WINDOW_VSCROLL | wimp_WINDOW_IGNORE_XEXTENT |
			wimp_WINDOW_IGNORE_YEXTENT,
	wimp_COLOUR_BLACK,
	wimp_COLOUR_LIGHT_GREY,
	wimp_COLOUR_LIGHT_GREY,
	wimp_COLOUR_WHITE,
	wimp_COLOUR_DARK_GREY,
	wimp_COLOUR_MID_LIGHT_GREY,
	wimp_COLOUR_CREAM,
	0,
	{0, -16384, 16384, 0},
	wimp_ICON_TEXT | wimp_ICON_INDIRECTED | wimp_ICON_HCENTRED |
			wimp_ICON_VCENTRED,
	wimp_BUTTON_DOUBLE_CLICK_DRAG << wimp_ICON_BUTTON_TYPE_SHIFT,
	wimpspriteop_AREA,
	1,
	1,
	{""},
	0,
	{}
};


/*	The hotlist window, toolbar and plot origins
*/
static wimp_w hotlist_window;
struct tree *hotlist_tree;

/*	Whether the editing facilities are for add so that we know how
	to reset the dialog boxes on a adjust-cancel and the action to
	perform on ok.
*/
struct node *dialog_folder_node;
struct node *dialog_entry_node;

void ro_gui_hotlist_initialise(void) {
	FILE *fp;
	const char *title;
	os_error *error;
	struct node *node;

	/*	Create our window
	*/
	title = messages_get("Hotlist");
	hotlist_window_definition.title_data.indirected_text.text =
			strdup(title);
	hotlist_window_definition.title_data.indirected_text.validation =
			(char *) -1;
	hotlist_window_definition.title_data.indirected_text.size =
			strlen(title);
	error = xwimp_create_window(&hotlist_window_definition,
			&hotlist_window);
	if (error) {
		LOG(("xwimp_create_window: 0x%x: %s",
				error->errnum, error->errmess));
		die(error->errmess);
	}

	/*	Either load or create a hotlist
	*/
	fp = fopen("Choices:WWW.NetSurf.Hotlist", "r");
	if (!fp) {
		hotlist_tree = calloc(sizeof(struct tree), 1);
		if (!hotlist_tree) {
			warn_user("NoMemory", 0);
			return;
		}
		hotlist_tree->root = tree_create_folder_node(NULL, "Root");
		if (!hotlist_tree->root) {
			warn_user("NoMemory", 0);
			free(hotlist_tree);
			hotlist_tree = NULL;
		}
		hotlist_tree->root->expanded = true;
		node = tree_create_folder_node(hotlist_tree->root, "NetSurf");
		if (!node)
			node = hotlist_tree->root;
		tree_create_URL_node(node, messages_get("HotlistHomepage"),
				"http://netsurf.sourceforge.net/", 0xfaf,
				time(NULL), -1, 0);
		tree_initialise(hotlist_tree);
	} else {
		fclose(fp);
		hotlist_tree = options_load_tree("Choices:WWW.NetSurf.Hotlist");
	}
	if (!hotlist_tree) return;
	hotlist_tree->handle = (int)hotlist_window;
	hotlist_tree->movable = true;

	/*	Create our toolbar
	*/
	hotlist_tree->toolbar = ro_gui_theme_create_toolbar(NULL,
			THEME_HOTLIST_TOOLBAR);
	if (hotlist_tree->toolbar)
		ro_gui_theme_attach_toolbar(hotlist_tree->toolbar,
				hotlist_window);
}


/**
 * Perform a save to the default file
 */
void ro_gui_hotlist_save(void) {
	os_error *error;

	if (!hotlist_tree) return;

	/*	Save to our file
	*/
	options_save_tree(hotlist_tree, "<Choices$Write>.WWW.NetSurf.Hotlist",
			"NetSurf hotlist");
	error = xosfile_set_type("<Choices$Write>.WWW.NetSurf.Hotlist", 0xfaf);
	if (error)
		LOG(("xosfile_set_type: 0x%x: %s",
				error->errnum, error->errmess));
}


/**
 * Respond to a mouse click
 *
 * \param pointer  the pointer state
 */
void ro_gui_hotlist_click(wimp_pointer *pointer) {
	ro_gui_tree_click(pointer, hotlist_tree);
	if (pointer->buttons == wimp_CLICK_MENU)
		ro_gui_menu_create(hotlist_menu, pointer->pos.x,
				pointer->pos.y, pointer->w);
	else
		ro_gui_menu_prepare_action(pointer->w, TREE_SELECTION, false);
}


/**
 * Informs the hotlist that some content has been visited
 *
 * \param content  the content visited
 */
void hotlist_visited(struct content *content) {
	if ((!content) || (!content->url) || (!hotlist_tree))
		return;
	ro_gui_hotlist_visited(content, hotlist_tree, hotlist_tree->root);
}


/**
 * Informs the hotlist that some content has been visited
 *
 * \param content  the content visited
 * \param tree	   the tree to find the URL data from
 * \param node	   the node to update siblings and children of
 */
void ro_gui_hotlist_visited(struct content *content, struct tree *tree,
		struct node *node) {
	struct node_element *element;

	for (; node; node = node->next) {
		if (!node->folder) {
			element = tree_find_element(node, TREE_ELEMENT_URL);
			if ((element) && (!strcmp(element->text,
					content->url))) {
				element->user_data =
						ro_content_filetype(content);
				element = tree_find_element(node,
						TREE_ELEMENT_VISITS);
				if (element)
					element->user_data += 1;
				element = tree_find_element(node,
						TREE_ELEMENT_LAST_VISIT);
				if (element)
					element->user_data = time(NULL);
				tree_update_URL_node(node);
				tree_handle_node_changed(tree, node, true,
						false);
			}
		}
		if (node->child)
			ro_gui_hotlist_visited(content, tree, node->child);
	}
}


/**
 * Prepares the folder dialog contents for a node
 *
 * \param node	   the node to prepare the dialogue for, or NULL
 */
void ro_gui_hotlist_prepare_folder_dialog(struct node *node) {
	dialog_folder_node = node;
	if (node) {
		ro_gui_set_window_title(dialog_folder,
				messages_get("EditFolder"));
		ro_gui_set_icon_string(dialog_folder, 1, node->data.text);
	} else {
		ro_gui_set_window_title(dialog_folder,
				messages_get("NewFolder"));
		ro_gui_set_icon_string(dialog_folder, 1,
				messages_get("Folder"));
	}
}


/**
 * Prepares the entry dialog contents for a node
 *
 * \param node	   the node to prepare the dialogue for, or NULL
 */
void ro_gui_hotlist_prepare_entry_dialog(struct node *node) {
	struct node_element *element;

	dialog_entry_node = node;
	if (node) {
		ro_gui_set_window_title(dialog_entry, messages_get("EditLink"));
		ro_gui_set_icon_string(dialog_entry, 1, node->data.text);
		element = tree_find_element(node, TREE_ELEMENT_URL);
		if (element)
			ro_gui_set_icon_string(dialog_entry, 3, element->text);
		else
			ro_gui_set_icon_string(dialog_entry, 3, "");
	} else {
		ro_gui_set_window_title(dialog_entry, messages_get("NewLink"));
		ro_gui_set_icon_string(dialog_entry, 1, messages_get("Link"));
		ro_gui_set_icon_string(dialog_entry, 3, "");
	}
}


/**
 * Respond to a mouse click
 *
 * \param pointer  the pointer state
 */
void ro_gui_hotlist_dialog_click(wimp_pointer *pointer) {
	struct node_element *element;
	struct node *node;
	char *title = NULL;
	char *url = NULL;
	char *old_value;
	int icon = pointer->i;
	int close_icon, ok_icon;
	url_func_result res;

	if (pointer->w == dialog_entry) {
		title = strip(ro_gui_get_icon_string(pointer->w, 1));
		url = strip(ro_gui_get_icon_string(pointer->w, 3));
		close_icon = 4;
		ok_icon = 5;
		node = dialog_entry_node;
	} else {
		title = strip(ro_gui_get_icon_string(pointer->w, 1));
		close_icon = 2;
		ok_icon = 3;
		node = dialog_folder_node;
	}

	if (icon == close_icon) {
		if (pointer->buttons == wimp_CLICK_SELECT) {
			ro_gui_dialog_close(pointer->w);
			xwimp_create_menu((wimp_menu *)-1, 0, 0);
		} else {
			if (pointer->w == dialog_folder)
				ro_gui_hotlist_prepare_folder_dialog(
						dialog_folder_node);
			else
				ro_gui_hotlist_prepare_entry_dialog(
						dialog_entry_node);
		}
		return;
	}

	if (icon != ok_icon)
		return;

	/*	Check we have valid values
	*/
	if ((title != NULL) && (strlen(title) == 0)) {
		warn_user("NoNameError", 0);
		return;
	}
	if ((url != NULL) && (strlen(url) == 0)) {
		warn_user("NoURLError", 0);
		return;
	}

	/*	Update/insert our data
	*/
	if (!node) {
		if (pointer->w == dialog_folder) {
			dialog_folder_node = tree_create_folder_node(
					hotlist_tree->root,
					title);
			node = dialog_folder_node;
		} else {
			dialog_entry_node = tree_create_URL_node(
					hotlist_tree->root,
					title, url, 0xfaf, time(NULL), -1, 0);
			node = dialog_entry_node;
		}
		tree_handle_node_changed(hotlist_tree, node, true, false);
		ro_gui_tree_scroll_visible(hotlist_tree, &node->data);
		tree_redraw_area(hotlist_tree, node->box.x - NODE_INSTEP,
				0, NODE_INSTEP, 16384);
	} else {
		if (url) {
			element = tree_find_element(node, TREE_ELEMENT_URL);
			if (element) {
				old_value = element->text;
				res = url_normalize(url, &element->text);
				if (res != URL_FUNC_OK) {
					warn_user("NoMemory", 0);
					element->text = old_value;
					return;
				}
				free(old_value);
			}
		}
		if (title) {
			old_value = node->data.text;
			node->data.text = strdup(title);
			if (!node->data.text) {
				warn_user("NoMemory", 0);
				node->data.text = old_value;
				return;
			}
			free(old_value);
		}
		tree_handle_node_changed(hotlist_tree, node, true, false);
	}

	if (pointer->buttons == wimp_CLICK_SELECT) {
		ro_gui_dialog_close(pointer->w);
	  	ro_gui_menu_closed();
		return;
	}
	if (pointer->w == dialog_folder)
		ro_gui_hotlist_prepare_folder_dialog(dialog_folder_node);
	else
		ro_gui_hotlist_prepare_entry_dialog(dialog_entry_node);
}


/**
 * Attempts to process an interactive help message request
 *
 * \param x  the x co-ordinate to give help for
 * \param y  the x co-ordinate to give help for
 * \return the message code index
 */
int ro_gui_hotlist_help(int x, int y) {
	return -1;
}
