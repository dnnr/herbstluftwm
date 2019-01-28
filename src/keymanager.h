#pragma once

#include "xkeygrabber.h"
#include "object.h"

class KeyManager : public Object {
public:
    int addKeybindCommand(Input input, Output output);
    int listKeybindsCommand(Output output);
    int removeKeybindCommand(Input input, Output output);

    void setKeyMask(std::string keyMask);

    void regrabAll();

private:
    XKeyGrabber xKeyGrabber_;
    std::string keyMask_;
};
