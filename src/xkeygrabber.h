#pragma once

#include <regex>
#include <X11/Xlib.h>

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

    KeyCombo xEventToKeyCombo(XEvent *ev);

    void grabKeyCombo(const KeyCombo& keyCombo);
    void ungrabKeyCombo(const KeyCombo& keyCombo);
    void ungrabAll();

private:
    void changeGrabbedState(const KeyCombo& keyCombo, bool grabbed);
    unsigned int numlockMask = 0;

    void updateNumlockMask();
};

