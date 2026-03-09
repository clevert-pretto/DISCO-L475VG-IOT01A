#ifndef XTOA_H
#define XTOA_H

#include <stdint.h>

class xtoa
{
    public:
        xtoa();     
        static void app_itoa(uint32_t val, char *str, uint32_t len);
        static void app_ftoa(float val, char *str, uint32_t len);

};

#endif //XTOA_H