#pragma once

#include <X11/Xlib.h>
#include <memory>
#include <string>
#include <vector>

#include "client.h"
#include "keybinding.h"
#include "object.h"
#include "types.h"
#include "xkeygrabber.h"

class Keymask {
public:
    Keymask(const std::string& maskStr = "")
        : str(maskStr)
    {
        if (str != "") {
            // Simply pass on any exceptions thrown here:
            regex = std::regex(str, std::regex::extended);
        }
    }

    bool operator==(const Keymask& other) const {
        return other.str == str;
    }

    std::string str;
    std::regex regex;
};

class KeyManager : public Object {
public:
    KeyManager();
    ~KeyManager();

    int addKeybindCommand(Input input, Output output);
    int listKeybindsCommand(Output output);
    int removeKeybindCommand(Input input, Output output);

    void handleKeyPress(XEvent* ev);

    void regrabAll();
    void ensureKeymask(const Client* client = nullptr);
    void setActiveKeymask(const Keymask& newMask);

    // TODO: This is not supposed to exist. It only does as a workaround,
    // because mouse.cpp still wants to know the numlock mask.
    unsigned int getNumlockMask() const {
        return xKeyGrabber_.getNumlockMask();
    }

    //! Currently defined keybindings (TODO: Make this private as soon as possible)
    std::vector<std::unique_ptr<KeyBinding>> binds;

private:
    bool removeKeybinding(const KeyCombo& comboToRemove);

    XKeyGrabber xKeyGrabber_;

    // The last known keymask (for comparison on change)
    Keymask activeKeymask_;
};
