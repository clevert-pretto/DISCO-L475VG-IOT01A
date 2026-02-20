#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#define LOGGER_MESSAGE_STR_LEN          64

typedef enum
{
    sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE = 0,
    sAPPLOGGER_EVENT_CODE_LOG_FAULT,
    sAPPLOGGER_EVENT_CODE_LOG_ERROR,
    sAPPLOGGER_EVENT_CODE_LOG_WARNING,
    
    sAPPLOGGER_EVENT_CODE_TOTAL
} sAppLoggerEventCode_t;


typedef struct
{
    char pcMessage[LOGGER_MESSAGE_STR_LEN];
    sAppLoggerEventCode_t enumEventCode;
} sAppLoggerMessage_t;

void appLogger_Init(void);

void appLoggerMessageEntry( const char *pcMessage, 
                            sAppLoggerEventCode_t enumEventCodes);
void vAppLoggerTask(void *pvParameters);

#endif // APP_LOGGER_H