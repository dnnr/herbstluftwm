#pragma once

#include <regex>

#include "keycombo.h"

/*!
 * Handles the actual grabbing/releasing of given key combinations.
 *
 * Expects to be notified about keyboard mapping changes so that it can keep
 * track of the current numlock mask value.
 */
class XKeyGrabber {
public:
    XKeyGrabber() {
        updateNumlockMask();
    }

    void updateKeyboardMapping() {
        updateNumlockMask();
    }

    void grabKeyCombo(const KeyCombo& keyCombo);
    void ungrabKeyCombo(const KeyCombo& keyCombo);
private:
    void changeGrabbedState(const KeyCombo& keyCombo, bool grabbed);
    unsigned int numlockMask = 0;

    void updateNumlockMask();
};

