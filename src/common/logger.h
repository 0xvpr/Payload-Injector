#ifndef   LOGGER_HEADER
#define   LOGGER_HEADER

#include  "common/definitions.h"

#ifndef   LOG_MSG
#define   LOG_MSG(injector, msg, shiftwidth) \
          injector->logger && injector->logger(injector, msg, shiftwidth)
#endif // LOG_MSG

int32_t log_basic(const parsed_args_t* args, const char * event, uint32_t shiftwidth);
int32_t log_advanced(const parsed_args_t* args, const char* event, uint32_t shiftwidth);

static const logger_t loggers[3] = {
    NULL,
    log_basic,
    log_advanced
};

#endif /* LOGGER_HEADER */
