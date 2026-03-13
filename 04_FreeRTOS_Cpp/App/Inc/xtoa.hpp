#ifndef XTOA_H
#define XTOA_H

#include <stdint.h>

namespace FreeRTOS_Cpp
{
    class xtoa
    {
        public:
            xtoa(void);     
            static void app_itoa(int32_t val, char *str, uint32_t len);
            static void app_ftoa(float val, char *str, uint32_t len);

    };
}
#endif //XTOA_H