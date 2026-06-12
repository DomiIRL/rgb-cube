#pragma once
#include "Cube.h"

class Mode {
public:
    virtual ~Mode() = default;
    virtual void update(Cube& cube, uint32_t ms) = 0;

    virtual void onEnter(Cube& cube) {
        cube.clear();
        cube.show();
    }

    virtual void onExit(Cube& cube) {
    }
};
