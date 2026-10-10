# pcre2

Perl-compatible regular expressions for [sysl](https://github.com/sysl-lang/sysl), bound to
[PCRE2](https://github.com/PCRE2Project/pcre2).

```
dependencies {
  pcre2 { git = "github.com/sysl-lang/pcre2", version = "0.1.3" }
}
```

```
import sh.sysl.pcre2.{compile, describe}

main() -> Result[unit, string]
    val re = compile("(\\w+)@([\\w.]+)").map_err(describe)?

    re.find("write to ed@example.com today").map_err(describe)? match
        Some(m) -> print(m.text(), m.group(1).unwrap(), m.group(2).unwrap())
        None -> print("no address")
```

## Why there are two regex engines

sysl's standard library already has one. `sysl.regex` is a Thompson/Pike engine over **POSIX extended
syntax**: every branch of the pattern is followed at once, so the cost is the input length times the
program length and no pattern makes it explode. What it does not have is the syntax people actually
write — there is no `\d`, no `\w`, no lazy quantifier, no lookaround, no backreference and no
non-capturing group, and under POSIX `\d` is not an error but a literal `d`.

This package is the other trade. PCRE2 gives the dialect, and pays for it by backtracking.

|  | `sysl.regex` | `sh.sysl.pcre2` |
|---|---|---|
| syntax | POSIX ERE | Perl, near-identical to JavaScript's |
| `\d` `\w` `\s` | literal `d`, `w`, `s` | character classes |
| lazy quantifiers | no | yes |
| lookahead / lookbehind | no | yes |
| backreferences | **cannot** — a Pike VM has no way | yes |
| named groups | no | yes |
| worst case | linear in input × program | exponential, and bounded by a budget |
| dependency | none, it is sysl | PCRE2 installed |
| bare metal | yes | no |

**Neither is the right default for everything.** A pattern from a configuration file or a person on
the far end of a socket is safer under `sysl.regex`, whose guarantee has no hole in it. A program
whose patterns are its own, written by someone who has used a regular expression before, wants this
one.

**The two have deliberately the same API**, so moving between them is changing an import. `Match`
answers `span`, `group`, `text`, `start` and `end`; `Regex` answers `find`, `find_at`, `is_match`,
`find_all`, `replace_all` and `split`. They differ in exactly two places, both forced: compiling here
can fail on syntax POSIX cannot express, and matching here can *run out*, which a Pike VM cannot — so
`find` answers a `Result[Option[Match], Error]` rather than an `Option[Match]`.

## The backtracking budget

PCRE2 backtracks, so `(a+)+$` against forty `a`s and no `b` is the standard demonstration of a short
pattern and a short subject costing an unbounded amount of time.

**Every pattern this package compiles is given a match context with limits set**, and an overrun is
`MatchLimit` rather than a program that stops answering:

```
default_limits() -> Limits = Limits(1000000, 10000)
```

PCRE2's own defaults are ten million steps, which is around a second of solid backtracking on a
current machine — long enough that whatever was waiting has already failed. A million is generous for
every pattern that is not pathological; this package's whole test suite runs inside it.

Pass your own through `compile_with` where a pattern genuinely needs more.

## JavaScript is not quite Perl, and the differences are silent

Two of PCRE2's defaults disagree with ECMAScript, and neither produces an error — the pattern
compiles either way and merely matches something else. `javascript()` is the preset that settles both:

- **`$`** means the very end in JavaScript, and in Perl also the position before a final newline.
  `/^foo$/` against `"foo\n"` is `false` in a browser and true under `plain()`.
- **`\d`** is `[0-9]` in JavaScript, and every Unicode decimal digit in Perl once `UCP` is on. This
  preset leaves `UCP` off.

`javascript_with(icase, multiline, dotall)` takes the flags a program spells after the slash.

**UTF-8 mode is not an option and cannot be turned off.** A sysl string is UTF-8 by construction, and
a pattern compiled without it would step bytes — `.` would match one third of a snowman.

## The binding

```
sh/sysl/pcre2/
    pcre2.sysl          the binding
    tests.sysl          18 tests, each written so only real matching passes it
    c/
        c.sysl          PCRE2 as C declares it
        shim.h          two lines, and the only C in the package
package.hocon           who this package is, and what it needs of the machine
```

**There is no shim, which is unusual enough to say why.** A binding normally needs C for one of three
things — a caller-allocated opaque type, a macro used as a value, or a shape with no sysl spelling —
and PCRE2 has none of them:

- **Every handle is allocated by the library.** `pcre2_compile` returns a `pcre2_code *` and
  `pcre2_code_free` takes it back. Nothing needs a `calloc` wrapper the way `regex_t` does in
  [regex](https://github.com/sysl-lang/regex), whose size is a platform's business.
- **The macros are numbers**, and `c const` asks the C compiler for them.
- **The match offsets are a plain `PCRE2_SIZE *`** — `size_t *`, which is `*usize` — rather than a
  struct with a layout only the header knows. `regmatch_t` is what forces POSIX regex through a shim;
  PCRE2 hands over the array itself.

`c/shim.h` is two lines and exists only because `pcre2.h` refuses to compile until
`PCRE2_CODE_UNIT_WIDTH` says which of the three libraries is wanted. That macro also decides what
every name in the header expands to: **`pcre2_compile` is not a symbol — `pcre2_compile_8` is**, which
is why each `extern` names the suffixed form and why no wrapper is needed to reach any of them.

## Installing PCRE2

```
brew install pcre2
```

Debian and Ubuntu package it as `libpcre2-dev`, Arch as `pcre2`, Alpine as `pcre2-dev`. The
`pkg_config` block in `package.hocon` turns a missing one into a sentence at build time rather than a
link error naming a symbol.

## Tests

```
sysl test .
```

Eighteen, and each is written so that only real PCRE2 matching produces the answer — the offsets are
mid-string, the groups are read by number and by name, and the two presets are told apart by subjects
on which they disagree. A test that only asked "did it match" would pass against an `extern` that
dropped an argument.

## License

ISC
