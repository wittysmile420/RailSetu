# RailSetu Evaluator Guide

## 1. Evaluation objectives

An evaluator should verify all of the following:

1. project builds and tests;
2. seat and PNR allocations remain unique;
3. concurrent requests respect capacity;
4. waitlist priority and FIFO ties are correct;
5. cancellation and refund/promotion behavior is correct;
6. persistence survives restart;
7. unsafe race and deadlock demonstrations are isolated;
8. stress output and reports are reproducible;
9. team members can explain implementation evidence and limitations.

## 2. Environment prerequisites

Required:

- C++17 compiler;
- CMake 3.16 or newer;
- Windows PowerShell 5.1/7 or POSIX shell;
- standard terminal.

Optional:

- VS Code Mermaid rendering;
- browser for `reports/railsetu_report.html`.

No network, database, or third-party C++ dependency is required.

## 3. Clean Windows build

Run at the workspace root:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Helper:

```powershell
.\scripts\build-and-test.ps1
```

Expected executables:

```text
build\railsetu_app.exe
build\railsetu_stress.exe
build\railsetu_tests.exe
```

## 4. Clean Linux/macOS build

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/railsetu_app
./build/railsetu_stress 100 20
```

Alternative helper:

```sh
./scripts/build.sh
```

## 5. Exact test targets

CTest registers:

| Test target | Covers |
|---|---|
| `railsetu_tests` | Core booking, cancellation, FIFO, persistence, semaphore, seat, demo |
| `test_concurrent_booking` | 20 concurrent capacity allocation |
| `test_cancellation` | Promotion and repeated-cancel rejection |
| `test_waiting_promotion` | Priority promotion |
| `test_semaphore` | Permit acquire/release |
| `test_persistence` | Restart load |
| `test_race_validation` | Safe race, unsafe demo, deadlock completion |
| `test_visual_features` | Waitlist and all-booking snapshots |

Run one target or all tests:

```powershell
ctest --test-dir build -R test_waiting_promotion --output-on-failure
ctest --test-dir build --output-on-failure
```

## 6. Demonstration isolation

Use a named state and log:

```powershell
.\build\railsetu_app.exe evaluation.state logs\evaluator.log
```

Linux:

```sh
./build/railsetu_app evaluation.state logs/evaluator.log
```

This demonstrates persistence without mixing with the default
`data/railsetu.state`.

## 7. Startup and discovery

```text
list
seats RS101
details
```

For menu details, enter `3` and train ID `RS101`; for seat display enter `4`
and train ID `RS101`.

Expected:

- three seeded trains if state is absent;
- RS101 capacity 120;
- A1 and S1 coaches;
- available seat labels such as `A1-1`.

## 8. Booking and PNR evidence

```text
book RS101 Alice 1 SENIOR
status RS00000001
search Alice
```

Ask evaluator to verify:

- PNR format;
- `CONFIRMED`;
- category `SENIOR`;
- fare `INR 100` per seat in the default runtime seed;
- unique coach-seat label;
- passenger search result.

Note: default seed train base fare is 100 in `src/main.cpp`, even though
`data/trains.json` is a separate illustrative seed carrying fares of 2450/850.
The runtime application is authoritative.

## 9. Priority and FIFO scenario

Fill RS101's free coach capacity or use a small new coach. Existing RS101
capacity is 120, so choose a new isolated train:

```text
coach DEMO C1 GENERAL 2
```

`coach DEMO` only works if train `DEMO` exists. Create it through the C++ API,
or use RS101 and fill enough seats. For an evaluator-safe waitlist:

```text
cancel RS00000001
```

That may free only one seat. Add waitlisted passengers while capacity is full
and then cancel the next confirmed PNR.

Priority order:

```text
SENIOR (3) > LADIES (2) > TATKAL (1) > GENERAL (0)
```

Within one category, `sequence` order is FIFO.

## 10. Cancellation and refund

```text
status RS00000001
cancel RS00000001
refund RS00000001
status RS00000001
history
```

Verify:

- status becomes `CANCELLED`;
- repeated cancel fails;
- refund is 80%;
- cancelled waitlist refund is zero;
- waitlist head moves to confirmed after enough capacity is released;
- cancellation UTC text appears in history.

## 11. Waitlist automatic promotion

The most reliable sequence:

1. Fill train capacity with an isolated train or by reserving every seat.
2. Create one GENERAL waitlist entry.
3. Create one SENIOR waitlist entry.
4. Cancel a confirmed booking.
5. Run `ladder RS101` or `ladder TRAIN`.
6. Run `status` for the senior PNR.

The senior PNR must confirm before GENERAL even if GENERAL arrived earlier,
while equal-category PNRs retain booking sequence order.

## 12. Condition-variable wait

```text
wait RS101 1 10000
```

Have a second terminal or evaluator cancel a PNR while the wait is sleeping.
The wait predicate rechecks availability after `notify_all`. A timeout prints
`Timed out`.

## 13. Persistence

```text
book RS101 PersistenceDemo 1 LADIES
save
backup
exit
```

Restart with the same state path:

```powershell
.\build\railsetu_app.exe data\railsetu.state logs\activity.log
```

Verify:

```text
search PersistenceDemo
status RS00000002
list
```

PNR numbers continue from persisted maximum sequence.

Backup exercise:

```text
backup
restore
list
```

## 14. Atomic recovery interpretation

`save` writes `railsetu.state.tmp`, copies the prior state to
`railsetu.state.bak`, removes the target, and renames temporary to target.
`restore` replaces state with the backup and calls `load()`.

If `load()` encounters malformed numeric data or invalid references, it fails
without installing a partial state.

## 15. Unsafe race demonstration

```text
unsafe-race
```

Observe:

- method executes eight isolated threads;
- it reports an allocation count;
- real state and disk remain untouched;
- result demonstrates stale-read/check-update danger.

Do not claim this method creates an undefined C++ race; atomic counters isolate
it while preserving logical race behavior.

## 16. Deadlock demonstration

```text
deadlock
```

The demo creates two mutexes and uses `std::lock(first, second)`.
Both threads acquire in one operation and print success.
The real booking path uses one manager mutex and avoids nested manager locks.

## 17. Concurrent stress

Windows:

```powershell
.\build\railsetu_stress.exe 64 10
.\build\railsetu_stress.exe 100 20
```

Linux:

```sh
./build/railsetu_stress 64 10
./build/railsetu_stress 100 20
```

Default shape:

```text
threads × bookings_per_thread
100 × 20 = 2000 requests
train STRESS capacity = 100
```

Acceptance:

- confirmed <= 100;
- failures = 0;
- waitlisted + confirmed = 2000;
- elapsed output is present;
- CSV and summary updated.

The stress executable disables persistence. It starts all worker threads behind
a semaphore gate, but the manager mutex serializes real booking transactions.

## 18. Report export

At prompt:

```text
dashboard
route
ladder RS101
log
export
```

Or menu 20.

Inspect:

```text
benchmark/railsetu_report.txt
benchmark/benchmark.csv
benchmark/benchmark_summary.txt
reports/occupancy.svg
reports/latency.svg
reports/priority.svg
reports/railsetu_report.html
reports/architecture.mmd
```

Open HTML in a browser. The latency SVG is explicitly illustrative; it does not
represent stored stress timing per sample.

## 19. Browser control center

The static browser experience is served locally and has no package installation
or external service dependency. It stores its own demo state in the browser's
`localStorage`; that data is intentionally separate from the C++ runtime state
file and is not an HTTP API connection to the C++ process.

Windows PowerShell:

```powershell
.\scripts\start-web.ps1
```

Linux/macOS:

```sh
sh scripts/start-web.sh
```

Open `http://localhost:4173`, then explore:

