AGENTS guide for ofxDividedArea

Build, lint, test
- Build example (Xcode): open example/example.xcodeproj and run the "example" scheme.
- Build (make): cd example && make Debug && make run
- Clean: cd example && make clean
- CI setup scripts: scripts/ci/osx/install.sh and scripts/ci/linux/install.sh
- Tests: no test framework present; `tests/` is empty aside from .gitkeep. To add tests, create an oF app under tests/ and build similarly to example.
- Single test: not applicable until a test harness exists.

Code style guidelines
- Language: C++17 as supported by openFrameworks 0.12+. Prefer standard library over custom utilities.
- Headers: use #pragma once. Keep includes minimal; include what you use. Prefer forward declarations in headers.
- Includes order: standard C++ > openFrameworks (ofMain.h, etc.) > local headers ("..."). Use angle brackets for system/OF, quotes for local.
- Formatting: 2 or 4 spaces, be consistent with surrounding files. Brace on same line; one statement per line. Limit lines to ~120 chars.
- Names: Types PascalCase (DividerLine), functions/methods camelCase (intersectsWith), variables camelCase, constants UPPER_SNAKE, private members with trailing underscore if needed.
- Types: Use const correctness, references over pointers when non-null, auto only when it improves clarity; prefer std::vector/std::optional.
- Errors: Avoid exceptions in real-time paths; validate inputs with assertions in debug and safe early-returns in release. Log via ofLogNotice/Warning/Error.
- Ownership: Prefer value or unique_ptr; avoid raw owning pointers. Pass spans or const refs for large data.
- Threading: openFrameworks is largely single-threaded; guard shared state and never call GL from non-main threads.
- Documentation: brief Doxygen-style comments for public APIs; explain non-obvious math/geometry in LineGeom and intersection logic.

Tooling and rules
- No Cursor or Copilot rules found in this repo.
- Respect addon structure: public API in src/*.h, implementation in *.cpp. Keep example building on macOS and Linux.
