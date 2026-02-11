#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H
#include "temp_types.h"

typedef struct AI_Interface AI_Interface;

typedef struct {
    void (*init)(struct AI_Interface *self, ConfigMap *params);
    void (*clean)(AI_Interface *self);
    Move (*get_best_move)(AI_Interface *self, GameState state);
} AI_VTable;

struct AI_Interface {
    const AI_VTable *vtable;
    void *context;
};

#endif