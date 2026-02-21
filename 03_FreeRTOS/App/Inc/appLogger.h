#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#pragma pack(1)

#define LOGGER_MESSAGE_STR_LEN                  128u
#define STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE    0x11223344U
#define LOG_EVENT_SECTOR_VERSION                1u

//#define LOG_SECTOR_SIZE         MX25R6435F_SECTOR_SIZE
#define MAX_LOG_EVENTS          255U
#define LOG_ENTRY_SIZE          sizeof(sStorageEvent_t)
#define LOG_HEADER_SIZE         sizeof(sLogSectorHeader_t) //16 = Signature + Definition
#define LOG_PARTITION_START     0U
#define LOG_DATA_START          LOG_HEADER_SIZE
#define LOG_PARTITION_END       (LOG_DATA_START + (MAX_LOG_EVENTS * LOG_ENTRY_SIZE))

#define LOG_MESSAGE_SIZE        sizeof(sAppLoggerMessage_t)
typedef enum
{
    sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE = 0,
    sAPPLOGGER_EVENT_CODE_LOG_INFO,
    sAPPLOGGER_EVENT_CODE_LOG_FAULT,
    sAPPLOGGER_EVENT_CODE_LOG_ERROR,
    sAPPLOGGER_EVENT_CODE_LOG_WARNING,
    
    sAPPLOGGER_EVENT_CODE_TOTAL
} sAppLoggerEventCode_t;

typedef enum {
    /* System Events */
    EVENT_ID_SYS_BOOT           = 0x1001U,
    EVENT_ID_SYS_FAULT          = 0x1002U,
    
    /* Sensor Events */
    EVENT_ID_SENSOR_READ_FAIL   = 0x2001U,
    EVENT_ID_SENSOR_DATA_POINT  = 0x2002U,
    EVENT_ID_T_SENSOR_DATA_POINT  = 0x2003U,
    EVENT_ID_H_SENSOR_DATA_POINT  = 0x2004U,
    
    /* Storage Events (Phase 3-M3) */
    EVENT_ID_QSPI_INIT_SUCCESS  = 0x3001U,
    EVENT_ID_QSPI_ERASE_BEGIN   = 0x3002U
} eEventID_t;

typedef struct __attribute__((packed))
{
    uint32_t timestamp;   /* Result of xTaskGetTickCount() */
    eEventID_t eventID;     /* e.g., 0x1001 for Boot, 0x2001 for sensor Data */
    uint16_t taskID;      /* Which task is writing? */
    uint32_t payload[2];  /* Raw data (e.g., Temp, Error Code) */
} sStorageEvent_t;

typedef struct __attribute__((packed))
{
    char pcMessage[LOGGER_MESSAGE_STR_LEN];
    sAppLoggerEventCode_t enumEventCode;
} sAppLoggerMessage_t;

typedef struct __attribute__((packed))
{
    uint32_t magicSignature;
    uint16_t version;
    uint16_t maxEvents;
    uint32_t eraseCount;
    uint32_t reserved;
} sLogSectorHeader_t;

void appLogger_Init(void);

uint8_t appLogger_storage_Init(void);

void appLoggerMessageEntry( const char *pcMessage, 
                            sAppLoggerEventCode_t enumEventCodes);

void appLoggerEventEntry(const sStorageEvent_t *sEvent);

void vAppLoggerTask(void *pvParameters);

void vCommandTask(void *pvParameters);
#endif // APP_LOGGER_H