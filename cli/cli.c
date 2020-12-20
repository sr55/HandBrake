/* cli.c

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

#if defined( PTW32_STATIC_LIB )
#include <pthread.h>
#endif

#include "handbrake/handbrake.h"
#include "handbrake/lang.h"
#include "parsecsv.h"
#include "help.h"
#include "presets.h"
#include "options.h"
#include "cli.h"

#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_common.h"
#endif

#if defined( __APPLE_CC__ )
#import <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include <sys/mount.h>
#endif

/* Exit cleanly on Ctrl-C */
static volatile hb_error_code done_error = HB_ERROR_NONE;
static volatile int die = 0;
static volatile int work_done = 0;
static void SigHandler( int );

/* Utils */
static void ShowCommands()
{
    fprintf(stdout, "\nCommands:\n");
    fprintf(stdout, " [h]elp    Show this message\n");
    fprintf(stdout, " [q]uit    Exit HandBrakeCLI\n");
    fprintf(stdout, " [p]ause   Pause encoding\n");
    fprintf(stdout, " [r]esume  Resume encoding\n");
}

static int         HandleEvents( hb_handle_t * h, hb_dict_t *preset_dict );

#ifdef __APPLE_CC__
static char* bsd_name_for_path(char *path);
static int device_is_dvd(char *device);
static io_service_t get_iokit_service( char *device );
static int is_dvd_service( io_service_t service );
static int is_whole_media_service( io_service_t service );
#endif

/* Only print the "Muxing..." message once */
static int show_mux_warning = 1;

/* Terminal detection */
static int stdout_tty = 0;
static int stderr_tty = 0;
static char * stdout_sep = "\r";
static char * stderr_sep = "\r";
static void test_tty()
{
#if defined(__MINGW32__)
    HANDLE handle;
    handle = (HANDLE) _get_osfhandle(_fileno(stdout));
    if ((handle != INVALID_HANDLE_VALUE) && (GetFileType(handle) == FILE_TYPE_CHAR))
    {
        stdout_tty = 1;
    }
    handle = (HANDLE) _get_osfhandle(_fileno(stderr));
    if ((handle != INVALID_HANDLE_VALUE) && (GetFileType(handle) == FILE_TYPE_CHAR))
    {
        stderr_tty = 1;
    }
#else
    if (isatty(1) == 1)
    {
        stdout_tty = 1;
    }
    if (isatty(2) == 1)
    {
        stderr_tty = 1;
    }
#endif

/*
    if (stdout_tty == 1) stdout_sep = "\r";
    if (stderr_tty == 1) stderr_sep = "\r";
*/
}

/****************************************************************************
 * hb_error_handler
 *
 * When using the CLI just display using hb_log as we always did in the past
 * make sure that we prefix with a nice ERROR message to catch peoples eyes.
 ****************************************************************************/
static void hb_cli_error_handler ( const char *errmsg )
{
    fprintf( stderr, "ERROR: %s\n", errmsg );
}

static int get_argv_utf8(int *argc_ptr, char ***argv_ptr)
{
#if defined( __MINGW32__ )
    int ret = 0;
    int argc;
    char **argv;

    wchar_t **argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv_utf16)
    {
        int i;
        int offset = (argc+1) * sizeof(char*);
        int size = offset;

        for(i = 0; i < argc; i++)
            size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, NULL, 0, NULL, NULL );

        argv = malloc(size);
        if (argv != NULL)
        {
            for (i = 0; i < argc; i++)
            {
                argv[i] = (char*)argv + offset;
                offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, argv[i], size-offset, NULL, NULL);
            }
            argv[argc] = NULL;
            ret = 1;
        }
        LocalFree(argv_utf16);
    }
    if (ret)
    {
        *argc_ptr = argc;
        *argv_ptr = argv;
    }
    return ret;
#else
    // On other systems, assume command line is already utf8
    return 1;
#endif
}

static volatile int job_running = 0;

