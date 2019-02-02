#pragma once

#include "xkeygrabber.h"
#include "object.h"
#include "client.h"

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

    bool operator==(const std::string& other) {
        return str == other;
    }

    bool operator==(const Keymask& other) {
        return other.str == str;
    }

    bool operator!=(const Keymask& other) {
        return !(*this == other);
    }

    std::string str;
    std::regex regex;
};

class KeyManager : public Object {
public:
    int addKeybindCommand(Input input, Output output);
    int listKeybindsCommand(Output output);
    int removeKeybindCommand(Input input, Output output);

    void regrabAll();
    void ensureKeymask(const Client* client = nullptr);
    void setActiveKeymask(const Keymask& newMask);

private:
    XKeyGrabber xKeyGrabber_;

    // The last known keymask (for comparison on change)
    Keymask activeKeymask_;
};
