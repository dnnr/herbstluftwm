#include "xkeygrabber.h"

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "globals.h"

void XKeyGrabber::updateNumlockMask() {
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockMask_ = 0;
    modmap = XGetModifierMapping(g_display);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(g_display, XK_Num_Lock))
                numlockMask_ = (1 << i);
    XFreeModifiermap(modmap);
}

/*!
 * Derives a "normalized" KeyCombo from a given event.
 *
 * Normalization means stripping any ignored modifiers from modifier mask
 * (including the runtime-defined Numlock mask).
 */
KeyCombo XKeyGrabber::xEventToKeyCombo(XEvent *ev) {
    KeyCombo combo;
    combo.keysym = XkbKeycodeToKeysym(g_display, ev->xkey.keycode, 0, 0);
    combo.modifiers = ev->xkey.state;

    // Normalize
    combo.modifiers &= ~(numlockMask_ | LockMask);

    return combo;
}

//! Grabs the given key combo
void XKeyGrabber::grabKeyCombo(const KeyCombo& keyCombo) {
    changeGrabbedState(keyCombo, true);
}

//! Ungrabs the given key combo
void XKeyGrabber::ungrabKeyCombo(const KeyCombo& keyCombo) {
    changeGrabbedState(keyCombo, false);
}

//! Removes all grabbed keys (without knowing them)
void XKeyGrabber::ungrabAll() {
    XUngrabKey(g_display, AnyKey, AnyModifier, g_root);
}

void XKeyGrabber::changeGrabbedState(const KeyCombo& keyCombo, bool grabbed) {
    // List of ignored modifiers (key combo needs to be grabbed for each of
    // them):
    const unsigned int ignModifiers[] = { 0, LockMask, numlockMask_, numlockMask_ | LockMask };

    KeyCode keycode = XKeysymToKeycode(g_display, keyCombo.keysym);
    if (!keycode) {
        // ignore unknown keysyms
        return;
    }
    // grab/ungrab key for each modifier that is ignored (capslock, numlock)
    for (auto& ignModifier : ignModifiers) {
        if (grabbed) {
            HSDebug("XKeyGrabber: ungrabbing %s for mod %i\n", keyCombo.origstr.c_str(), ignModifier);
            XGrabKey(g_display, keycode, ignModifier | keyCombo.modifiers, g_root,
                    True, GrabModeAsync, GrabModeAsync);
        } else {
            HSDebug("XKeyGrabber: ungrabbing %s for mod %i\n", keyCombo.origstr.c_str(), ignModifier);
            XUngrabKey(g_display, keycode, ignModifier | keyCombo.modifiers, g_root);
        }
    }
}
