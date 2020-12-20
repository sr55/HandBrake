/* presets.c

   Copyright (c) 2003-2020 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef SYS_SunOS
#include <strings.h>
#endif

#if defined( __MINGW32__ )
#include <windows.h>
#include <conio.h>
#endif

#include "handbrake/handbrake.h"

 /****************************************************************************
 * ShowPresets:
 ****************************************************************************/
static const char *
reverse_search_char(const char *front, const char *back, char delim)
{
    while (back != front && *back != delim)
        back--;
    return back;
}

#if defined( __MINGW32__ )
static char * my_strndup(const char *src, int len)
{
    int src_len = strlen(src);
    int alloc = src_len < len ? src_len + 1 : len + 1;
    char *result = malloc(alloc);
    strncpy(result, src, alloc - 1);
    result[alloc - 1] = 0;
    return result;
}
#else
#define my_strndup strndup
#endif

static char** str_width_split( const char *str, int width )
{
    const char *  pos;
    const char *  end;
    char ** ret;
    int     count, ii;
    int     len;
    char    delem = ' ';

    if ( str == NULL || str[0] == 0 )
    {
        ret = malloc( sizeof(char*) );
        if ( ret == NULL ) return ret;
        *ret = NULL;
        return ret;
    }

    len = strlen(str);

    // Find number of elements in the string
    count = 1;
    pos = str;
    end = pos + width;
    while (end < str + len)
    {
        end = reverse_search_char(pos, end, delem);
        if (end == pos)
        {
            // Shouldn't happen for reasonable input
            break;
        }
        count++;
        pos = end + 1;
        end = pos + width;
    }
    count++;
    ret = calloc( ( count + 1 ), sizeof(char*) );
    if ( ret == NULL ) return ret;

    pos = str;
    end = pos + width;
    for (ii = 0; ii < count - 1 && end < str + len; ii++)
    {
        end = reverse_search_char(pos, end, delem);
        if (end == pos)
        {
            break;
        }
        ret[ii] = my_strndup(pos, end - pos);
        pos = end + 1;
        end = pos + width;
    }
    if (*pos != 0 && ii < count - 1)
    {
        ret[ii] = my_strndup(pos, width);
    }

    return ret;
}

static void Indent(FILE *f, char *whitespace, int indent)
{
    int ii;
    for (ii = 0; ii < indent; ii++)
    {
        fprintf(f, "%s", whitespace);
    }
}

void ShowPresets(hb_value_array_t *presets, int indent, int descriptions)
{
    if (presets == NULL)
        presets = hb_presets_get();

    int count = hb_value_array_len(presets);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        const char *name;
        hb_dict_t *preset_dict = hb_value_array_get(presets, ii);
        name = hb_value_get_string(hb_dict_get(preset_dict, "PresetName"));
        Indent(stderr, "    ", indent);
        if (hb_value_get_bool(hb_dict_get(preset_dict, "Folder")))
        {
            indent++;
            fprintf(stderr, "%s/\n", name);
            hb_value_array_t *children;
            children = hb_dict_get(preset_dict, "ChildrenArray");
            if (children == NULL)
                continue;
            ShowPresets(children, indent, descriptions);
            indent--;
        }
        else
        {
            fprintf(stderr, "%s\n", name);
            if (descriptions)
            {
                const char *desc;
                desc = hb_value_get_string(hb_dict_get(preset_dict,
                                                       "PresetDescription"));
                if (desc != NULL && desc[0] != 0)
                {
                    int ii;
                    char **split = str_width_split(desc, 60);
                    for (ii = 0; split[ii] != NULL; ii++)
                    {
                        Indent(stderr, "    ", indent+1);
                        fprintf(stderr, "%s\n", split[ii]);
                    }
                    hb_str_vfree(split);
                }
            }
        }
    }
}

