#include "keymanager.h"

#include <X11/keysym.h>
#include <memory>

#include "clientmanager.h"
#include "completion.h"
#include "globals.h"
#include "ipc-protocol.h"
#include "key.h"
#include "root.h"
#include "utils.h"

using std::string;
using std::unique_ptr;
using std::vector;

typedef struct {
    const char* name;
    unsigned int mask;
} Name2Modifier;

extern Name2Modifier g_modifier_names[];

extern vector<unique_ptr<KeyBinding>> g_key_binds;

int KeyManager::addKeybindCommand(Input input, Output output) {
    if (input.size() < 2) {
        return HERBST_NEED_MORE_ARGS;
    }

    auto newBinding = make_unique<KeyBinding>();

    try {
        newBinding->keyCombo = KeyCombo(input.front());
    } catch (std::runtime_error &error) {
        output << input.command() << ": " << error.what() << std::endl;
        return HERBST_INVALID_ARGUMENT;
    }

    input.shift();
    // Store remaining input as the associated command
    newBinding->cmd = {input.begin(), input.end()};

    // Remove existing binding with same keysym/modifiers
    key_remove_bind_with_keysym(newBinding->keyCombo.modifiers, newBinding->keyCombo.keysym);

    // Grab for events on this keycode
    xKeyGrabber_.grabKeyCombo(newBinding->keyCombo);

    // Add keybinding to list
    g_key_binds.push_back(std::move(newBinding));

    ensureKeymask();

    return HERBST_EXIT_SUCCESS;
}

int KeyManager::listKeybindsCommand(Output output) {
    for (auto& binding : g_key_binds) {
        // add keybinding
        output << keybinding_to_string(binding.get());
        // add associated command
        output << "\t" << ArgList(binding->cmd).join('\t');
        output << "\n";
    }
    return 0;
}

int KeyManager::removeKeybindCommand(Input input, Output output) {
    std::string arg;
    if (!(input >> arg)) {
        return HERBST_NEED_MORE_ARGS;
    }

    if (arg == "--all" || arg == "-F") {
        key_remove_all_binds();
    } else {
        unsigned int modifiers;
        KeySym keysym;
        try {
            modifiers = KeyCombo::string2modifiers(arg);
            keysym = KeyCombo::string2keysym(arg);
        } catch (std::runtime_error &error) {
            output << input.command() << ": " << arg << ": " << error.what() << "\n";
            return HERBST_INVALID_ARGUMENT;
        }

        if (key_remove_bind_with_keysym(modifiers, keysym) == false) {
            output << input.command() << ": Key \"" << arg << "\" is not bound\n";
        }
        regrab_keys();
    }

    return HERBST_EXIT_SUCCESS;
}

void KeyManager::regrabAll() {
    xKeyGrabber_.updateKeyboardMapping();

     // Remove all current grabs:
    XUngrabKey(g_display, AnyKey, AnyModifier, g_root);

    for (auto& binding : g_key_binds) {
        xKeyGrabber_.grabKeyCombo(binding->keyCombo);
    }

}


/*!
 * Ensures that the keymask of the currently focused client is applied.
 */
void KeyManager::ensureKeymask() {
    // Reapply the current keymask (if any)
    auto focusedClient = Root::get()->clients()->focus();
    if (focusedClient != nullptr) {
        key_set_keymask(focusedClient->keymask_());
    }

}
