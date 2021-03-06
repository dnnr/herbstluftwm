#pragma once

#include <X11/X.h>
#include <regex>
#include <string>
#include <vector>

/*!
 * Represents a keypress combination of a keysym and modifiers (optional).
 *
 * Handles the parsing and creation of string representations of itself.
 */
class KeyCombo {
public:
    KeyCombo() = default;

    typedef struct {
        std::string name;
        unsigned int mask;
    } ModifierNameAndMask;

    /*!
     * List of existing modifiers and their corresponding key masks.
     *
     * This is not an std::map because even though it is a surjective relation,
     * we need well-defined reverse lookups for stringification.
     */
    static const std::vector<ModifierNameAndMask> modifierMasks;

    static constexpr auto separators = "+-";

    std::string str() const;
    bool matches(const std::regex& regex) const;
    bool operator==(const KeyCombo& other) const;
    static std::vector<std::string> tokensFromString(std::string keySpec);
    static unsigned int modifierMaskFromTokens(const std::vector<std::string>& tokens);
    static KeySym keySymFromString(const std::string& str);
    static KeyCombo fromString(const std::string& str);
    static std::vector<std::string> getPossibleKeySyms();

    KeySym keysym;
    unsigned int modifiers;

private:
    static unsigned int getMaskForModifierName(std::string name);
    static std::vector<std::string> getNamesForModifierMask(unsigned int mask);
};