1. **Overview**: network KPIs, traffic chart, departures, activity, and
   per-train occupancy meters.
2. **Reservations**: select a train, passenger category, and seat count; submit
   to allocate a unique seat or enter the waitlist.
3. **Bookings**: filter by passenger/PNR, status, and priority; cancel a booking
   and confirm the refund/promotion toast.
4. **Train network**: open a train card and inspect its seat map; select a free
   seat to carry that exact selection into the reservation form.
5. **Concurrency lab**: choose safe/unsafe simulation and run the visual trace.
6. **Analytics**: inspect category distribution and priority ranking.
7. **Activity log**: filter browser-local events.
8. **Export reports**: download CSV booking data, occupancy SVG, HTML report,
   and a Markdown snapshot.

`Reset demo data` asks for confirmation and restores the browser sample dataset.
It does not delete or alter C++ state files.

## 20. Source evidence table

| Claim | Source |
|---|---|
| Mutual exclusion | `src/BookingManager.cpp`, `mutex_` |
| PNR/sequence | `next_pnr()` |
| Priority/FIFO | `book()` queue insertion and `cancel()` promotion |
| Capacity invariant | `confirmed_by_train_` and `capacity` |
| Seat uniqueness | occupied-label scan in `book()` and promotion |
| Condition variable | `wait_for_seats()` |
| Counting semaphore | `include/railsetu/Semaphore.hpp` |
| Persistence | `src/Persistence.cpp` |
| Restart reconstruction | `BookingManager::load()` |
| Logging | `src/Logger.cpp` |
| Stress | `src/stress.cpp` |
| UI | `src/main.cpp`, `src/ConsoleUi.cpp` |

## 21. Suggested viva sequence

1. show clean build and CTest;
2. explain mutex and critical section;
3. create booking and show PNR/seat;
4. fill/waitlist and show priority ladder;
5. cancel and show promotion/refund;
6. save, restart, and restore;
7. run unsafe race and deadlock demos;
8. run 100-thread stress;
9. open benchmark and exports;
10. explain limitations and future scope.

Recommended duration: 12–15 minutes.
