/* cli.h

   Copyright (c) 2003-2020 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* Options */
static int     debug               = HB_DEBUG_ALL;
static int     json                = 0;
static int     dvdnav              = 1;
static char *  input               = NULL;
static char *  output              = NULL;
static int     titleindex          = 1;
static int     titlescan           = 0;
static int     main_feature        = 0;

static char *   preset_export_name   = NULL;
static char *   preset_export_desc   = NULL;
static char *   preset_export_file   = NULL;
static char *   preset_name          = NULL;
static char *   queue_import_name    = NULL;
static int      preview_count = 10;
static int      store_previews = 0;
static uint64_t min_title_duration = 10;
