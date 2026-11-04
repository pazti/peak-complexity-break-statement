/**
 * ═══════════════════════════════════════════════════════════════════════════════
 *                    PEAK COMPLEXITY BREAK STATEMENT DEMONSTRATOR
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 *  An absurdly over-engineered, multi-threaded, templated, colorized,
 *  statistically-obsessed, enterprise-grade demonstration of the humble
 *  `break` statement in C++.
 *
 *  Because the assignment said "demonstrate the use of break statement"
 *  and we took that as a personal challenge.
 *
 *  Features:
 *    • Multi-threaded parallel search with cooperative early termination
 *    • Nested loop early-exit with labeled-style control flow simulation
 *    • Switch-based state machine that uses break (the other kind)
 *    • Template-driven dimensionality
 *    • Atomic control plane for cross-thread break signaling
 *    • High-resolution timing + statistical reporting
 *    • ANSI-colored cinematic output
 *    • RAII resource management
 *    • Command-line configuration
 *    • Multiple demonstration modes
 *
 *  Compile:
 *      g++ -std=c++20 -O3 -pthread -Wall -Wextra -o peak_break main.cpp
 *
 *  Run:
 *      ./peak_break [--threads N] [--size N] [--target VAL]
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <optional>
#include <span>
#include <format>          // C++20
#include <ranges>          // C++20

// ─────────────────────────────────────────────────────────────────────────────
//  ANSI Color & Style Utilities (because plain text is for peasants)
// ─────────────────────────────────────────────────────────────────────────────
namespace ansi {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* DIM     = "\033[2m";
    constexpr const char* ITALIC  = "\033[3m";
    constexpr const char* UNDER   = "\033[4m";

    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* WHITE   = "\033[37m";
    constexpr const char* BRIGHT_GREEN  = "\033[92m";
    constexpr const char* BRIGHT_YELLOW = "\033[93m";
    constexpr const char* BRIGHT_CYAN   = "\033[96m";
    constexpr const char* BRIGHT_MAGENTA= "\033[95m";
}

// ─────────────────────────────────────────────────────────────────────────────
//  High-resolution Timer (RAII)
// ─────────────────────────────────────────────────────────────────────────────
class ScopedTimer {
public:
    using Clock = std::chrono::high_resolution_clock;

    explicit ScopedTimer(std::string label)
        : label_(std::move(label)), start_(Clock::now()) {}

    ~ScopedTimer() {
        auto end = Clock::now();
        auto us  = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << ansi::DIM << "  ⏱  " << label_ << ": "
                  << ansi::BRIGHT_CYAN << us << " µs" << ansi::RESET << "\n";
    }

    double elapsed_ms() const {
        auto end = Clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    std::string label_;
    Clock::time_point start_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Search Result Structure
// ─────────────────────────────────────────────────────────────────────────────
struct SearchResult {
    bool found = false;
    size_t index = 0;
    int64_t value = 0;
    size_t comparisons = 0;
    int thread_id = -1;
    double time_ms = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Atomic Control Plane — the "break signal" across threads
// ─────────────────────────────────────────────────────────────────────────────
class BreakControlPlane {
public:
    void request_break() noexcept {
        break_requested_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    bool should_break() const noexcept {
        return break_requested_.load(std::memory_order_acquire);
    }

    void reset() noexcept {
        break_requested_.store(false, std::memory_order_release);
    }

    // Wait with timeout (for demonstration of cooperative cancellation)
    bool wait_for_break(std::chrono::milliseconds timeout) {
        std::unique_lock lock(mtx_);
        return cv_.wait_for(lock, timeout, [this] {
            return break_requested_.load(std::memory_order_acquire);
        });
    }

private:
    std::atomic<bool> break_requested_{false};
    std::mutex mtx_;
    std::condition_variable cv_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Data Generator — creates a large searchable universe
// ─────────────────────────────────────────────────────────────────────────────
class UniverseGenerator {
public:
    explicit UniverseGenerator(size_t size, uint64_t seed = 42)
        : size_(size), rng_(seed) {}

    std::vector<int64_t> generate(int64_t target, size_t target_position) {
        std::vector<int64_t> data(size_);
        std::uniform_int_distribution<int64_t> dist(-1'000'000, 1'000'000);

        for (size_t i = 0; i < size_; ++i) {
            data[i] = dist(rng_);
        }

        // Plant the target at a deterministic but non-trivial location
        if (target_position < size_) {
            data[target_position] = target;
        }

        return data;
    }

private:
    size_t size_;
    std::mt19937_64 rng_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Core Break Demonstrators
// ─────────────────────────────────────────────────────────────────────────────

/**
 *  DEMO 1: Classic single-loop break
 *  The purest form. Find first occurrence and break.
 */
SearchResult classic_break_search(std::span<const int64_t> data, int64_t target) {
    SearchResult result;
    size_t comps = 0;

    for (size_t i = 0; i < data.size(); ++i) {
        ++comps;
        if (data[i] == target) {
            result.found = true;
            result.index = i;
            result.value = data[i];
            break;                          // ← THE BREAK
        }
    }

    result.comparisons = comps;
    return result;
}