void EventLoop(hb_handle_t *h, hb_dict_t *preset_dict)
{
    /* Wait... */
    work_done = 0;
    while (!die && !work_done)
    {
#if defined( __MINGW32__ )
        if( _kbhit() ) {
            switch( _getch() )
            {
                case 0x03: /* ctrl-c */
                case 'q':
                    fprintf( stdout, "\nEncoding Quit by user command\n" );
                    done_error = HB_ERROR_CANCELED;
                    die = 1;
                    break;
                case 'p':
                    fprintf(stdout,
                            "\nEncoding Paused by user command, 'r' to resume\n");
                    hb_pause(h);
                    hb_system_sleep_allow(h);
                    break;
                case 'r':
                    hb_system_sleep_prevent(h);
                    hb_resume(h);
                    break;
                case 'h':
                    ShowCommands();
                    break;
            }
        }
#else
        fd_set         fds;
        struct timeval tv;
        int            ret;
        char           buf[257];

        tv.tv_sec  = 0;
        tv.tv_usec = 100000;

        FD_ZERO( &fds );
        FD_SET( STDIN_FILENO, &fds );
        ret = select( STDIN_FILENO + 1, &fds, NULL, NULL, &tv );

        if( ret > 0 )
        {
            int size = 0;

            while( size < 256 &&
                   read( STDIN_FILENO, &buf[size], 1 ) > 0 )
            {
                if( buf[size] == '\n' )
                {
                    break;
                }
                size++;
            }

            if( size >= 256 || buf[size] == '\n' )
            {
                switch( buf[0] )
                {
                    case 'q':
                        fprintf( stdout, "\nEncoding Quit by user command\n" );
                        done_error = HB_ERROR_CANCELED;
                        die = 1;
                        break;
                    case 'p':
                        fprintf(stdout,
                                "\nEncoding Paused by user command, 'r' to resume\n");
                        hb_pause(h);
                        hb_system_sleep_allow(h);
                        break;
                    case 'r':
                        hb_system_sleep_prevent(h);
                        hb_resume(h);
                        break;
                    case 'h':
                        ShowCommands();
                        break;
                }
            }
        }
#endif
        hb_snooze(200);

        HandleEvents( h, preset_dict );
    }
    job_running = 0;
}

int RunQueueJob(hb_handle_t *h, hb_dict_t *job_dict)
{
    if (job_dict == NULL)
    {
        return -1;
    }

    char * json_job;
    json_job = hb_value_get_json(job_dict);
    hb_value_free(&job_dict);
    if (json_job == NULL)
    {
        fprintf(stderr, "Error in setting up job! Aborting.\n");
        return -1;
    }

    hb_add_json(h, json_job);
    free(json_job);
    job_running = 1;
    hb_start( h );

    EventLoop(h, NULL);

    return 0;
}

int RunQueue(hb_handle_t *h, const char *queue_import_name)
{
    hb_value_t * queue = hb_value_read_json(queue_import_name);

    if (hb_value_type(queue) == HB_VALUE_TYPE_DICT)
    {
        return RunQueueJob(h, hb_dict_get(queue, "Job"));
    }
    else if (hb_value_type(queue) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count, result = 0;

        count = hb_value_array_len(queue);
        for (ii = 0; ii < count; ii++)
        {
            hb_dict_t * entry = hb_value_array_get(queue, ii);
            int ret = RunQueueJob(h, hb_dict_get(entry, "Job"));
            if (ret < 0)
            {
                result = ret;
            }
            if (die)
            {
                break;
            }
        }
        return result;
    }
    else
    {
        fprintf(stderr, "Error: Invalid queue file %s\n", queue_import_name);
        return -1;
    }
    return 0;
}

