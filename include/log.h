#ifndef _LOG_H_
#define _LOG_H_
#include <string.h>

typedef enum E_LOG_LEVEL
{
    LOG_DEFINE = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARNING = 3,
    LOG_ERROR = 4,
    LOG_CRITICAL = 5,
}eLogLevel;

#define LOG_ERR_OK    (0)
#define LOG_ERR_FAIL    (-1)

#define filename(x) strrchr(x, '/')?strrchr(x, '/')+1:x

void log_core_printf(eLogLevel level, const char* format, ...);

/*各级别日志对外宏*/
#define LOG_DEBUG(fmt, ...) \
    log_core_printf(LOG_DEBUG, "%s(%d):%s:"fmt, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define LOG_INFO(fmt, ...) \
    log_core_printf(LOG_INFO, "%s(%d):%s:"fmt, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define LOG_WARNING(fmt, ...) \
    log_core_printf(LOG_WARNING, "%s(%d):%s:"fmt, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define LOG_ERROR(fmt, ...) \
    log_core_printf(LOG_ERROR, "%s(%d):%s:"fmt, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define LOG_CRITICAL(fmt, ...) \
    log_core_printf(LOG_CRITICAL, "%s(%d):%s:"fmt, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);

/*统一对外接口*/
void log_init(char *config_file);
void log_destory();

#endif
