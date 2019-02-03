#pragma once

#include <X11/X.h>
#include <iostream>
#include <regex>
#include <vector>

/*!
 * Represents a keypress combination of a keysym and modifiers (optional).
 *
 * Handles the parsing and creation of string representations of itself.
 */
class KeyCombo {
public:
    KeyCombo() = default;
    KeyCombo(std::string str);
    std::string origstr; // just for debugging

    typedef struct {
        std::string name;
        unsigned int mask;
    } ModifierNameAndMask;

    std::string str() const;

    bool matches(const std::regex& regex) const;

    /*!
     * List of modifiers and their corresponding key masks.
     *
     * This is not an std::map because even though this an surjective relation,
     * we need well-defined reverse lookups (from mask to string).
     */
    static const std::vector<ModifierNameAndMask> modifierMasks;

    static unsigned int getMaskForModifierName(std::string name);

    static std::vector<std::string> getNamesForModifierMask(unsigned int mask);

    static unsigned int string2modifiers(const std::string& str);
    static KeySym string2keysym(const std::string& str);

    KeySym keysym;
    unsigned int modifiers;

    bool operator==(const KeyCombo& other);

private:
    static std::vector<std::string> splitKeySpec(std::string keySpec);
};