int main( int argc, char ** argv )
{
    hb_handle_t * h;

    hb_global_init();
    hb_presets_builtin_update();
    hb_presets_cli_default_init();

    /* Init libhb */
    h = hb_init(4);  // Show all logging until debug level is parsed

    test_tty(); // Terminal detection

    // Get utf8 command line if windows
    get_argv_utf8(&argc, &argv);

    /* Parse command line */
    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        hb_log_level_set(h, debug);
        goto cleanup;
    }

    hb_log_level_set(h, debug);

    /* Register our error handler */
    hb_register_error_handler(&hb_cli_error_handler);

    hb_dvd_set_dvdnav( dvdnav );

    /* Show version */
    fprintf( stderr, "%s - %s - %s\n",
             HB_PROJECT_TITLE, HB_PROJECT_HOST_TITLE, HB_PROJECT_URL_WEBSITE );

    /* Geeky */
    fprintf( stderr, "%d CPU%s detected\n", hb_get_cpu_count(),
             hb_get_cpu_count() > 1 ? "s" : "" );

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT, SigHandler );

    if (queue_import_name != NULL)
    {
        hb_system_sleep_prevent(h);
        RunQueue(h, queue_import_name);
    }
    else
    {
        // Apply all command line overrides to the preset that are possible.
        // Some command line options are applied later to the job
        // (e.g. chapter names, explicit audio & subtitle tracks).
        hb_dict_t *preset_dict = PreparePreset(preset_name);
        if (preset_dict == NULL)
        {
            // An appropriate error message should have already
            // been spilled by PreparePreset.
            done_error = HB_ERROR_WRONG_INPUT;
            goto cleanup;
        }

        if (preset_export_name != NULL)
        {
            hb_dict_set(preset_dict, "PresetName",
                        hb_value_string(preset_export_name));
            if (preset_export_desc != NULL)
            {
                hb_dict_set(preset_dict, "PresetDescription",
                            hb_value_string(preset_export_desc));
            }
            if (preset_export_file != NULL)
            {
                hb_presets_write_json(preset_dict, preset_export_file);
            }
            else
            {
                char *json;
                json = hb_presets_package_json(preset_dict);
                fprintf(stdout, "%s\n", json);
            }
            // If the user requested to export a preset, but not to
            // transcode or scan a file, exit here.
            if (input == NULL ||
                (!titlescan && titleindex != 0 && output == NULL))
            {
                hb_value_free(&preset_dict);
                goto cleanup;
            }
        }

        /* Feed libhb with a DVD to scan */
        fprintf( stderr, "Opening %s...\n", input );

        if (main_feature) {
            /*
             * We need to scan for all the titles in order to
             * find the main feature
             */
            titleindex = 0;
        }

        hb_system_sleep_prevent(h);

        hb_scan(h, input, titleindex, preview_count, store_previews,
                min_title_duration * 90000LL);

        EventLoop(h, preset_dict);
        hb_value_free(&preset_dict);
    }

cleanup:
    /* Clean up */
    hb_close(&h);
    hb_global_close();
    free(input);
    free(output);
    free(preset_name);
    free(preset_export_name);
    free(preset_export_desc);
    free(preset_export_file);
    free(queue_import_name);
    
    OptionsCleanup();
    
    // write a carriage return to stdout
    // avoids overlap / line wrapping when stderr is redirected
    fprintf(stdout, "\n");
    fprintf(stderr, "HandBrake has exited.\n");

    return done_error;
}

