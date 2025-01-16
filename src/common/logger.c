#include "common/logger.h"
#include "common/util.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int32_t log_basic(const parsed_args_t* args, const char* event, uint32_t shiftwidth)
{
    (void)args;
    int bytes_written = 0;

    time_t rawtime = 0;
    struct tm info = { 0 };

    time(&rawtime);
    localtime_s(&info, &rawtime);

    char prefix[64] = { 0 };
    strftime(prefix, sizeof(prefix)-1, "[%Y-%m-%d %H:%M:%S]:", &info);
    
    while (shiftwidth--) bytes_written += fputc(' ', stdout);
    bytes_written += fprintf(stdout, "%s %s.\n", prefix, event);

    return bytes_written;
}

int32_t log_advanced(const parsed_args_t* args, const char* event, uint32_t shiftwidth)
{
    int bytes_written = 0;

    FILE* fp = NULL;
    if ( !file_exists(args->log_path) &&
        (!fopen_s(&fp, args->log_path, "wb")) )
    {
        time_t rawtime = 0;
        struct tm info = { 0 };

        time(&rawtime);
        localtime_s(&info, &rawtime);

        char prefix[64] = { 0 };
        strftime(prefix, sizeof(prefix)-1, "[%Y-%m-%d %H:%M:%S]:", &info);
        
        while (shiftwidth--) fputc(' ', fp);
        bytes_written += fprintf(fp, "%s Log file created.\n", prefix);
        fclose(fp);
    }
    else if (!fopen_s(&fp, args->log_path, "ab"))
    {
        time_t rawtime = 0;
        struct tm info = { 0 };

        time(&rawtime);
        localtime_s(&info, &rawtime);

        char prefix[64] = { 0 };
        strftime(prefix, sizeof(prefix)-1, "[%Y-%m-%d %H:%M:%S]:", &info);
        
        while (shiftwidth--) fputc(' ', fp);
        bytes_written += fprintf(fp, "%s %s.\n", prefix, event);
        fclose(fp);
    } 

    return bytes_written;
}
