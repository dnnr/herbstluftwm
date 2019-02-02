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

    if (!newBinding->keyCombo.matches(activeKeymask_.regex)) {
        // Grab for events on this keycode
        xKeyGrabber_.grabKeyCombo(newBinding->keyCombo);
        newBinding->enabled = true;
    }

    // Add keybinding to list
    g_key_binds.push_back(std::move(newBinding));

    return HERBST_EXIT_SUCCESS;
}

int KeyManager::listKeybindsCommand(Output output) {
    for (auto& binding : g_key_binds) {
        // add key combo
        output << binding->keyCombo.str();
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
        binding->enabled = true;
    }
}

/*!
 * Makes sure that the currently active keymask is correct for the currently
 * focused client and regrabs keys if necessary
 *
 * FIXME: The Client* argument only exists because I failed to find a place to
 * call this function on focus changes where ClientManager::focus is already
 * updated.
 */
void KeyManager::ensureKeymask(const Client* client) {
    std::string targetMaskStr;
    if (client == nullptr) {
        client = Root::get()->clients()->focus();
    }

    if (client == nullptr) {
        targetMaskStr = "";
        HSDebug("KeyManager::ensureKeymask(): Nothing in focus, using empty mask\n");
    } else {
        targetMaskStr = client->keymask_();
        HSDebug("KeyManager::ensureKeymask(): Assuming focused client: %s\n", client->window_id_str().c_str());
    }


    if (activeKeymask_ != targetMaskStr) {
        try {
            HSDebug("KeyManager::ensureKeymask(): Applying new keymask: \"%s\"\n", targetMaskStr.c_str());
            setActiveKeymask({targetMaskStr});
        } catch(std::regex_error& err) {
            HSDebug("KeyManager::ensureKeymask(): Can not apply invalid regex \"%s\": %s\n",
                    targetMaskStr.c_str(), err.what());

            // Fall back to empty mask:
            setActiveKeymask({});
        }

    } else {
        HSDebug("KeyManager::ensureKeymask(): nothing to do (keymask is still \"%s\")\n", activeKeymask_.str.c_str());
    }
}

//! Apply new keymask by grabbing/ungrabbing current bindings accordingly
void KeyManager::setActiveKeymask(const Keymask& newMask) {
    for (auto& binding : g_key_binds) {
        auto name = binding->keyCombo.str();
        bool isMasked = binding->keyCombo.matches(newMask.regex);

        if (!isMasked && !binding->enabled) {
            HSDebug("KeyManager::setActiveKeymask(): Grabbing %s\n", name.c_str());
            xKeyGrabber_.grabKeyCombo(binding->keyCombo);
            binding->enabled = true;
        } else if (isMasked && binding->enabled) {
            HSDebug("KeyManager::setActiveKeymask(): Ungrabbing %s\n", name.c_str());
            xKeyGrabber_.ungrabKeyCombo(binding->keyCombo);
            binding->enabled = false;
        }
    }
    activeKeymask_ = newMask;
}