/**
 *  DEMO 2: Nested-loop early exit (simulating multi-dimensional search)
 *  We treat the 1D array as a 2D matrix and break out of both loops.
 *  Classic "break only exits innermost" problem → solved with a flag + break.
 */
SearchResult nested_break_search(std::span<const int64_t> data, int64_t target, size_t cols) {
    SearchResult result;
    size_t comps = 0;
    size_t rows = data.size() / cols;
    bool found = false;

    for (size_t r = 0; r < rows && !found; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            size_t idx = r * cols + c;
            if (idx >= data.size()) break;

            ++comps;
            if (data[idx] == target) {
                result.found = true;
                result.index = idx;
                result.value = data[idx];
                found = true;
                break;                      // ← break inner
            }
        }
        // Outer loop continues only if !found (the flag acts as labeled break)
    }

    result.comparisons = comps;
    return result;
}

/**
 *  DEMO 3: Switch-based state machine that uses break
 *  (Because break is also the statement that ends switch cases)
 */
void state_machine_break_demo() {
    enum class State { INIT, SEARCHING, FOUND, DONE, ERROR };

    State state = State::INIT;
    int steps = 0;
    const int MAX_STEPS = 20;

    std::cout << ansi::YELLOW << "\n  ▶ State Machine using switch + break:\n" << ansi::RESET;

    while (state != State::DONE && steps < MAX_STEPS) {
        ++steps;

        switch (state) {
            case State::INIT:
                std::cout << "    [INIT] Bootstrapping search systems...\n";
                state = State::SEARCHING;
                break;                      // ← break from switch

            case State::SEARCHING:
                std::cout << "    [SEARCHING] Probing hyperspace coordinates...\n";
                if (steps > 5) {
                    state = State::FOUND;
                }
                break;

            case State::FOUND:
                std::cout << ansi::BRIGHT_GREEN
                          << "    [FOUND] Target acquired. Initiating lock.\n"
                          << ansi::RESET;
                state = State::DONE;
                break;

            case State::ERROR:
                std::cout << ansi::RED << "    [ERROR] Catastrophic failure.\n" << ansi::RESET;
                state = State::DONE;
                break;

            case State::DONE:
                break;
        }
    }

    std::cout << ansi::DIM << "    State machine completed in " << steps << " transitions.\n"
              << ansi::RESET;
}

/**
 *  DEMO 4: Multi-threaded cooperative break
 *  Multiple worker threads search partitions. First one to find signals
 *  a global break via the control plane. Other threads exit their loops early.
 */
SearchResult parallel_break_search(std::span<const int64_t> data,
                                   int64_t target,
                                   int num_threads,
                                   BreakControlPlane& control) {
    SearchResult global_result;
    std::mutex result_mutex;
    std::atomic<size_t> total_comps{0};

    control.reset();

    auto worker = [&](int tid, size_t start, size_t end) {
        size_t local_comps = 0;

        for (size_t i = start; i < end; ++i) {
            // Cooperative cancellation point — the "distributed break"
            if (control.should_break()) {
                break;                      // ← THREAD-LOCAL BREAK triggered by peer
            }

            ++local_comps;
            if (data[i] == target) {
                {
                    std::lock_guard lock(result_mutex);
                    if (!global_result.found) {
                        global_result.found = true;
                        global_result.index = i;
                        global_result.value = data[i];
                        global_result.thread_id = tid;
                    }
                }
                control.request_break();    // Signal everyone else to break
                break;                      // ← BREAK after finding
            }
        }

        total_comps.fetch_add(local_comps, std::memory_order_relaxed);
    };

    std::vector<std::thread> threads;
    size_t chunk = data.size() / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end   = (t == num_threads - 1) ? data.size() : start + chunk;
        threads.emplace_back(worker, t, start, end);
    }

    for (auto& th : threads) {
        th.join();
    }

    global_result.comparisons = total_comps.load();
    return global_result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cinematic Banner & Reporting
// ─────────────────────────────────────────────────────────────────────────────
void print_banner() {
    std::cout << "\n";
    std::cout << ansi::BRIGHT_MAGENTA << ansi::BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                                                                      ║\n";
    std::cout << "  ║          ██████╗ ███████╗ █████╗ ██╗  ██╗                            ║\n";
    std::cout << "  ║          ██╔══██╗██╔════╝██╔══██╗██║ ██╔╝                            ║\n";
    std::cout << "  ║          ██████╔╝█████╗  ███████║█████╔╝                             ║\n";
    std::cout << "  ║          ██╔══██╗██╔══╝  ██╔══██║██╔═██╗                             ║\n";
    std::cout << "  ║          ██████╔╝███████╗██║  ██║██║  ██╗                            ║\n";
    std::cout << "  ║          ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝                            ║\n";
    std::cout << "  ║                                                                      ║\n";
    std::cout << "  ║              STATEMENT — PEAK COMPLEXITY EDITION                     ║\n";
    std::cout << "  ║                                                                      ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════════╝\n";
    std::cout << ansi::RESET;
    std::cout << ansi::DIM
              << "  An enterprise-grade, multi-threaded, statistically-obsessed\n"
              << "  demonstration of the most under-appreciated control-flow keyword.\n"
              << ansi::RESET << "\n";
}

