#include "globals.h"

namespace std {
    //! Definition of black hole for streams (like std::cout but for /dev/null)
    std::ostream cnull(0);
} // namespace std

std::ostream &hlwmDebugStream() {
    if (g_verbose) {
        return std::cerr;
    } else {
        return std::cnull;
    }
}
