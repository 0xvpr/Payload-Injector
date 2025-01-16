#include "pidjeon/parser.h"

#include "common/definitions.h"
#include "common/logger.h"
#include "common/util.h"

#include "inject/loadlibrary.h"
#include "inject/manual_map.h"
#include "inject/crt.h"

#include <windows.h>
#include <conio.h>

#include <stdio.h>
#include <getopt.h>

static const struct option long_options[] = {
    { "delay",          required_argument,  0,  'd' },
    { "inject-method",  required_argument,  0,  'i' },
    { "output-file",    required_argument,  0,  'o' },
    { "stealth",        optional_argument,  0,  's' },
    { "verbosity",      optional_argument,  0,  'v' },
};

static inline
void get_cursor_position(short* x, short* y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    *x = csbi.dwCursorPosition.X;
    *y = csbi.dwCursorPosition.Y;
}

static inline
void goto_xy(short x, short y)
{
    COORD coord = { .X = x, .Y = y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

static inline
void print_options(const DWORD* processes, const size_t size, size_t selected_option, short x_start, short y_start)
{
    for (size_t i = 0; i < size; ++i)
    {
        goto_xy(x_start, y_start + (short)(i & 0xFFFF));
        if (i == selected_option)
        {
            fprintf(stdout, "[+] %lu\n",  processes[i]);
        }
        else
        {
            fprintf(stdout, "[ ] %lu\n",  processes[i]);
        }
    }
}

static inline
uint32_t select_process(DWORD* processes, const size_t size)
{
    size_t selected_option = 0;

    if (size == 1)
    {
        return processes[0];
    }

    fprintf(stdout, "Multiple processed found.\n Select process (press Enter|Space|Escape to select):\n\n");

    // Get initial cursor position
    short x_start, y_start;
    get_cursor_position(&x_start, &y_start);

    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo = { 0 };
    cursorInfo.bVisible = 0;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    print_options(processes, size, selected_option, x_start, y_start);

    while (1)
    {
        // Print options
        print_options(processes, size, selected_option, x_start, y_start);

        // Handle input
        int key = _getch();
        if (key == VK_RETURN || key == VK_SPACE || key == VK_ESCAPE) // Enter|| Space || Escape  
        {
            break;
        }
        if (key == 'j')
        {
            selected_option = (selected_option - 1 + size) % size;
        }
        if (key == 'k')
        {
            selected_option = (selected_option + 1) % size;
        }
        else if (key == 224) // Arrow key
        {
            key = _getch();
            switch (key) {
                case 72: // Up arrow
                {
                    selected_option = (selected_option - 1 + size) % size;
                    break;
                }
                case 80: // Down arrow
                {
                    selected_option = (selected_option + 1) % size;
                    break;
                }
            }
        }
    }

    // Show cursor before exiting
    cursorInfo.bVisible = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    return processes[selected_option];
}

enum errcode_t parse_arguments(int argc, char** argv, parsed_args_t* args)
{
    if (argc < 3) {
        usage("Not enough arguments provided", args);
    }
    else
    {
        size_t argv0_length = strlen(argv[0]);
        size_t progam_name_length = sizeof(args->program_name);
        size_t min_length = progam_name_length < argv0_length ? progam_name_length-1 : argv0_length;
        strncpy(args->program_name, argv[0], min_length);
    }

    int opt = 0;
    int long_index = 0;
    enum errcode_t err = err_no_error;
    while ( (opt = getopt_long(argc, argv, "d:i:o:sv", long_options, &long_index)) != -1 )
    {
        switch (opt)
        {
            case 'd':
            {
                if (sscanf(optarg, "%llu", &(args->delay)) != 1)
                {
                    usage("Failed to read argument", args);
                    err = err_invalid_arguments;
                    return err;
                }
                break;
            }
            case 'i':
            {
                if (*optarg == '=')
                {
                    ++optarg;
                }
                size_t optarg_length = strlen(optarg);

                if ( (optarg_length == strlen("loadlibrarya") && !stricmp("loadlibrarya", optarg)) ||
                     (optarg_length == strlen("lla") && !stricmp("lla", optarg)) )
                {
                    args->operation = load_library_a;
                    break;
                }
                if ( (optarg_length == strlen("loadlibraryw") && !stricmp("loadlibraryw", optarg)) ||
                     (optarg_length == strlen("llw") && !stricmp("llw", optarg)) )
                {
                    args->operation = load_library_w;
                    break;
                }
                if ( (optarg_length == strlen("manualmap") && !stricmp("manualmap", optarg)) ||
                     (optarg_length == strlen("mm") && !stricmp("mm", optarg)) )
                {
                    args->operation = manual_map;
                    break;
                }
                if ( (optarg_length == strlen("createremotethread") && !stricmp("createremotethread", optarg)) ||
                     (optarg_length == strlen("crt") && !stricmp("crt", optarg)) )
                {
                    args->operation = create_remote_thread;
                    break;
                }
                usage("Invalid injection method", args);
                return err_invalid_arguments;
                break;
            }
            case 'o':
            {
                size_t optarg_length = strlen(optarg);
                size_t log_path_length = sizeof(args->log_path);
                size_t min_length = log_path_length < optarg_length ? log_path_length-1 : optarg_length;
                strncpy(args->log_path, optarg, min_length);
                break;
            }
            case 's':
            {
                ++args->stealth;
                break;
            }
            case 'v':
            {
                ++args->verbosity;
                break;
            }
            case '?':
            {
                break;
            }
            default:
            {
                usage("Unknown option", args);
                break;
            }
        }
    }

    if (optind >= argc)
    {
        usage("Positional arguments missing", args);
        return err_invalid_arguments;
    }

    DWORD processes[25] = { 0 };
    size_t processes_size = sizeof(processes) / sizeof(processes[0]);

    int process_id_set = 0;
    int payload_path_set = 0;
    while ( optind < argc )
    {
        if (!process_id_set)
        {
            DWORD process_id = 0;
            if (sscanf(argv[optind], "%lu", &process_id) == 1)
            {
                args->process_id = process_id;
                if ( get_process_name_by_process_id(args->process_id, args->process_name, sizeof(args->process_name)) )
                {
                    ++optind;
                    process_id_set = 1;
                }

                continue;
            }

            size_t n_processes = get_process_ids_by_process_name(argv[optind], processes, processes_size);
            if (n_processes)
            {
                size_t argv_length = strlen(argv[optind]);
                strncpy(args->process_name, argv[optind], sizeof(args->process_name) < argv_length ? sizeof(args->process_name)-1 : argv_length);
                args->process_id = select_process(processes, n_processes);

                process_id_set = 1;
                ++optind;
            }
        }

        if (!payload_path_set && file_exists(argv[optind]))
        {
            size_t optind_length = strlen(argv[optind]);
            strncpy(args->payload_path, argv[optind], sizeof(args->payload_path) < optind_length ? sizeof(args->payload_path)-1 : optind_length);

            payload_path_set = 1;
        }
        ++optind;
    }

    if (!args->process_id || !file_exists(args->payload_path)) {
        usage("Positional arguments missing", args);
        return err_invalid_arguments;
    }

    // truncation/validation
    if (args->verbosity > 2)
    {
        args->verbosity = 2;
    }

    if (args->stealth > 2)
    {
        args->stealth = 2;
    }

    args->logger = loggers[args->verbosity];

    fprintf(stdout, "Selected process: %lu.\n", args->process_id);
    return err_no_error;
}
