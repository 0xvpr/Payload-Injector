/**
 * Created by:  VPR
 * Created:     September 14th, 2021
 *
 * Updated by:  VPR
 * Updated:     January 15th, 2025
 *
 * Description: Payload injection tool.
**/

#include   "pidjeon/parser.h"

#include   "common/logger.h"
#include   "common/util.h"

#include   "inject/loadlibrary.h"

#include   <stdio.h>

static inline
int __handle_status(enum status_t inject_code)
{
    switch (inject_code)
    {
        case status_incorrect_parameters:
        {
            fprintf(stderr, "Error code %d: Incorrect Paramters.\n", status_incorrect_parameters);
            break;
        }
        case status_process_not_running:
        {
            fprintf(stderr, "Error code %d: Process is not running.\n", status_process_not_running);
            break;
        }
        case status_dll_does_not_exist:
        {
            fprintf(stderr, "Error code %d: DLL does not exist.\n", status_dll_does_not_exist);
            break;
        }
        case status_injection_failed:
        {
            fprintf(stderr, "Error code %d: Injection Failed.\n", status_dll_does_not_exist);
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

static inline
int __handle_error(enum errcode_t err)
{
    switch (err)
    {
        case err_invalid_arguments:
        {
            fprintf(stderr, "Error code %d: Incorrect Paramters.\n", err_invalid_arguments);
            break;
        }
        case err_missing_arguments:
        {
            fprintf(stderr, "Error code %d: Process is not running.\n", err_missing_arguments);
            break;
        }
        case err_unsupported_arguments:
        {
            fprintf(stderr, "Error code %d: DLL does not exist.\n", err_unsupported_arguments);
            break;
        }
        case err_parse_error:
        {
            fprintf(stderr, "Error code %d: Injection Failed.\n", err_parse_error);
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

static inline
void init_parsed_args(parsed_args_t* args, const char* argv0)
{
    if (!args || !argv0)
    {
        __handle_error(err_parse_error);
    }

    args->process_id = 0;
    args->verbosity = 0;
    args->stealth = 0;
    args->delay = 0;

    memcpy(args->program_name, argv0, sizeof(args->program_name)-1);
    memcpy(args->log_path, "%%APPDATA%%/log.txt", sizeof("%%APPDATA%%/log.txt"));
    memset(args->process_name, 0, sizeof(args->process_name));
    memset(args->payload_path, 0, sizeof(args->payload_path));

    args->logger = NULL;
    args->operation = (enum status_t (*)(const struct _parsed_args_t* args))load_library_a;
}


int main(int argc, char** argv)
{
    parsed_args_t args = { 0 };
    init_parsed_args(&args, argv[0]);

    enum errcode_t err = parse_arguments(argc, argv, &args);
    err && __handle_error(err);

    err = validate_arguments(&args); 
    err && __handle_error(err);

    if (args.delay)
    {
        fprintf(stdout, "Delay(ms): %llu\n\n", args.delay);
        Sleep((DWORD)args.delay);
    }

    enum status_t status = args.operation(&args);
    status != status_success && __handle_status(status);

    char buffer[64] = { 0 };
    snprintf(buffer, sizeof(buffer)-1, "Injection: %s", status ? "failed" : "successful");
    LOG_MSG((&args), buffer, 0);


    return 0;
}
