/*
 * Copyright 2005 John M Bell <jmb202@ecs.soton.ac.uk>
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

/** \file
 * UCS conversion tables (interface)
 * This is only used if nothing claims Service_International,8
 */

extern struct gui_utf8_table *riscos_utf8_table;

nserror utf8_to_local_encoding(const char *string, size_t len, char **result);
nserror utf8_from_local_encoding(const char *string, size_t len, char **result);

const int *ucstable_from_alphabet(int alphabet);
