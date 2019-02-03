#ifndef __HERBST_KEY_H_
#define __HERBST_KEY_H_

#include <X11/Xlib.h>

#include "glib-backports.h"
#include "keybinding.h"
#include "keycombo.h"
#include "types.h"

#define KEY_COMBI_SEPARATORS "+-"

unsigned int modifiername2mask(const char* name);
const char* modifiermask2name(unsigned int mask);

int list_keysyms(int argc, char** argv, Output output);
bool key_remove_bind_with_keysym(unsigned int modifiers, KeySym sym);
void key_remove_all_binds();
void key_find_binds(const char* needle, Output output);
void complete_against_modifiers(const char* needle, char seperator,
                                char* prefix, Output output);
void complete_against_keysyms(const char* needle, char* prefix, Output output);
void update_numlockmask();
unsigned int* get_numlockmask_ptr();

#endif
