/* help.c

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
#include "help.h"

/****************************************************************************
 * ShowHelp:
 ****************************************************************************/
static void showFilterPresets(FILE* const out, int filter_id)
{
    char ** names = hb_filter_get_presets_short_name(filter_id);
    int     ii, count = 0;

    // Count number of entries we want to display
    for (ii = 0; names[ii] != NULL; ii++)
    {
        if (!strcasecmp(names[ii], "custom") || // skip custom
            !strcasecmp(names[ii], "off")    || // skip off
            !strcasecmp(names[ii], "default"))  // skip default
            continue;
        count++;
    }

    // If there are no entries, display nothing.
    if (count == 0)
    {
        return;
    }
    fprintf(out, "                           Presets:\n");
    for (ii = 0; names[ii] != NULL; ii++)
    {
        if (!strcasecmp(names[ii], "custom") || // skip custom
            !strcasecmp(names[ii], "off")    || // skip off
            !strcasecmp(names[ii], "default"))  // skip default
            continue;
        fprintf(out, "                               %s\n", names[ii]);
    }

    hb_str_vfree(names);
}

static void showFilterTunes(FILE* const out, int filter_id)
{
    char ** tunes = hb_filter_get_tunes_short_name(filter_id);
    int     ii, count = 0;

    // Count number of entries we want to display
    for (ii = 0; tunes[ii] != NULL; ii++)
    {
        /*
        if (!strcasecmp(tunes[ii], "custom") || // skip custom
            !strcasecmp(tunes[ii], "off")    || // skip off
            !strcasecmp(tunes[ii], "default"))  // skip default
            continue;
        */
        count++;
    }

    // If there are no entries, display nothing.
    if (count == 0)
    {
        return;
    }
    fprintf(out, "                           Tunes:\n");
    for (ii = 0; tunes[ii] != NULL; ii++)
    {
        /*
        if (!strcasecmp(tunes[ii], "custom") || // skip custom
            !strcasecmp(tunes[ii], "off")    || // skip off
            !strcasecmp(tunes[ii], "default"))  // skip default
            continue;
        */
        fprintf(out, "                               %s\n", tunes[ii]);
    }

    hb_str_vfree(tunes);
}

static void showFilterKeys(FILE* const out, int filter_id)
{
    char ** keys = hb_filter_get_keys(filter_id);
    char  * colon = "", * newline;
    int     ii, linelen = 0;

    fprintf(out, "                           Custom Format:\n"
                 "                               ");
    for (ii = 0; keys[ii] != NULL; ii++)
    {
        int c = tolower(keys[ii][0]);
        int len = strlen(keys[ii]) + 3;
        if (linelen + len > 48)
        {
            newline = "\n                               ";
            linelen = 0;
        }
        else
        {
            newline = "";
        }
        fprintf(out, "%s%s%s=%c", colon, newline, keys[ii], c);
        linelen += len;
        colon = ":";
    }
    fprintf(out, "\n");
    hb_str_vfree(keys);
}

static void showFilterDefault(FILE* const out, int filter_id)
{
    const char * preset = "default";

    fprintf(out, "                           Default:\n"
                 "                               ");
    switch (filter_id)
    {
        case HB_FILTER_UNSHARP:
            preset = UNSHARP_DEFAULT_PRESET;
            break;
        case HB_FILTER_LAPSHARP:
            preset = LAPSHARP_DEFAULT_PRESET;
            break;
        case HB_FILTER_CHROMA_SMOOTH:
            preset = CHROMA_SMOOTH_DEFAULT_PRESET;
            break;
        case HB_FILTER_NLMEANS:
            preset = NLMEANS_DEFAULT_PRESET;
            break;
        case HB_FILTER_DEINTERLACE:
            preset = DEINTERLACE_DEFAULT_PRESET;
            break;
        case HB_FILTER_DECOMB:
            preset = DECOMB_DEFAULT_PRESET;
            break;
        case HB_FILTER_DETELECINE:
            preset = DETELECINE_DEFAULT_PRESET;
            break;
        case HB_FILTER_HQDN3D:
            preset = HQDN3D_DEFAULT_PRESET;
            break;
        case HB_FILTER_COMB_DETECT:
            preset = COMB_DETECT_DEFAULT_PRESET;
            break;
        case HB_FILTER_DEBLOCK:
            preset = DEBLOCK_DEFAULT_PRESET;
            break;
        default:
            break;
    }
    switch (filter_id)
    {
        case HB_FILTER_DEINTERLACE:
        case HB_FILTER_NLMEANS:
        case HB_FILTER_CHROMA_SMOOTH:
        case HB_FILTER_UNSHARP:
        case HB_FILTER_LAPSHARP:
        case HB_FILTER_DECOMB:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_HQDN3D:
        case HB_FILTER_COMB_DETECT:
        case HB_FILTER_DEBLOCK:
        {
            hb_dict_t * settings;
            settings = hb_generate_filter_settings(filter_id, preset,
                                                   NULL, NULL);
            char * str = hb_filter_settings_string(filter_id, settings);
            hb_value_free(&settings);

            char ** split = hb_str_vsplit(str, ':');
            char  * colon = "", * newline;
            int     ii, linelen = 0;

            for (ii = 0; split[ii] != NULL; ii++)
            {
                int len = strlen(split[ii]) + 1;
                if (linelen + len > 48)
                {
                    newline = "\n                               ";
                    linelen = 0;
                }
                else
                {
                    newline = "";
                }
                fprintf(out, "%s%s%s", colon, newline, split[ii]);
                linelen += len;
                colon = ":";
            }
            hb_str_vfree(split);
            free(str);
        } break;
        case HB_FILTER_ROTATE:
            fprintf(out, "%s", ROTATE_DEFAULT);
            break;
        default:
            break;
    }
    fprintf(out, "\n");
}

