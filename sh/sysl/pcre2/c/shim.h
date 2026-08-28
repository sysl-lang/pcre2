/* PCRE2 is three libraries -- 8-bit, 16-bit and 32-bit code units -- built from
 * one source and distinguished by a macro. `pcre2.h` refuses to compile unless
 * `PCRE2_CODE_UNIT_WIDTH` says which one is wanted, and the macro also decides
 * what every `pcre2_*` name in the header expands to: with 8 set, `pcre2_compile`
 * is a macro for `pcre2_compile_8`, which is the symbol the linker actually has.
 *
 * That is the whole of the C in this package. There is no `.c` file: every
 * function is a real symbol under its suffixed name, so `c.sysl` declares them
 * directly and nothing has to be wrapped. This header exists only because a
 * `c const` block needs the width set before the header it reads is included,
 * and there is nowhere else to say it.
 *
 * sysl strings are UTF-8, so 8 is the only width this binding could want.
 */
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
