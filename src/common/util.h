#ifndef   UTIL_HEADER
#define   UTIL_HEADER

#include  "common/definitions.h"

void usage(const char* const restrict error_message, parsed_args_t* parsed_args);

size_t get_process_ids_by_process_name(const char* const restrict process_name, DWORD* restrict processes, size_t processes_size);

int get_process_name_by_process_id(DWORD process_id, char* buffer, size_t buffer_size);

enum machine_t get_machine_type(const HANDLE process_handle);

int file_exists(const char* restrict filename);

enum errcode_t validate_arguments(parsed_args_t* parsed_args);

#endif /* UTIL_HEADER */