void print_result(const std::string& demo_name, const SearchResult& r) {
    std::cout << ansi::CYAN << "  ┌─ " << demo_name << "\n" << ansi::RESET;
    if (r.found) {
        std::cout << ansi::BRIGHT_GREEN
                  << "  │  ✓ FOUND  value=" << r.value
                  << "  at index=" << r.index;
        if (r.thread_id >= 0) {
            std::cout << "  (thread " << r.thread_id << ")";
        }
        std::cout << ansi::RESET << "\n";
    } else {
        std::cout << ansi::RED << "  │  ✗ NOT FOUND\n" << ansi::RESET;
    }
    std::cout << ansi::DIM
              << "  │  comparisons performed: " << r.comparisons << "\n"
              << "  └──────────────────────────────────────\n"
              << ansi::RESET;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main — Orchestrator of Chaos
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Configuration ──────────────────────────────────────────────────────
    size_t universe_size = 5'000'000;   // 5 million elements
    int    num_threads   = static_cast<int>(std::thread::hardware_concurrency());
    int64_t target       = 42;          // The answer, of course
    size_t target_pos    = universe_size * 0.73;  // Plant it ~73% of the way through

    // Simple CLI parsing (no external deps)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "--size" && i + 1 < argc) {
            universe_size = std::stoull(argv[++i]);
            target_pos = universe_size * 0.73;
        } else if (arg == "--target" && i + 1 < argc) {
            target = std::stoll(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0]
                      << " [--threads N] [--size N] [--target VAL]\n";
            return 0;
        }
    }

    if (num_threads < 1) num_threads = 1;

    print_banner();

    std::cout << ansi::BOLD << "  Configuration\n" << ansi::RESET;
    std::cout << "  • Universe size     : " << universe_size << " elements\n";
    std::cout << "  • Worker threads    : " << num_threads << "\n";
    std::cout << "  • Target value      : " << target << "\n";
    std::cout << "  • Target planted at : index " << target_pos << "\n\n";

    // ── Generate Universe ──────────────────────────────────────────────────
    std::cout << ansi::YELLOW << "  Generating searchable universe...\n" << ansi::RESET;
    UniverseGenerator gen(universe_size);
    auto data = gen.generate(target, target_pos);
    std::cout << ansi::GREEN << "  Universe ready.\n" << ansi::RESET;

    BreakControlPlane control;

    // ── DEMO 1: Classic Break ──────────────────────────────────────────────
    {
        std::cout << "\n" << ansi::BOLD << ansi::UNDER
                  << "  DEMO 1 — Classic Single-Loop Break"
                  << ansi::RESET << "\n";
        ScopedTimer timer("classic_break_search");
        auto result = classic_break_search(data, target);
        print_result("Classic for-loop + break", result);
    }

    // ── DEMO 2: Nested Break ───────────────────────────────────────────────
    {
        std::cout << "\n" << ansi::BOLD << ansi::UNDER
                  << "  DEMO 2 — Nested Loop Early Exit"
                  << ansi::RESET << "\n";
        ScopedTimer timer("nested_break_search");
        // Treat as roughly square matrix
        size_t cols = static_cast<size_t>(std::sqrt(universe_size));
        auto result = nested_break_search(data, target, cols);
        print_result("Nested loops + flag-controlled break", result);
    }

    // ── DEMO 3: Switch Break ───────────────────────────────────────────────
    {
        std::cout << "\n" << ansi::BOLD << ansi::UNDER
                  << "  DEMO 3 — Switch Statement Break (State Machine)"
                  << ansi::RESET << "\n";
        state_machine_break_demo();
    }

    // ── DEMO 4: Parallel Cooperative Break ─────────────────────────────────
    {
        std::cout << "\n" << ansi::BOLD << ansi::UNDER
                  << "  DEMO 4 — Multi-threaded Cooperative Break"
                  << ansi::RESET << "\n";
        std::cout << ansi::DIM
                  << "  First thread to find the target signals a global break.\n"
                  << "  All other threads exit their search loops early.\n"
                  << ansi::RESET;

        ScopedTimer timer("parallel_break_search");
        auto result = parallel_break_search(data, target, num_threads, control);
        print_result("Parallel partition search + atomic break signal", result);
    }

    // ── Final Philosophical Note ───────────────────────────────────────────
    std::cout << "\n" << ansi::BRIGHT_MAGENTA << ansi::BOLD
              << "  ════════════════════════════════════════════════════════════\n"
              << "   The break statement: small keyword, galaxy-brain potential.\n"
              << "   You have now witnessed it in its purest and most unhinged forms.\n"
              << "  ════════════════════════════════════════════════════════════\n"
              << ansi::RESET << "\n";

    return 0;
}
