```markdown
# 🔥 Peak Complexity Break Statement

> An absurdly over-engineered, multi-threaded, colorized, statistically-obsessed demonstration of the humble `break` statement in C++.

Because the assignment said:

> **Write and run a C++ program to demonstrate the use of break statement**

...and we decided that "demonstrate" actually meant **"unleash the full industrial might of modern C++ upon a single keyword."**

---

## What This Actually Does

This program demonstrates the `break` statement in four increasingly unhinged ways:

1. **Classic Single-Loop Break**  
   The pure form. Linear search that exits the moment the target is found.

2. **Nested Loop Early Exit**  
   Treats the data as a 2D matrix and breaks out of both loops using a flag (the classic "break only exits the innermost loop" problem, solved properly).

3. **Switch Statement Break (State Machine)**  
   A tiny finite state machine where `break` is used the other way — to terminate `case` labels. Includes cinematic status messages.

4. **Multi-threaded Cooperative Break**  
   The main event. The data is partitioned across N worker threads. The first thread to find the target raises a global atomic "break" signal. Every other thread checks this signal and exits its search loop early. True distributed early termination.

---

## Features (because of course)

- Multi-threaded parallel search with cooperative cancellation
- Atomic control plane for cross-thread break signaling
- High-resolution microsecond timing
- ANSI-colored cinematic terminal output
- Configurable universe size, thread count, and target value
- RAII scoped timers
- Modern C++20 (`std::span`, `<format>`-ready style, ranges-friendly)
- Zero external dependencies

---

## Build

```bash
g++ -std=c++20 -O3 -pthread -Wall -Wextra -o peak_break main.cpp
```

## Run

```bash
./peak_break                          # defaults: 5M elements, all cores, target=42
./peak_break --size 1000000 --threads 8
./peak_break --target 1337 --size 2000000
./peak_break --help
```

---

## Example Output

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║          ██████╗ ███████╗ █████╗ ██╗  ██╗                            ║
  ║          ... BREAK STATEMENT — PEAK COMPLEXITY EDITION ...           ║
  ╚══════════════════════════════════════════════════════════════════════╝

  Configuration
  • Universe size     : 5000000 elements
  • Worker threads    : 16
  • Target value      : 42
  • Target planted at : index 3650000

  DEMO 1 — Classic Single-Loop Break
  ✓ FOUND  value=42  at index=3650000
  comparisons performed: 3650001

  DEMO 4 — Multi-threaded Cooperative Break
  ✓ FOUND  value=42  at index=3650000  (thread 11)
  comparisons performed: ~230000   ← dramatically fewer thanks to early break
```

---

## Why?

Because sometimes the purest way to honor a simple assignment is to take it extremely seriously.

Also because `break` deserves better than the 5-line textbook example.

---

Made with ❤️ and an unreasonable amount of caffeine.  
*"The break statement: small keyword, galaxy-brain potential."*
