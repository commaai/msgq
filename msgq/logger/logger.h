#pragma once

#include <stdarg.h>

#define CLOUDLOG_DEBUG 10
#define CLOUDLOG_INFO 20
#define CLOUDLOG_WARNING 30
#define CLOUDLOG_ERROR 40
#define CLOUDLOG_CRITICAL 50

#ifdef __GNUC__
#define MSGQ_LOG_CHECK_FMT(a, b) __attribute__ ((format (printf, a, b)))
#else
#define MSGQ_LOG_CHECK_FMT(a, b)
#endif

typedef void (*msgq_logger_callback_t)(int level, const char *file, int line, const char *msg);

#ifdef __cplusplus
extern "C" {
#endif

void msgq_set_logger(msgq_logger_callback_t callback);
void msgq_logv(int level, const char *file, int line, const char *fmt, va_list args) MSGQ_LOG_CHECK_FMT(4, 0);
void msgq_log(int level, const char *file, int line, const char *fmt, ...) MSGQ_LOG_CHECK_FMT(4, 5);

#ifdef __cplusplus
}
#endif

#define LOGD(fmt, ...) msgq_log(CLOUDLOG_DEBUG, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOG(fmt, ...) msgq_log(CLOUDLOG_INFO, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOGW(fmt, ...) msgq_log(CLOUDLOG_WARNING, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOGE(fmt, ...) msgq_log(CLOUDLOG_ERROR, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
