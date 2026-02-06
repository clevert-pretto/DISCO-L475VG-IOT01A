#include "my_stm32_map.h"


void iwdg_init(void)
{
    IWDG->KR = 0xCCCC;  //Start IWDG
    IWDG->KR = 0x5555;  // Enable register access
    IWDG->PR = 0x4;     // Prescalar /64
    IWDG->RLR = 1000;   // Reload value
    while (IWDG->SR != 0);    //Wait for register to update
    IWDG->KR = 0xAAAA;      //Refresh
}

void iwdg_feed(void)
{
    IWDG->KR = 0xAAAA; //Kick the dog
}