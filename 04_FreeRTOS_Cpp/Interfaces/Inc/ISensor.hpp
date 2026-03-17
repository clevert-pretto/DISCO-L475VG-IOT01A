#ifndef ISENSOR_HPP
#define ISENSOR_HPP

namespace FreeRTOS_Cpp {
    class ISensor {
    public:
        virtual ~ISensor() = default;
        virtual bool init() = 0;
        virtual float read() = 0;
    };
}

#endif