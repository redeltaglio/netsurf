/*
 * Copyright 2010 Ole Loots <ole@monochrom.net>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "desktop/browser.h"
#include "desktop/options.h"
#include "desktop/tree.h"
#include "desktop/tree_url_node.h"
#include "desktop/gui.h"
#include "desktop/history_global_core.h"
#include "desktop/browser.h"
#include "content/content.h"
#include "content/hlcache.h"
#include "content/urldb.h"
#include "utils/log.h"
#include "atari/treeview.h"
#include "atari/findfile.h"
#include "atari/res/netsurf.rsh"
#include "atari/history.h"

extern char * tree_directory_icon_name;

struct s_atari_global_history gl_history;


void global_history_add_recent( const char *url )
{

}

char **global_history_get_recent( int *count )
{
    return NULL;
}


void global_history_open( void )
{
	if( gl_history.init == false ) {
		printf("history not init");
		return;
	}
	if( gl_history.open == false ) {
		WindOpen( gl_history.window, -1, -1, app.w/3, app.h/2);
		gl_history.open = true;
		atari_treeview_open( gl_history.tv );
	} else {
		WindTop( gl_history.window );
	}
}

void global_history_close( void )
{
	WindClose(gl_history.window);
	gl_history.open = false;
	atari_treeview_close( gl_history.tv );
}


static void __CDECL evnt_history_close( WINDOW *win, short buff[8] )
{
	global_history_close();
}


static void __CDECL evnt_history_mbutton( WINDOW *win, short buff[8] )
{
	/* todo: implement popup?
	if(evnt.mbut & 2) {

	}
	*/
}

bool global_history_init( void )
{

	if( gl_history.init == false ) {

		int flags = ATARI_TREEVIEW_WIDGETS;
		gl_history.open = false;
		gl_history.window = WindCreate( flags, 40, 40, app.w, app.h );
		if( gl_history.window == NULL ) {
			LOG(("Failed to allocate history window"));
			return( false );
		}
		/* TODO: load history strings from messages */
		WindSetStr( gl_history.window, WF_NAME, (char*)"History" );
		//WindSetPtr( gl_history.window, WF_TOOLBAR, tree, evnt_history_toolbar );
		EvntAttach( gl_history.window, WM_CLOSED, evnt_history_close );
		EvntAttach( gl_history.window, WM_XBUTTON,evnt_history_mbutton );

		gl_history.tv = atari_treeview_create(
			history_global_get_tree_flags(),
			gl_history.window
		);
		if (gl_history.tv == NULL) {
			/* handle it properly, clean up previous allocs */
			LOG(("Failed to allocate history treeview"));
			return( false );
		}

		history_global_initialise( gl_history.tv->tree, "dir.png" );
		gl_history.init = true;
	}
	return( true );
}


void global_history_destroy( void )
{
	if( gl_history.init == false ) {
		return;
	}
	if( gl_history.window != NULL ) {
		history_global_cleanup();
		if( gl_history.open )
			global_history_close();
		WindDelete( gl_history.window );
		gl_history.window = NULL;
		atari_treeview_destroy( gl_history.tv  );
		gl_history.init = false;
	}
	LOG(("done"));
}


