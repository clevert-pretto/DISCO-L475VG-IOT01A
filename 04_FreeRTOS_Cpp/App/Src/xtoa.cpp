#include <stdint.h>
#include <stdbool.h>
#include "xtoa.hpp"

namespace FreeRTOS_Cpp {
    
    void xtoa::app_itoa(int32_t val, char *str, uint32_t len) 
    {
        if (len < 2U) 
            return; // Buffer too small for even a single digit + null

        uint32_t uval;
        uint32_t pos = 0;

        // 1. Handle Negative Sign
        if (val < 0) 
        {
            str[pos++] = '-';
            uval = static_cast<uint32_t>(-val);
        } 
        else 
        {
            uval = static_cast<uint32_t>(val);
        }

        // 2. Handle '0' explicitly
        if (uval == 0U) 
        {
            if (pos < (len - 1U)) 
            {
                str[pos++] = '0';
                str[pos] = '\0';
            }
            return;
        }

        // 3. Calculate digits for the absolute value
        uint32_t digits = 0;
        uint32_t temp = uval;
        while (temp > 0U) 
        {
            digits++;
            temp /= 10U;
        }

        // 4. Check if total string fits: (sign? + digits + null)
        if ((pos + digits) < len) 
        {
            str[pos + digits] = '\0';
            for (uint32_t i = digits; i > 0U; i--) 
            {
                str[pos + i - 1U] = static_cast<char>((uval % 10U) + '0');
                uval /= 10U;
            }
        }
    }

    void xtoa::app_ftoa(float val, char *str, uint32_t len) {
        if (len < 6U) return; // Need space for "0.00\0"

        // 1. Handle Negative Floats
        if (val < 0.0f) 
        {
            str[0] = '-';
            // Recursively call with positive value, shifting buffer by 1
            app_ftoa(-val, &str[1], len - 1U);
            return;
        }

        // 2. Extract Parts
        int32_t integer_part = static_cast<int32_t>(val);
        // Rounding to 2 decimal places: multiply by 100 and add 0.5 for rounding
        uint32_t fractional_part = static_cast<uint32_t>((val - static_cast<float>(integer_part)) * 100.0f + 0.5f);

        // Handle case where fractional rounding rolls over (e.g., 3.999 -> 4.00)
        if (fractional_part >= 100U) {
            integer_part++;
            fractional_part = 0U;
        }

        // 3. Convert Integer Part
        app_itoa(integer_part, str, len);

        // 4. Find end of integer part
        uint32_t i = 0;
        while ((i < (len - 1U)) && (str[i] != '\0')) { i++; }

        // 5. Append Decimal and Fraction
        if (i < (len - 4U)) {
            str[i++] = '.';
            str[i++] = static_cast<char>((fractional_part / 10U) + '0');
            str[i++] = static_cast<char>((fractional_part % 10U) + '0');
            str[i] = '\0';
        }
    }
}