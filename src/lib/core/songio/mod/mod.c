/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Format description:
 * http://svn.dslinux.org/viewvc/dslinux/branches/vendor/xmp/docs/formats/Ultimate_Soundtracker-format.txt?view=markup
 * http://www.koeln.netsurf.de/~michael.mey/faq1.txt
 */
/* Strategy:
 * Simple: 
 * * just map everything to one mathildetracker
 *   - hard to edit afterwards
 * * create patterns for each position
 *
 * Advanced:
 * * create one mathildetracker for each wave
 *   + easy to add fx etc.
 * * detect duplicated patterns and join
 *   + easier to edit further
 */
