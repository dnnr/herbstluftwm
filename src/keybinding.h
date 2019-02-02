#pragma once

#include "keycombo.h"

class KeyBinding {
public:
    KeyCombo keyCombo;

    //! Command to call
    std::vector<std::string> cmd;

    // TODO: Rename this to "grabbed"
    bool    enabled;  // Is the keybinding already grabbed
};