void ShowHelp()
{
    int i, clock_min, clock_max, clock;
    const hb_rate_t *rate;
    const hb_dither_t *dither;
    const hb_mixdown_t *mixdown;
    const hb_encoder_t *encoder;
    const hb_container_t *container;
    FILE* const out = stdout;

    fprintf( out,
"Usage: HandBrakeCLI [options] -i <source> -o <destination>\n"
"\n"
"General Options --------------------------------------------------------------\n"
"\n"
"   -h, --help              Print help\n"
"   --version               Print version\n"
"   --json                  Log title, progress, and version info in\n"
"                           JSON format\n"
"   -v, --verbose[=number]  Be verbose (optional argument: logging level)\n"
"   -Z. --preset <string>   Select preset by name (case-sensitive)\n"
"                           Enclose names containing spaces in double quotation\n"
"                           marks (e.g. \"Preset Name\")\n"
"   -z, --preset-list       List available presets\n"
"   --preset-import-file <filespec>\n"
"                           Import presets from a json preset file.\n"
"                           'filespec' may be a list of files separated\n"
"                           by spaces, or it may use shell wildcards.\n"
"   --preset-import-gui     Import presets from GUI config preset file.\n"
"   --preset-export <string>\n"
"                           Create a new preset from command line options and\n"
"                           write a json representation of the preset to the\n"
"                           console or a file if '--preset-export-file' is\n"
"                           specified. The required argument will be the name\n"
"                           of the new preset.\n"
"   --preset-export-file <filename>\n"
"                           Write new preset generated by '--preset-export'\n"
"                           to file 'filename'.\n"
"   --preset-export-description <string>\n"
"                           Add a description to the new preset created with\n"
"                           '--preset-export'\n"
"   --queue-import-file <filename>\n"
"                           Import an encode queue file created by the GUI\n"
"       --no-dvdnav         Do not use dvdnav for reading DVDs\n"
"\n"
"\n"
"Source Options ---------------------------------------------------------------\n"
"\n"
"   -i, --input <string>    Set input file or device (\"source\")\n"
"   -t, --title <number>    Select a title to encode (0 to scan all titles\n"
"                           only, default: 1)\n"
"       --min-duration      Set the minimum title duration (in seconds).\n"
"                           Shorter titles will be ignored (default: 10).\n"
"       --scan              Scan selected title only.\n"
"       --main-feature      Detect and select the main feature title.\n"
"   -c, --chapters <string> Select chapters (e.g. \"1-3\" for chapters\n"
"                           1 to 3 or \"3\" for chapter 3 only,\n"
"                           default: all chapters)\n"
"       --angle <number>    Select the video angle (DVD or Blu-ray only)\n"
"       --previews <number:boolean>\n"
"                           Select how many preview images are generated,\n"
"                           and whether to store to disk (0 or 1).\n"
"                           (default: 10:0)\n"
"   --start-at-preview <number>\n"
"                           Start encoding at a given preview.\n"
"   --start-at <string:number>\n"
"                           Start encoding at a given offset in seconds,\n"
"                           frames, or pts (on a 90kHz clock)\n"
"                           (e.g. seconds:10, frames:300, pts:900000).\n"
"                           Units must match --stop-at units, if specified.\n"
"   --stop-at  <string:number>\n"
"                           Stop encoding after a given duration in seconds,\n"
"                           frames, or pts (on a 90kHz clock) has passed\n"
"                           (e.g. seconds:10, frames:300, pts:900000).\n"
"                           Duration is relative to --start-at, if specified.\n"
"                           Units must match --start-at units, if specified.\n"
"\n"
"\n"
"Destination Options ----------------------------------------------------------\n"
"\n"
"   -o, --output <filename> Set destination file name\n"
"   -f, --format <string>   Select container format:\n");
    container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        fprintf(out, "                               %s\n", container->short_name);
    }
    fprintf(out,
"                           default: auto-detected from destination file name)\n"
"   -m, --markers           Add chapter markers\n"
"       --no-markers        Disable preset chapter markers\n"
"   -O, --optimize          Optimize MP4 files for HTTP streaming (fast start,\n"
"                           s.s. rewrite file to place MOOV atom at beginning)\n"
"       --no-optimize       Disable preset 'optimize'\n"
"   -I, --ipod-atom         Add iPod 5G compatibility atom to MP4 container\n"
"       --no-ipod-atom      Disable iPod 5G atom\n"
"       --align-av          Add audio silence or black video frames to start\n"
"                           of streams so that all streams start at exactly\n"
"                           the same time\n"
"   --inline-parameter-sets Create adaptive streaming compatible output.\n"
"                           Inserts parameter sets (SPS and PPS) inline\n"
"                           in the video stream before each IDR.\n"
"\n"
"\n"
"Video Options ----------------------------------------------------------------\n"
"\n"
"   -e, --encoder <string>  Select video encoder:\n");
    encoder = NULL;
    while ((encoder = hb_video_encoder_get_next(encoder)) != NULL)
    {
        fprintf(out, "                               %s\n", encoder->short_name);
    }
    fprintf(out,
"       --encoder-preset <string>\n"
"                           Adjust video encoding settings for a particular\n"
"                           speed/efficiency tradeoff (encoder-specific)\n"
"   --encoder-preset-list <string>\n"
"                           List supported --encoder-preset values for the\n"
"                           specified video encoder\n"
"       --encoder-tune <string>\n"
"                           Adjust video encoding settings for a particular\n"
"                           type of source or situation (encoder-specific)\n"
"   --encoder-tune-list <string>\n"
"                           List supported --encoder-tune values for the\n"
"                           specified video encoder\n"
"   -x, --encopts <string>  Specify advanced encoding options in the same\n"
"                           style as mencoder (all encoders except theora):\n"
"                           option1=value1:option2=value2\n"
"       --encoder-profile <string>\n"
"                           Ensure compliance with the requested codec\n"
"                           profile (encoder-specific)\n"
"   --encoder-profile-list <string>\n"
"                           List supported --encoder-profile values for the\n"
"                           specified video encoder\n"
"       --encoder-level <string>\n"
"                           Ensures compliance with the requested codec\n"
"                           level (encoder-specific)\n"
"   --encoder-level-list <string>\n"
"                           List supported --encoder-level values for the\n"
"                           specified video encoder\n"
"   -q, --quality <float>   Set video quality (e.g. 22.0)\n"
"   -b, --vb <number>       Set video bitrate in kbit/s (default: 1000)\n"
"   -2, --two-pass          Use two-pass mode\n"
"       --no-two-pass       Disable two-pass mode\n"
"   -T, --turbo             When using 2-pass use \"turbo\" options on the\n"
"                           first pass to improve speed\n"
"                           (works with x264 and x265)\n"
"       --no-turbo          Disable 2-pass mode's \"turbo\" first pass\n"
"   -r, --rate <float>      Set video framerate\n"
"                           (" );
    i = 0;
    rate = NULL;
    while ((rate = hb_video_framerate_get_next(rate)) != NULL)
    {
        if (i > 0)
        {
            // separate multiple items
            i++;
            fprintf(out, "/");
        }
        if (hb_video_framerate_get_next(rate) != NULL)
        {
            if (i + strlen(rate->name) > 32)
            {
                // break long lines
                i = 0;
                fprintf(out, "\n                           ");
            }
            i += strlen(rate->name);
        }
        fprintf(out, "%s", rate->name);
    }
    fprintf( out, "\n"
"                           or a number between " );
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);
    fprintf(out, "%i and %i", (clock / clock_max), (clock / clock_min));
    fprintf( out, ").\n"
"                           Be aware that not specifying a framerate lets\n"
"                           HandBrake preserve a source's time stamps,\n"
"                           potentially creating variable framerate video\n"
"   --vfr, --cfr, --pfr     Select variable, constant or peak-limited\n"
"                           frame rate control. VFR preserves the source\n"
"                           timing. CFR makes the output constant rate at\n"
"                           the rate given by the -r flag (or the source's\n"
"                           average rate if no -r is given). PFR doesn't\n"
"                           allow the rate to go over the rate specified\n"
"                           with the -r flag but won't change the source\n"
"                           timing if it's below that rate.\n"
"                           If none of these flags are given, the default\n"
"                           is --pfr when -r is given and --vfr otherwise\n"
"\n"
"\n"
"Audio Options ----------------------------------------------------------------\n"
"\n"
"       --audio-lang-list <string>\n"
"                           Specify a comma separated list of audio\n"
"                           languages you would like to select from the\n"
"                           source title. By default, the first audio\n"
"                           matching each language will be added to your\n"
"                           output. Provide the language's ISO 639-2 code\n"
"                           (e.g. fre, eng, spa, dut, et cetera)\n"
"                           Use code 'und' (Unknown) to match all languages.\n"
"       --all-audio         Select all audio tracks matching languages in\n"
"                           the specified language list (--audio-lang-list).\n"
"                           Any language if list is not specified.\n"
"       --first-audio       Select first audio track matching languages in\n"
"                           the specified language list (--audio-lang-list).\n"
"                           Any language if list is not specified.\n"
"   -a, --audio <string>    Select audio track(s), separated by commas\n"
"                           (\"none\" for no audio, \"1,2,3\" for multiple\n"
"                           tracks, default: first one).\n"
"                           Multiple output tracks can be used for one input.\n"
"   -E, --aencoder <string> Select audio encoder(s):\n" );
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        fprintf(out, "                               %s\n", encoder->short_name);
    }
    fprintf(out,
"                           \"copy:<type>\" will pass through the corresponding\n"
"                           audio track without modification, if pass through\n"
"                           is supported for the audio type.\n"
"                           Separate tracks by commas.\n"
"                           Defaults:\n");
    container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        int audio_encoder = hb_audio_encoder_get_default(container->format);
        fprintf(out, "                               %-8s %s\n",
                container->short_name,
                hb_audio_encoder_get_short_name(audio_encoder));
    }
    fprintf(out,
"       --audio-copy-mask <string>\n"
"                           Set audio codecs that are permitted when the\n"
"                           \"copy\" audio encoder option is specified\n"
"                           (" );
    i       = 0;
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if ((encoder->codec &  HB_ACODEC_PASS_FLAG) &&
            (encoder->codec != HB_ACODEC_AUTO_PASS))
        {
            if (i)
            {
                fprintf(out, "/");
            }
            i = 1;
            // skip "copy:"
            fprintf(out, "%s", encoder->short_name + 5);
        }
    }
    fprintf(out, ")\n"
"                           Separated by commas for multiple allowed options.\n"
"       --audio-fallback <string>\n"
"                           Set audio codec to use when it is not possible\n"
"                           to copy an audio track without re-encoding.\n"
"   -B, --ab <number>       Set audio track bitrate(s) in kbit/s.\n"
"                           (default: determined by the selected codec, mixdown,\n"
"                           and samplerate combination).\n"
"                           Separate tracks by commas.\n"
"   -Q, --aq <float>        Set audio quality metric.\n"
"                           Separate tracks by commas.\n"
"   -C, --ac <float>        Set audio compression metric.\n"
"                           (available depending on selected codec)\n"
"                           Separate tracks by commas.\n"
"   -6, --mixdown <string>  Format(s) for audio downmixing/upmixing:\n");
    // skip HB_AMIXDOWN_NONE
    mixdown = hb_mixdown_get_next(NULL);
    while((mixdown = hb_mixdown_get_next(mixdown)) != NULL)
    {
        fprintf(out, "                               %s\n",
                mixdown->short_name);
    }
    fprintf(out,
"                           Separate tracks by commas.\n"
"                           Defaults:\n");
    encoder = NULL;
    while((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if (!(encoder->codec & HB_ACODEC_PASS_FLAG))
        {
            // layout: UINT64_MAX (all channels) should work with any mixdown
            int mixdown = hb_mixdown_get_default(encoder->codec, UINT64_MAX);
            // assumes that the encoder short name is <= 16 characters long
            fprintf(out, "                               %-16s up to %s\n",
                    encoder->short_name, hb_mixdown_get_short_name(mixdown));
        }
    }
    fprintf(out,
"       --normalize-mix     Normalize audio mix levels to prevent clipping.\n"
"              <string>     Separate tracks by commas.\n"
"                           0 = Disable Normalization (default)\n"
"                           1 = Enable Normalization\n"
"   -R, --arate             Set audio samplerate(s)\n"
"                           (" );
    rate = NULL;
    while ((rate = hb_audio_samplerate_get_next(rate)) != NULL)
    {
        fprintf(out, "%s", rate->name);
        if (hb_audio_samplerate_get_next(rate) != NULL)
        {
            fprintf(out, "/");
        }
    }
    fprintf( out, " kHz)\n"
"                           or \"auto\". Separate tracks by commas.\n"
"   -D, --drc <float>       Apply extra dynamic range compression to the\n"
"                           audio, making soft sounds louder. Range is 1.0\n"
"                           to 4.0 (too loud), with 1.5 - 2.5 being a useful\n"
"                           range.\n"
"                           Separate tracks by commas.\n"
"       --gain <float>      Amplify or attenuate audio before encoding.  Does\n"
"                           NOT work with audio passthru (copy). Values are\n"
"                           in dB.  Negative values attenuate, positive\n"
"                           values amplify. A 1 dB difference is barely\n"
"                           audible.\n"
"       --adither <string>  Select dithering to apply before encoding audio:\n");
    dither = NULL;
    while ((dither = hb_audio_dither_get_next(dither)) != NULL)
    {
        if (dither->method == hb_audio_dither_get_default())
        {
            fprintf(out, "                               %s (default)\n", dither->short_name);
        }
        else
        {
            fprintf(out, "                               %s\n", dither->short_name);
        }
    }
    fprintf(out,
"                           Separate tracks by commas.\n"
"                           Supported by encoder(s):\n");
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if (hb_audio_dither_is_supported(encoder->codec, 0))
        {
            fprintf(out, "                               %s\n", encoder->short_name);
        }
    }
    fprintf(out,
"   -A, --aname <string>    Set audio track name(s).\n"
"                           Separate tracks by commas.\n"
"\n"
"\n"
"Picture Options --------------------------------------------------------------\n"
"\n"
"   -w, --width  <number>   Set storage width in pixels\n"
"   -l, --height <number>   Set storage height in pixels\n"
"       --crop   <top:bottom:left:right>\n"
"                           Set picture cropping in pixels\n"
"                           (default: automatically remove black bars)\n"
"       --loose-crop        Always crop to a multiple of the modulus\n"
"       --no-loose-crop     Disable preset 'loose-crop'\n"
"   -Y, --maxHeight <number>\n"
"                           Set maximum height in pixels\n"
"   -X, --maxWidth  <number>\n"
"                           Set maximum width in pixels\n"
"   --non-anamorphic        Set pixel aspect ratio to 1:1\n"
"   --auto-anamorphic       Store pixel aspect ratio that maximizes storage\n"
"                           resolution\n"
"   --loose-anamorphic      Store pixel aspect ratio that is as close as\n"
"                           possible to the source video pixel aspect ratio\n"
"   --custom-anamorphic     Store pixel aspect ratio in video stream and\n"
"                           directly control all parameters.\n"
"   --display-width <number>\n"
"                           Set display width in pixels, for custom anamorphic.\n"
"                           This determines the display aspect during playback,\n"
"                           which may differ from the storage aspect.\n"
"   --keep-display-aspect   Preserve the source's display aspect ratio\n"
"                           when using custom anamorphic\n"
"   --no-keep-display-aspect\n"
"                           Disable preset 'keep-display-aspect'\n"
"   --pixel-aspect <par_x:par_y>\n"
"                           Set pixel aspect for custom anamorphic\n"
"                           (--display-width and --pixel-aspect are mutually\n"
"                           exclusive.\n"
"   --itu-par               Use wider ITU pixel aspect values for loose and\n"
"                           custom anamorphic, useful with underscanned sources\n"
"   --no-itu-par            Disable preset 'itu-par'\n"
"   --modulus <number>      Set storage width and height modulus\n"
"                           Dimensions will be made divisible by this number.\n"
"                           (default: set by preset, typically 2)\n"
"   -M, --color-matrix <string>\n"
"                           Set the color space signaled by the output:\n"
"                           Overrides color signalling with no conversion.\n"
"                               2020\n"
"                               709\n"
"                               601\n"
"                               ntsc (same as 601)\n"
"                               pal\n"
"                           (default: auto-detected from source)\n"
"\n"
"\n"
"Filters Options --------------------------------------------------------------\n"
"\n"
"   --comb-detect[=string]  Detect interlace artifacts in frames.\n"
"                           If not accompanied by the decomb or deinterlace\n"
"                           filters, this filter only logs the interlaced\n"
"                           frame count to the activity log.\n"
"                           If accompanied by the decomb or deinterlace\n"
"                           filters, it causes these filters to selectively\n"
"                           deinterlace only those frames where interlacing\n"
"                           is detected.\n");
    showFilterPresets(out, HB_FILTER_COMB_DETECT);
    showFilterKeys(out, HB_FILTER_COMB_DETECT);
    showFilterDefault(out, HB_FILTER_COMB_DETECT);
    fprintf( out,
"   --no-comb-detect        Disable preset comb-detect filter\n"
"   -d, --deinterlace[=string]\n"
"                           Deinterlace video using FFmpeg yadif.\n");
    showFilterPresets(out, HB_FILTER_DEINTERLACE);
    showFilterKeys(out, HB_FILTER_DEINTERLACE);
    showFilterDefault(out, HB_FILTER_DEINTERLACE);
    fprintf( out,
"       --no-deinterlace    Disable preset deinterlace filter\n"
"   -5, --decomb[=string]   Deinterlace video using a combination of yadif,\n"
"                           blend, cubic, or EEDI2 interpolation.\n");
    showFilterPresets(out, HB_FILTER_DECOMB);
    showFilterKeys(out, HB_FILTER_DECOMB);
    showFilterDefault(out, HB_FILTER_DECOMB);
    fprintf( out,
"   --no-decomb             Disable preset decomb filter\n"
"   -9, --detelecine[=string]\n"
"                           Detelecine (ivtc) video with pullup filter\n"
"                           Note: this filter drops duplicate frames to\n"
"                           restore the pre-telecine framerate, unless you\n"
"                           specify a constant framerate\n"
"                           (--rate 29.97 --cfr)\n");
    showFilterPresets(out, HB_FILTER_DETELECINE);
    showFilterKeys(out, HB_FILTER_DETELECINE);
    showFilterDefault(out, HB_FILTER_DETELECINE);
    fprintf( out,
"   --no-detelecine         Disable preset detelecine filter\n"
"   -8, --hqdn3d[=string]   Denoise video with hqdn3d filter\n");
    showFilterPresets(out, HB_FILTER_HQDN3D);
    showFilterKeys(out, HB_FILTER_HQDN3D);
    showFilterDefault(out, HB_FILTER_HQDN3D);
    fprintf( out,
"   --no-hqdn3d             Disable preset hqdn3d filter\n"
"   --denoise[=string]      Legacy alias for '--hqdn3d'\n"
"   --nlmeans[=string]      Denoise video with NLMeans filter\n");
    showFilterPresets(out, HB_FILTER_NLMEANS);
    showFilterKeys(out, HB_FILTER_NLMEANS);
    showFilterDefault(out, HB_FILTER_NLMEANS);
    fprintf( out,

"   --no-nlmeans            Disable preset NLMeans filter\n"
"   --nlmeans-tune <string> Tune NLMeans filter to content type\n");
    showFilterTunes(out, HB_FILTER_NLMEANS);
    fprintf( out,
"                           Applies to NLMeans presets only (does not affect\n"
"                           custom settings)\n"
"   --chroma-smooth[=string]      Sharpen video with chroma smooth filter\n");
    showFilterPresets(out, HB_FILTER_CHROMA_SMOOTH);
    showFilterKeys(out, HB_FILTER_CHROMA_SMOOTH);
    showFilterDefault(out, HB_FILTER_CHROMA_SMOOTH);
    fprintf( out,

"   --no-chroma-smooth            Disable preset chroma smooth filter\n"
"   --chroma-smooth-tune <string> Tune chroma smooth filter\n");
    showFilterTunes(out, HB_FILTER_CHROMA_SMOOTH);
    fprintf( out,
"                                 Applies to chroma smooth presets only (does\n"
"                                 not affect custom settings)\n"
"   --unsharp[=string]      Sharpen video with unsharp filter\n");
    showFilterPresets(out, HB_FILTER_UNSHARP);
    showFilterKeys(out, HB_FILTER_UNSHARP);
    showFilterDefault(out, HB_FILTER_UNSHARP);
    fprintf( out,

"   --no-unsharp            Disable preset unsharp filter\n"
"   --unsharp-tune <string> Tune unsharp filter\n");
    showFilterTunes(out, HB_FILTER_UNSHARP);
    fprintf( out,
"                           Applies to unsharp presets only (does not affect\n"
"                           custom settings)\n"
"   --lapsharp[=string]     Sharpen video with lapsharp filter\n");
    showFilterPresets(out, HB_FILTER_LAPSHARP);
    showFilterKeys(out, HB_FILTER_LAPSHARP);
    showFilterDefault(out, HB_FILTER_LAPSHARP);
    fprintf( out,

"   --no-lapsharp           Disable preset lapsharp filter\n"
"   --lapsharp-tune <string>\n"
"                           Tune lapsharp filter\n");
    showFilterTunes(out, HB_FILTER_LAPSHARP);
    fprintf( out,
"                           Applies to lapsharp presets only (does not affect\n"
"                           custom settings)\n"
"   -7, --deblock[=string]  Deblock video with avfilter deblock\n");
    showFilterPresets(out, HB_FILTER_DEBLOCK);
    showFilterKeys(out, HB_FILTER_DEBLOCK);
    showFilterDefault(out, HB_FILTER_DEBLOCK);
    fprintf( out,
"   --no-deblock            Disable preset deblock filter\n"
"   --deblock-tune <string>\n"
"                           Tune deblock filter\n");
    showFilterTunes(out, HB_FILTER_DEBLOCK);
    fprintf( out,
"                           Applies to deblock presets only (does not affect\n"
"                           custom settings)\n"
"   --rotate[=string]       Rotate image or flip its axes.\n"
"                           angle rotates clockwise, can be one of:\n"
"                               0, 90, 180, 270\n"
"                           hflip=1 flips the image on the x axis (horizontally).\n");
    showFilterKeys(out, HB_FILTER_ROTATE);
    showFilterDefault(out, HB_FILTER_ROTATE);
    fprintf( out,
"   --pad <string>          Pad image with borders (e.g. letterbox).\n"
"                           The padding color may be set (default black).\n"
"                           Color may be an HTML color name or RGB value.\n"
"                           The position of image in pad may also be set.\n");
    showFilterKeys(out, HB_FILTER_PAD);
    fprintf( out,
"   -g, --grayscale         Grayscale encoding\n"
"   --no-grayscale          Disable preset 'grayscale'\n"
"\n"
"\n"
"Subtitles Options ------------------------------------------------------------\n"
"\n"
"  --subtitle-lang-list <string>\n"
"                           Specify a comma separated list of subtitle\n"
"                           languages you would like to select from the\n"
"                           source title. By default, the first subtitle\n"
"                           matching each language will be added to your\n"
"                           output. Provide the language's ISO 639-2 code\n"
"                           (e.g. fre, eng, spa, dut, et cetera)\n"
"      --all-subtitles      Select all subtitle tracks matching languages in\n"
"                           the specified language list\n"
"                           (--subtitle-lang-list).\n"
"                           Any language if list is not specified.\n"
"      --first-subtitle     Select first subtitle track matching languages in\n"
"                           the specified language list\n"
"                           (--subtitle-lang-list).\n"
"                           Any language if list is not specified.\n"
"  -s, --subtitle <string>  Select subtitle track(s), separated by commas\n"
"                           More than one output track can be used for one\n"
"                           input. \"none\" for no subtitles.\n"
"                           Example: \"1,2,3\" for multiple tracks.\n"
"                           A special track name \"scan\" adds an extra first\n"
"                           pass. This extra pass scans subtitles matching\n"
"                           the language of the first audio or the language \n"
"                           selected by --native-language.\n"
"                           The one that's only used 10 percent of the time\n"
"                           or less is selected. This should locate subtitles\n"
"                           for short foreign language segments. Best used in\n"
"                           conjunction with --subtitle-forced.\n"
"  -S, --subname <string>   Set subtitle track name(s).\n"
"                           Separate tracks by commas.\n"
"  -F, --subtitle-forced[=string]\n"
"                           Only display subtitles from the selected stream\n"
"                           if the subtitle has the forced flag set. The\n"
"                           values in 'string' are indexes into the\n"
"                           subtitle list specified with '--subtitle'.\n"
"                           Separate tracks by commas.\n"
"                           Example: \"1,2,3\" for multiple tracks.\n"
"                           If \"string\" is omitted, the first track is\n"
"                           forced.\n"
"      --subtitle-burned[=number, \"native\", or \"none\"]\n"
"                           \"Burn\" the selected subtitle into the video\n"
"                           track. If \"subtitle\" is omitted, the first\n"
"                           track is burned. \"subtitle\" is an index into\n"
"                           the subtitle list specified with '--subtitle'\n"
"                           or \"native\" to burn the subtitle track that may\n"
"                           be added by the 'native-language' option.\n"
"      --subtitle-default[=number or \"none\"]\n"
"                           Flag the selected subtitle as the default\n"
"                           subtitle to be displayed upon playback.  Setting\n"
"                           no default means no subtitle will be displayed\n"
"                           automatically. 'number' is an index into the\n"
"                           subtitle list specified with '--subtitle'.\n"
"                           \"none\" may be used to override an automatically\n"
"                           selected default subtitle track.\n"
"  -N, --native-language <string>\n"
"                           Specify your language preference. When the first\n"
"                           audio track does not match your native language\n"
"                           then select the first subtitle that does. When\n"
"                           used in conjunction with --native-dub the audio\n"
"                           track is changed in preference to subtitles.\n"
"                           Provide the language's ISO 639-2 code\n"
"                           (e.g. fre, eng, spa, dut, et cetera)\n"
"      --native-dub         Used in conjunction with --native-language\n"
"                           requests that if no audio tracks are selected the\n"
"                           default selected audio track will be the first\n"
"                           one that matches the --native-language. If there\n"
"                           are no matching audio tracks then the first\n"
"                           matching subtitle track is used instead.\n"
"     --srt-file <string>   SubRip SRT filename(s), separated by commas.\n"
"     --srt-codeset <string>\n"
"                           Character codeset(s) that the SRT file(s) are\n"
"                           encoded as, separated by commas.\n"
"                           If not specified, 'latin1' is assumed.\n"
"                           Command 'iconv -l' provides a list of valid codesets.\n"
"     --srt-offset <string> Offset (in milliseconds) to apply to the SRT\n"
"                           file(s), separated by commas. If not specified,\n"
"                           zero is assumed. Offsets may be negative.\n"
"     --srt-lang <string>   SRT track language as an ISO 639-2 code\n"
"                           (e.g. fre, eng, spa, dut, et cetera)\n"
"                           If not specified, then 'und' is used.\n"
"                           Separate by commas.\n"
"     --srt-default[=number]\n"
"                           Flag the selected SRT as the default subtitle\n"
"                           to be displayed during playback.\n"
"                           Setting no default means no subtitle will be\n"
"                           automatically displayed. If 'number' is omitted,\n"
"                           the first SRT is the default.\n"
"                           'number' is a 1-based index into the 'srt-file' list\n"
"     --srt-burn[=number]   \"Burn\" the selected SRT subtitle into\n"
"                           the video track.\n"
"                           If 'number' is omitted, the first SRT is burned.\n"
"                           'number' is a 1-based index into the 'srt-file' list\n"
"     --ssa-file <string>   SubStationAlpha SSA filename(s), separated by\n"
"                           commas.\n"
"     --ssa-offset <string> Offset (in milliseconds) to apply to the SSA\n"
"                           file(s), separated by commas. If not specified,\n"
"                           zero is assumed. Offsets may be negative.\n"
"     --ssa-lang <string>   SSA track language as an ISO 639-2 code\n"
"                           (e.g. fre, eng, spa, dut, et cetera)\n"
"                           If not specified, then 'und' is used.\n"
"                           Separate by commas.\n"
"     --ssa-default[=number]\n"
"                           Flag the selected SSA as the default subtitle\n"
"                           to be displayed during playback.\n"
"                           Setting no default means no subtitle will be\n"
"                           automatically displayed. If 'number' is omitted,\n"
"                           the first SSA is the default.\n"
"                           'number' is a 1-based index into the 'ssa-file' list\n"
"     --ssa-burn[=number]   \"Burn\" the selected SSA subtitle into\n"
"                           the video track.\n"
"                           If 'number' is omitted, the first SSA is burned.\n"
"                           'number' is a 1-based index into the 'ssa-file' list\n"
"\n"
    );

#if HB_PROJECT_FEATURE_QSV
if (hb_qsv_available())
{
    fprintf( out,
"\n"
"-- Intel Quick Sync Video Options --------------------------------------------\n"
"\n"
"   --enable-qsv-decoding   Allow QSV hardware decoding of the video track\n"
"   --disable-qsv-decoding  Disable QSV hardware decoding of the video track,\n"
"                           forcing software decoding instead\n"
"   --qsv-async-depth[=number]\n"
"                           Set the number of asynchronous operations that\n"
"                           should be performed before the result is\n"
"                           explicitly synchronized.\n"
"                           Omit 'number' for zero.\n"
"                           (default: 4)\n"
"\n"
    );
}
#endif
}

