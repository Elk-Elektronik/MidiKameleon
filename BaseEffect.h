#ifndef BASE_EFFECT_H
#define BASE_EFFECT_H

#include "Globals.h"

class BaseEffect {
public:
    virtual void process(State_t *state) = 0;
    virtual void handleClock() = 0;
    virtual ~BaseEffect() {}
};

#endif // BASE_EFFECT_H