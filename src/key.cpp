#include "key.h"

#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>

#include "client.h"
#include "command.h"
#include "glib-backports.h"
#include "globals.h"
#include "ipc-protocol.h"
#include "keycombo.h"
#include "utils.h"

using std::string;
using std::unique_ptr;
using std::vector;

static unsigned int numlockmask = 0;
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask))

unsigned int* get_numlockmask_ptr() {
    return &numlockmask;
}

// TODO: Turn this into a private member of KeyManager as soon as possible.
vector<unique_ptr<KeyBinding>> g_key_binds = {};

void key_init() {
    update_numlockmask();
}

void key_destroy() {
    key_remove_all_binds();
}

void key_remove_all_binds() {
    g_key_binds.clear();
    regrab_keys();
}

static gint keysym_equals(const KeyCombo* a, const KeyCombo* b) {
    bool equal = (CLEANMASK(a->modifiers) == CLEANMASK(b->modifiers));
    equal = equal && (a->keysym == b->keysym);
    return equal ? 0 : -1;
}

void handle_key_press(XEvent* ev) {
    KeyCombo pressed;
    pressed.keysym = XkbKeycodeToKeysym(g_display, ev->xkey.keycode, 0, 0);
    pressed.modifiers = ev->xkey.state;
    auto found = std::find_if(g_key_binds.begin(), g_key_binds.end(),
            [=](const unique_ptr<KeyBinding> &other) {
                return keysym_equals(&pressed, &(other->keyCombo)) == 0;
                });
    if (found != g_key_binds.end()) {
        // call the command
        std::ostringstream discardedOutput;
        auto& cmd = (*found)->cmd;
        Input input(cmd.front(), {cmd.begin() + 1, cmd.end()});
        Commands::call(input, discardedOutput);
    }
}

bool key_remove_bind_with_keysym(unsigned int modifiers, KeySym keysym){
    KeyCombo combo;
    combo.modifiers = modifiers;
    combo.keysym = keysym;
    // search this keysym in list and remove it
    for (auto iter = g_key_binds.begin(); iter != g_key_binds.end(); iter++) {
        if (keysym_equals(&combo, &((*iter)->keyCombo)) == 0) {
            g_key_binds.erase(iter);
            return true;
        }
    }
    return false;
}

void regrab_keys() {
    update_numlockmask();
    // init modifiers after updating numlockmask
    XUngrabKey(g_display, AnyKey, AnyModifier, g_root); // remove all current grabs
    for (auto& binding : g_key_binds) {
        grab_keybind(binding.get());
    }
}

void grab_keybind(KeyBinding* binding) {
    unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
    KeyCode keycode = XKeysymToKeycode(g_display, binding->keyCombo.keysym);
    if (!keycode) {
        // ignore unknown keysyms
        return;
    }
    // grab key for each modifier that is ignored (capslock, numlock)
    for (int i = 0; i < LENGTH(modifiers); i++) {
        XGrabKey(g_display, keycode, modifiers[i]|binding->keyCombo.modifiers, g_root,
                 True, GrabModeAsync, GrabModeAsync);
    }
    // mark the keybinding as enabled
    binding->enabled = true;
}

// update the numlockmask
// from dwm.c
void update_numlockmask() {
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(g_display);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(g_display, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

// STRTODO
struct key_find_context {
    Output      output;
    const char* needle;
    size_t      needle_len;
};

static void key_find_binds_helper(KeyBinding* b, struct key_find_context* c) {
    auto name = b->keyCombo.str();
    if (name.find(c->needle) == 0) {
        /* add to output if key starts with searched needle */
        c->output << name << std::endl;
    }
}

void key_find_binds(const char* needle, Output output) {
    struct key_find_context c = {
        output, needle, strlen(needle)
    };
    for (auto& binding : g_key_binds) {
        key_find_binds_helper(binding.get(), &c);
    }
}

void complete_against_keysyms(const char* needle, char* prefix, Output output) {
    // get all possible keysyms
    int min, max;
    XDisplayKeycodes(g_display, &min, &max);
    int kc_count = max - min + 1;
    int ks_per_kc; // count of keysysms per keycode
    KeySym* keysyms;
    keysyms = XGetKeyboardMapping(g_display, min, kc_count, &ks_per_kc);
    // only symbols at a position i*ks_per_kc are symbols that are recieved in
    // a keyevent, it should be the symbol for the keycode if no modifier is
    // pressed
    for (int i = 0; i < kc_count; i++) {
        if (keysyms[i * ks_per_kc] != NoSymbol) {
            char* str = XKeysymToString(keysyms[i * ks_per_kc]);
            try_complete_prefix(needle, str, prefix, output);
        }
    }
    XFree(keysyms);
}

void complete_against_modifiers(const char* needle, char seperator,
                                char* prefix, Output output) {
    GString* buf = g_string_sized_new(20);
    for (auto& strToMask : KeyCombo::modifierMasks) {
        g_string_printf(buf, "%s%c", strToMask.name.c_str(), seperator);
        try_complete_prefix_partial(needle, buf->str, prefix, output);
    }
    g_string_free(buf, true);
}
