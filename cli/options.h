/* options.h

   Copyright (c) 2003-2020 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"

#define LAPSHARP_DEFAULT_PRESET      "medium"
#define UNSHARP_DEFAULT_PRESET       "medium"
#define CHROMA_SMOOTH_DEFAULT_PRESET "medium"
#define NLMEANS_DEFAULT_PRESET       "medium"
#define DEINTERLACE_DEFAULT_PRESET   "default"
#define DECOMB_DEFAULT_PRESET        "default"
#define DETELECINE_DEFAULT_PRESET    "default"
#define COMB_DETECT_DEFAULT_PRESET   "default"
#define HQDN3D_DEFAULT_PRESET        "medium"
#define ROTATE_DEFAULT               "angle=180:hflip=0"
#define DEBLOCK_DEFAULT_PRESET       "medium"

int ParseOptions( int argc, char ** argv );

int CheckOptions( int argc, char ** argv );

hb_dict_t * PreparePreset(const char *preset_name);

hb_dict_t* PrepareJob(hb_handle_t *h, hb_title_t *title, hb_dict_t *preset_dict);

void OptionsCleanup();