static void PrintTitleInfo( hb_title_t * title, int feature )
{
    int i;

    fprintf( stderr, "+ title %d:\n", title->index );
    if ( title->index == feature )
    {
        fprintf( stderr, "  + Main Feature\n" );
    }
    if ( title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE )
    {
        fprintf( stderr, "  + stream: %s\n", title->path );
    }
    else if ( title->type == HB_DVD_TYPE )
    {
        fprintf( stderr, "  + index %d\n", title->index);
    }
    else if( title->type == HB_BD_TYPE )
    {
        fprintf( stderr, "  + playlist: %05d.MPLS\n", title->playlist );
    }
    if (title->angle_count > 1)
        fprintf( stderr, "  + angle(s) %d\n", title->angle_count );
    fprintf( stderr, "  + duration: %02d:%02d:%02d\n",
             title->hours, title->minutes, title->seconds );
    fprintf( stderr, "  + size: %dx%d, pixel aspect: %d/%d, display aspect: %.2f, %.3f fps\n",
             title->geometry.width, title->geometry.height,
             title->geometry.par.num, title->geometry.par.den,
             (float)title->dar.num / title->dar.den,
             (float)title->vrate.num / title->vrate.den );
    fprintf( stderr, "  + autocrop: %d/%d/%d/%d\n", title->crop[0],
             title->crop[1], title->crop[2], title->crop[3] );

    fprintf( stderr, "  + chapters:\n" );
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        hb_chapter_t  * chapter;
        chapter = hb_list_item( title->list_chapter, i );
        fprintf( stderr, "    + %d: duration %02d:%02d:%02d\n",
                 chapter->index, chapter->hours, chapter->minutes,
                 chapter->seconds );
    }
    fprintf( stderr, "  + audio tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        hb_audio_config_t *audio;
        audio = hb_list_audio_config_item( title->list_audio, i );
        if( ( audio->in.codec == HB_ACODEC_AC3 ) || ( audio->in.codec == HB_ACODEC_DCA) )
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s), %dHz, %dbps\n",
                     i + 1,
                     audio->lang.description,
                     audio->lang.iso639_2,
                     audio->in.samplerate,
                     audio->in.bitrate );
        }
        else
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s)\n",
                     i + 1,
                     audio->lang.description,
                     audio->lang.iso639_2 );
        }
    }
    fprintf( stderr, "  + subtitle tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        hb_subtitle_t *subtitle;
        subtitle = hb_list_item( title->list_subtitle, i );
        fprintf(stderr, "    + %d, %s\n", i + 1, subtitle->lang);
    }

    if(title->detected_interlacing)
    {
        /* Interlacing was found in half or more of the preview frames */
        fprintf( stderr, "  + combing detected, may be interlaced or telecined\n");
    }

}

static void PrintTitleSetInfo( hb_title_set_t * title_set )
{
    if (json)
    {
        hb_dict_t * title_set_dict;
        char      * title_set_json;

        title_set_dict = hb_title_set_to_dict(title_set);
        title_set_json = hb_value_get_json(title_set_dict);
        hb_value_free(&title_set_dict);
        fprintf(stdout, "JSON Title Set: %s\n", title_set_json);
        free(title_set_json);
    }
    else
    {
        int i;
        hb_title_t * title;

        for( i = 0; i < hb_list_count( title_set->list_title ); i++ )
        {
            title = hb_list_item( title_set->list_title, i );
            PrintTitleInfo( title, title_set->feature );
        }
    }
}


static void show_progress_json(hb_state_t * state)
{
    hb_dict_t * state_dict;
    char      * state_json;

    state_dict = hb_state_to_dict(state);
    state_json = hb_value_get_json(state_dict);
    hb_value_free(&state_dict);
    fprintf(stdout, "Progress: %s\n", state_json);
    free(state_json);
    fflush(stderr);
}

