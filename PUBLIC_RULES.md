# Public Rules Notes

## What the public cases are actually checking

- The public cases do not compare only standard output.
- They compare: program output, then the `main` return value.
- The `main` return value is interpreted as an 8-bit exit code.

## Evidence from `public/`

- `public/testfile1.txt` returns `3`, and `public/output1.txt` is `3`.
- `public/testfile12.txt` returns `2 - 10 = -8`, and `public/output12.txt` is `248`.
  - This means the judge uses `(-8) mod 256 = 248`.
- `public/testfile26.txt` prints `1024` and returns `1024`, while `public/output26.txt` is:

```text
1024
0
```

  - This means the return value is reduced to `1024 mod 256 = 0`.

## Runtime rule

- We must not redefine runtime functions in generated code.
- Generated code should only declare and use runtime symbols such as:
  - `getint`
  - `getch`
  - `getarray`
  - `putint`
  - `putch`
  - `putarray`
  - `putf`
  - `_sysy_starttime`
  - `_sysy_stoptime`

## Packaging rule

- The platform expects the runtime files under the `runtime/` directory.
- Therefore the submission archive must preserve:
  - `runtime/sylib.c`
  - `runtime/sylib.h`
- Flattening these two files into the archive root is unsafe.

## Local test rule

- Local `test_codegen` should use the files under `public/` directly.
- Local result comparison should follow the same rule as the public outputs:
  - normalized stdout
  - plus 8-bit normalized exit code from `main`
