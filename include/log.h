#ifndef _LOG_H_
#define _LOG_H_

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

void log_core_printf(eLogLevel level, const char* format, ...);

/*各级别日志对外宏*/
#define LOG_DEBUG(fmt, ...) \
    log_core_printf(LOG_DEBUG, "%s:%s(%d):"fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define LOG_INFO(fmt, ...) \
    log_core_printf(LOG_INFO, "%s:%s(%d):"fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define LOG_WARNING(fmt, ...) \
    log_core_printf(LOG_WARNING, "%s:%s(%d):"fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define LOG_ERROR(fmt, ...) \
    log_core_printf(LOG_ERROR, "%s:%s(%d):"fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define LOG_CRITICAL(fmt, ...) \
    log_core_printf(LOG_CRITICAL, "%s:%s(%d):"fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

/*统一对外接口*/
void log_init();
void log_destory();

/*内部接口,以后要移除，不对外暴露*/
void log_core_init();
void log_core_destroy();

#endif
