#include "keycombo.h"

#include <X11/Xlib.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "globals.h"

using std::vector;
using std::string;

#define KEY_COMBI_SEPARATORS "+-"

const vector<KeyCombo::ModifierNameAndMask> KeyCombo::modifierMasks = {
    { "Mod1",       Mod1Mask },
    { "Mod2",       Mod2Mask },
    { "Mod3",       Mod3Mask },
    { "Mod4",       Mod4Mask },
    { "Mod5",       Mod5Mask },
    { "Alt",        Mod1Mask },
    { "Super",      Mod4Mask },
    { "Shift",      ShiftMask },
    { "Control",    ControlMask },
    { "Ctrl",       ControlMask },
};

/*!
 * Provides the mask value for a given modifier name.
 *
 * If the modifier name is unknown, an empty mask (zero) is returned.
 */
unsigned int KeyCombo::getMaskForModifierName(string name) {
    for (auto& entry : modifierMasks) {
        if (entry.name == name) {
            return entry.mask;
        }
    }

    throw std::runtime_error("Unknown modifier name " + name);
    // or return 0?
}

/*!
 * Provides the names of modifiers that are set in a given mask.
 */
vector<string> KeyCombo::getNamesForModifierMask(unsigned int mask) {
    vector<string> names;
    for (auto& entry : modifierMasks) {
        if (entry.mask & mask) {
            names.push_back(entry.name);

            // remove match from mask
            mask = mask & ~ entry.mask;
        }
    }

    return names;
}


/*!
 * Provides a canonical string representation of the key combo
 */
std::string KeyCombo::str() const {
    std::stringstream str;

    /* add modifiers */
    unsigned int old_mask = 0, new_mask = modifiers;
    while (new_mask != 0 && new_mask != old_mask) {
        old_mask = new_mask;

        // reverse lookup modifier name by its mask
        string name;
        unsigned int mask = 0;
        for (auto &strToMask : KeyCombo::modifierMasks) {
            if (strToMask.mask & old_mask) {
                name = strToMask.name;
                mask = strToMask.mask;
                break;
            }
        }

        str << name << KEY_COMBI_SEPARATORS[0];
        /* remove found mask from mask */
        new_mask = old_mask & ~ mask;
    }

    /* add keysym */
    const char* name = XKeysymToString(keysym);
    if (!name) {
        HSWarning("XKeysymToString failed! using \'?\' instead\n");
        name = "?";
    }
    str << name;

    return str.str();
}

/*!
 * Returns true if the string representation of this KeyCombo matches the given
 * regex
 */
bool KeyCombo::matches(const std::regex& regex) const {
    return std::regex_match(str(), regex);
}

/*!
 * Parses the modifiers part of a key combo string into a mask value.
 *
 * \throws meaningful exceptions on parsing errors
 */
unsigned int KeyCombo::string2modifiers(const string& str) {
    auto splitted = splitKeySpec(str);

    if (splitted.empty()) {
        throw std::runtime_error("Empty keysym");
    }

    unsigned int modifiers = 0;
    // all parts except last one are modifiers
    for (auto iter = splitted.begin(); iter + 1 != splitted.end(); iter++) {
        unsigned int mask = 0;
        // Linear search for matching list entry:
        for (auto& entry : modifierMasks) {
            if (entry.name == *iter) {
                mask = entry.mask;
                break;
            }
        }
        if (mask == 0) {
            throw std::runtime_error("Unknown modifier \"" + *iter + "\"");
        }
        modifiers |= mask;
    }
    return modifiers;
}

/*!
 * Parses the keysym part of a key combo string into a KeySym.
 *
 * \throws meaningful exceptions on parsing errors
 */
KeySym KeyCombo::string2keysym(const string& str) {
    // last one is the key
    auto lastToken = splitKeySpec(str).back();
    auto keysym = XStringToKeysym(lastToken.c_str());
    if (keysym == NoSymbol) {
        throw std::runtime_error("Unknown KeySym \"" + lastToken + "\"");
    }
    return keysym;
}

/*!
 * Creates a KeyCombo from a given string representation
 *
 * Example inputs: "Mod1-space", "Mod4+f", "f"
 *
 * In order to avoid throwing exceptions from constructors (they often get
 * called implicitly), this is implemented as a static method.
 *
 * \throws meaningful exceptions on parsing errors
 */
KeyCombo KeyCombo::fromString(const std::string& str) {
    KeyCombo combo;
    combo.modifiers = string2modifiers(str);
    combo.keysym = string2keysym(str);
    return combo;
}

bool KeyCombo::operator==(const KeyCombo& other) const {
    bool sameMods = modifiers == other.modifiers;
    bool sameKeySym = keysym == other.keysym;
    return sameMods && sameKeySym;
}

vector<string> KeyCombo::splitKeySpec(string keySpec)
{
    // Normalize spec to use a single separator:
    char baseSep = KEY_COMBI_SEPARATORS[0];
    for (auto &sep : KEY_COMBI_SEPARATORS) {
        std::replace(keySpec.begin(), keySpec.end(), sep, baseSep);
    }

    // Split spec into tokens:
    vector<string> tokens;
    string token;
    std::istringstream tokenStream(keySpec);
    while (std::getline(tokenStream, token, baseSep)) {
        tokens.push_back(token);
    }
    return tokens;
}

