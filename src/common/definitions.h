#ifndef   DEFINITIONS_HEADER
#define   DEFINITIONS_HEADER

#include  <windows.h>
#include  <stdint.h>

enum status_t
{
    status_success              = 0,
    status_incorrect_parameters = 100,
    status_process_not_running  = 101,
    status_dll_does_not_exist   = 102,
    status_injection_failed     = 103,
    status_method_not_found     = 104
};

enum operation_t
{
    op_no_operation             = 0,
    op_load_library_a           = 1,
    op_load_library_w           = 2,
    op_manual_map               = 3,
    op_crt                      = 4,
};

enum machine_t
{
    machine_type_err            = -1,
    machine_unknown             =  0,
    machine_x86                 =  1,
    machine_x64                 =  2
};

enum errcode_t
{
    err_no_error,
    err_missing_arguments,
    err_invalid_arguments,
    err_unsupported_arguments,
    err_parse_error
};

typedef struct _parsed_args_t
{
    DWORD                       process_id;
    int32_t                     verbosity;
    int32_t                     stealth;
    uint64_t                    delay;
    char                        program_name[MAX_PATH];
    char                        process_name[MAX_PATH];
    char                        payload_path[MAX_PATH];
    char                        log_path[MAX_PATH];
    int32_t                     (* logger)(const struct _parsed_args_t* args, const char* event, uint32_t shiftwidth);
    enum status_t               (* operation)(const struct _parsed_args_t* args);
} parsed_args_t;

typedef int (* logger_t)(const parsed_args_t*, const char*, uint32_t);
typedef void* exec_mem_t;

#endif // DEFINITIONS_HEADER
