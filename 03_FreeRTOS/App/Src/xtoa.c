#include<stdint.h>
#include "xtoa.h"

/**
 * @brief MISRA-compliant integer to string conversion.
 * @param val Value to convert
 * @param str Buffer to store the result
 * @param len Size of the buffer to prevent overflow (Rule 21.18)
 */
void app_itoa(uint32_t val, char *str, uint32_t len)
{
    uint32_t temp = val;

    /* Handle the '0' case explicitly */
    if (temp == 0U)
    {
        if (len > 1U)
        {
            str[0] = '0';
            str[1] = '\0';
        }
    }
    else
    {
        /* Count digits to find the end of the string */
        uint32_t digits = 0U;
        uint32_t count_temp = temp;
        while (count_temp > 0U)
        {
            digits++;
            count_temp /= 10U;
        }

        /* Check for buffer overflow (Rule 21.18) */
        if (digits < len)
        {
            str[digits] = '\0';
            for (uint32_t i = digits; i > 0U; i--)
            {
                uint32_t digit = ((temp % 10U) + (uint32_t)'0');
                str[i - 1U] = (char)digit;
                temp /= 10U;
            }
        }
    }
}

/**
 * @brief MISRA-compliant float to string conversion (2 decimal places).
 * @param val Floating point value
 * @param str Buffer to store the result
 * @param len Size of the buffer
 */
void app_ftoa(float val, char *str, uint32_t len)
{
    /* 1. Extract the integer part */
    int32_t integer_part = (int32_t)val;

    /* 2. Extract the fractional part (scaled to 2 decimal places) */
    /* Use 100.0f for 2 decimal places, 1000.0f for 3, etc. */
    float diff = val - (float)integer_part;
    uint32_t fractional_part = (uint32_t)(diff * 100.0f);

    /* 3. Convert integer part to string */
    app_itoa((uint32_t)integer_part, str, len);

    /* 4. Find the null terminator to append the decimal point */
    uint32_t i = 0U;
    while ((i < (len - 1U)) && (str[i] != '\0'))
    {
        i++;
    }

    /* 5. Append '.' and fractional part if space permits (Rule 21.18) */
    if (i < (len - 4U)) 
    {
        str[i] = '.';
        i++;
        /* Handle leading zero in fraction (e.g., .05) */
        if (fractional_part < 10U)
        {
            str[i] = '0';
            i++;
        }
        app_itoa(fractional_part, &str[i], len - i);
    }
}