static int HandleEvents(hb_handle_t * h, hb_dict_t *preset_dict)
{
    hb_state_t s;

    hb_get_state( h, &s );
    switch( s.state )
    {
        case HB_STATE_IDLE:
            /* Nothing to do */
            break;

#define p s.param.scanning
        case HB_STATE_SCANNING:
            /* Show what title is currently being scanned */
            if (json)
            {
                show_progress_json(&s);
                break;
            }
            if (p.preview_cur)
            {
                fprintf(stderr, "%sScanning title %d of %d, preview %d, %.2f %%",
                        stderr_sep, p.title_cur, p.title_count, p.preview_cur, 100 * p.progress);
            }
            else
            {
                fprintf(stderr, "%sScanning title %d of %d, %.2f %%",
                        stderr_sep, p.title_cur, p.title_count, 100 * p.progress);
            }
            fflush(stderr);
            break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_title_set_t * title_set;
            hb_title_t * title;

            if (job_running)
            {
                // SCANDONE generated by a scan during execution of the job
                break;
            }
            title_set = hb_get_title_set( h );
            if( !title_set || !hb_list_count( title_set->list_title ) )
            {
                /* No valid title, stop right there */
                fprintf( stderr, "No title found.\n" );
                done_error = HB_ERROR_WRONG_INPUT;
                die = 1;
                break;
            }
            if (main_feature)
            {
                int i;
                int main_feature_idx=0;
                int main_feature_pos=-1;
                int main_feature_time=0;
                int title_time;

                fprintf( stderr, "Searching for main feature title...\n" );

                for( i = 0; i < hb_list_count( title_set->list_title ); i++ )
                {
                    title = hb_list_item( title_set->list_title, i );
                    title_time = (title->hours*60*60 ) + (title->minutes *60) + (title->seconds);
                    fprintf( stderr, " + Title (%d) index %d has length %dsec\n",
                             i, title->index, title_time );
                    if( main_feature_time < title_time )
                    {
                        main_feature_time = title_time;
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                    }
                    if( title_set->feature == title->index )
                    {
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                        break;
                    }
                }
                if( main_feature_pos == -1 )
                {
                    fprintf( stderr, "No main feature title found.\n" );
                    done_error = HB_ERROR_WRONG_INPUT;
                    die = 1;
                    break;
                }
                titleindex = main_feature_idx;
                fprintf(stderr, "Found main feature title %d\n",
                        main_feature_idx);

                title = hb_list_item(title_set->list_title, main_feature_pos);
            } else {
                title = hb_list_item(title_set->list_title, 0);
            }

            if (!titleindex || titlescan)
            {
                /* Scan-only mode, print infos and exit */
                PrintTitleSetInfo( title_set );
                die = 1;
                break;
            }

            fprintf( stderr, "+ Using preset: %s\n",
                hb_value_get_string(hb_dict_get(preset_dict, "PresetName")));

            PrintTitleInfo(title, title_set->feature);

            // All overrides to the preset are complete
            // Initialize the job from preset + overrides
            // and apply job specific command line overrides
            hb_dict_t *job_dict = PrepareJob(h, title, preset_dict);
            if (job_dict == NULL)
            {
                die = 1;
                return -1;
            }

            char * json_job;
            json_job = hb_value_get_json(job_dict);
            hb_value_free(&job_dict);
            if (json_job == NULL)
            {
                fprintf(stderr, "Error in setting up job! Aborting.\n");
                die = 1;
                return -1;
            }


            hb_add_json(h, json_job);
            free(json_job);
            job_running = 1;
            hb_start( h );
            break;
        }

#define p s.param.working
        case HB_STATE_SEARCHING:
            if (json)
            {
                show_progress_json(&s);
                break;
            }
            fprintf( stdout, "%sEncoding: task %d of %d, Searching for start time, %.2f %%",
                     stdout_sep, p.pass, p.pass_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (ETA %02dh%02dm%02ds)",
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;

        case HB_STATE_WORKING:
            if (json)
            {
                show_progress_json(&s);
                break;
            }
            fprintf( stdout, "%sEncoding: task %d of %d, %.2f %%",
                     stdout_sep, p.pass, p.pass_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (%.2f fps, avg %.2f fps, ETA "
                         "%02dh%02dm%02ds)", p.rate_cur, p.rate_avg,
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;
#undef p

#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            if (json)
            {
                show_progress_json(&s);
                break;
            }
            if (show_mux_warning)
            {
                fprintf( stdout, "%sMuxing: this may take awhile...", stdout_sep );
                fflush(stdout);
                show_mux_warning = 0;
            }
            break;
        }
#undef p

#define p s.param.working
        case HB_STATE_WORKDONE:
            /* Print error if any, then exit */
            if (json)
            {
                show_progress_json(&s);
            }
            switch( p.error )
            {
                case HB_ERROR_NONE:
                    fprintf( stderr, "\nEncode done!\n" );
                    break;
                case HB_ERROR_CANCELED:
                    fprintf( stderr, "\nEncode canceled.\n" );
                    break;
                default:
                    fprintf( stderr, "\nEncode failed (error %x).\n",
                             p.error );
            }
            done_error = p.error;
            work_done = 1;
            job_running = 0;
            break;
#undef p
    }
    return 0;
}

/****************************************************************************
 * SigHandler:
 ****************************************************************************/
static volatile int64_t i_die_date = 0;
void SigHandler( int i_signal )
{
    done_error = HB_ERROR_CANCELED;
    if( die == 0 )
    {
        die = 1;
        i_die_date = hb_get_date();
        fprintf( stderr, "Signal %d received, terminating - do it "
                 "again in case it gets stuck\n", i_signal );
    }
    else if( i_die_date + 500 < hb_get_date() )
    {
        fprintf( stderr, "Dying badly, files might remain in your /tmp\n" );
        exit( done_error );
    }
}

#ifdef __APPLE_CC__
/****************************************************************************
 * bsd_name_for_path
 *
 * Returns the BSD device name for the block device that contains the
 * passed-in path. Returns NULL on failure.
 ****************************************************************************/
static char* bsd_name_for_path(char *path)
{
    const char *prefix = "/dev/";
    struct statfs s;

    if (statfs(path, &s) == -1)
    {
        return NULL;
    }

    size_t lenpre = strlen(prefix),
           lenstr = strlen(s.f_mntfromname);

    if (lenstr > lenpre && strncmp(prefix, s.f_mntfromname, lenpre) == 0)
    {
        return strdup(s.f_mntfromname + lenpre);
    }

    return strdup(s.f_mntfromname);
}

/****************************************************************************
 * device_is_dvd
 *
 * Returns whether or not the passed in BSD device represents a DVD, or other
 * optical media.
 ****************************************************************************/
static int device_is_dvd(char *device)
{
    io_service_t service = get_iokit_service(device);
    if( service == IO_OBJECT_NULL )
    {
        return 0;
    }
    int result = is_dvd_service(service);
    IOObjectRelease(service);
    return result;
}

/****************************************************************************
 * get_iokit_service
 *
 * Returns the IOKit service object for the passed in BSD device name.
 ****************************************************************************/
static io_service_t get_iokit_service( char *device )
{
    CFMutableDictionaryRef matchingDict;
    matchingDict = IOBSDNameMatching( kIOMasterPortDefault, 0, device );
    if( matchingDict == NULL )
    {
        return IO_OBJECT_NULL;
    }
    // Fetch the object with the matching BSD node name. There should only be
    // one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    return IOServiceGetMatchingService( kIOMasterPortDefault, matchingDict );
}

/****************************************************************************
 * is_dvd_service
 *
 * Returns whether or not the service passed in is a DVD.
 *
 * Searches for an IOMedia object that represents the entire (whole) media that
 * the volume is on. If the volume is on partitioned media, the whole media
 * object will be a parent of the volume's media object. If the media is not
 * partitioned, the volume's media object will be the whole media object.
 ****************************************************************************/
static int is_dvd_service( io_service_t service )
{
    kern_return_t  kernResult;
    io_iterator_t  iter;

    // Create an iterator across all parents of the service object passed in.
    kernResult = IORegistryEntryCreateIterator( service,
                                                kIOServicePlane,
                                                kIORegistryIterateRecursively | kIORegistryIterateParents,
                                                &iter );
    if( kernResult != KERN_SUCCESS )
    {
        return 0;
    }
    if( iter == IO_OBJECT_NULL )
    {
        return 0;
    }

    // A reference on the initial service object is released in the do-while
    // loop below, so add a reference to balance.
    IOObjectRetain( service );

    int result = 0;
    do
    {
        if( is_whole_media_service( service ) &&
            IOObjectConformsTo( service, kIODVDMediaClass) )
        {
            result = 1;
        }
        IOObjectRelease( service );
    } while( !result && (service = IOIteratorNext( iter )) );
    IOObjectRelease( iter );

    return result;
}

/****************************************************************************
 * is_whole_media_service
 *
 * Returns whether or not the service passed in is an IOMedia service and
 * represents the "whole" media instead of just a partition.
 *
 * The whole media object is indicated in the IORegistry by the presence of a
 * property with the key "Whole" and value "Yes".
 ****************************************************************************/
static int is_whole_media_service( io_service_t service )
{
    int result = 0;

    if( IOObjectConformsTo( service, kIOMediaClass ) )
    {
        CFTypeRef wholeMedia = IORegistryEntryCreateCFProperty( service,
                                                                CFSTR( kIOMediaWholeKey ),
                                                                kCFAllocatorDefault,
                                                                0 );
        if ( !wholeMedia )
        {
            return 0;
        }
        result = CFBooleanGetValue( (CFBooleanRef)wholeMedia );
        CFRelease( wholeMedia );
    }

    return result;
}
#endif // __APPLE_CC__
