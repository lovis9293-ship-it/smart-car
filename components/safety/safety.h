#ifndef SAFETY_H
#define SAFETY_H
#include "car.h"
#include "hcsr05.h"

#define SAFETY_STOP_CM 20.0f
#define SAFETY_PERIOD_MS 70

typedef struct
{
    HCSR05_typedef sensor;
    Car_typedef *car;
} Safety_typedef;

void safety_init(Safety_typedef *safety, Car_typedef *car,
                 int trig_pin, int echo_pin);
#endif
