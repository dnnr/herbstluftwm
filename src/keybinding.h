#pragma once

#include "keycombo.h"

/*!
 * Simple container class for tracking a keybinding
 */
class KeyBinding {
public:
    KeyCombo keyCombo;

    //! Command to call
    std::vector<std::string> cmd;

    //! Whether this binding is currently grabbed
    bool grabbed;
};
