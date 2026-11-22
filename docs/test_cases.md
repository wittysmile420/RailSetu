# RailSetu Test Cases and Validation

## 1. Test philosophy

Tests use C++ assertions and CTest process exit status. A passing executable
returns zero; an assertion abort or explicit non-zero return is failure.
Tests are focused on invariants, synchronization behavior, persistence, and UI
snapshots.

## 2. Build tests

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Linux:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 3. CTest matrix

| Test executable | Source | Primary cases |
|---|---|---|
| `railsetu_tests` | `tests/test_booking.cpp` | Core booking, cancellation, persistence, malformed state, semaphore, seat labels, refund, demos |
| `test_concurrent_booking` | `tests/test_concurrent_booking.cpp` | 20 threads × one seat against capacity 20 |
| `test_cancellation` | `tests/test_cancellation.cpp` | promotion, repeated cancellation rejection |
| `test_waiting_promotion` | `tests/test_waiting_promotion.cpp` | senior over general, retained general |
| `test_semaphore` | `tests/test_semaphore.cpp` | blocking/release, try-acquire |
| `test_persistence` | `tests/test_persistence.cpp` | save/load restart |
| `test_race_validation` | `tests/test_race_validation.cpp` | safe ten-seat load, isolated race, deadlock |
| `test_visual_features` | `tests/test_visual_features.cpp` | waitlist, all bookings, seat availability |

## 4. Functional acceptance cases

| ID | Setup | Action | Expected | Invariant |
|---:|---|---|---|---|
| F01 | T1 capacity 2 | Alice/Bob confirm | 2 successes | confirmed=2 |
| F02 | T1 full | Carol requests | waitlisted | confirmed=2, wait=1 |
| F03 | confirmed Alice and wait Carol | cancel Alice | Carol confirms | confirmed=2, wait=0 |
| F04 | unknown PNR | cancel | false | no state change |
| F05 | cancelled PNR | cancel again | false | no second history/refund |
| F06 | request 0 seats | book | false | no PNR, no counter |
| F07 | passenger empty | book | false | no PNR |
| F08 | unknown train | book | false | no PNR |
| F09 | duplicate train ID | add train | false | only first record |
| F10 | no coaches | confirmed request | confirmed no coach | capacity still enforces |
| F11 | C1 capacity 2 | allocate two | unique C1-1/C1-2 | no duplicate label |
| F12 | C1 full | third request | waitlisted | no seat label |

## 5. Priority/FIFO cases

| ID | Setup | Expected |
|---:|---|---|
| P01 | GENERAL then SENIOR | senior promotes first |
| P02 | SENIOR S1 then SENIOR S2 | S1 promotes before S2 |
| P03 | LADIES before TATKAL | LADIES priority 2 wins |
| P04 | TATKAL before GENERAL | TATKAL priority 1 wins |
| P05 | three categories | rank SENIOR > LADIES > TATKAL > GENERAL |
| P06 | waitlist reload | sorting by priority then sequence reconstructs queue |
| P07 | general head, senior later | new senior insertion can precede general |

The strict tie-break is:

```text
lp != rp ? lp > rp : left.sequence < right.sequence
```

## 6. Cancellation cases

| ID | State | Expected refund | Promotion |
|---:|---|---:|---|
| C01 | Confirmed, fare 100, seats 1 | 80 | eligible queue head |
| C02 | Confirmed, fare 500, seats 2 | 800 | same 80% total |
| C03 | Waitlisted | 0 | removes PNR |
| C04 | Cancelled twice | no new refund | no duplicate history |
| C05 | Unknown PNR | 0 | no state change |

The implementation computes `fare_paid * 80 / 100` for confirmed cancellation.

## 7. Persistence cases

| ID | Setup | Action | Expected |
|---:|---|---|---|
| D01 | valid V2 state | save/reload | identical booking and train state |
| D02 | valid V1 state | load | accepted train-only legacy format |
| D03 | malformed train capacity | load | false |
| D04 | unknown booking train | load | false |
| D05 | invalid status integer | load | false |
| D06 | invalid category integer | load | false |
| D07 | confirmed seats exceed capacity | load | false |
| D08 | missing `.bak` | restore | false |
| D09 | backup exists | restore | state reloads |
| D10 | `.tmp` close failure | save | false; prior state remains |

Load does not install state before validation completes.

## 8. Semaphore cases

| ID | State | Action | Expected |
|---:|---|---|---|
| S01 | count 0 | acquire | block |
| S02 | releaser release 1 | acquire | unblock and decrement |
| S03 | count 0 | try_acquire | false |
| S04 | count 1 | try_acquire | true; count 0 |
| S05 | count near max | release overflow | `overflow_error` |

## 9. Concurrent test cases

### 20-seat exact capacity

Source: `tests/test_concurrent_booking.cpp`

```text
20 threads
1 seat per thread
train capacity 20
```

Expected confirmed = 20 and waitlist = 0.

### 100-thread stress

Source: `src/stress.cpp`

```text
100 threads
20 requests per thread
STRESS capacity 100
```

Expected confirmed = 100, waitlisted = 1900, failures = 0.

### 64-thread medium stress

```text
64 × 10 = 640 requests
capacity 100
```

Expected confirmed = 100, waitlisted = 540, failures = 0.

## 10. Race/deadlock cases

| ID | Action | Expected |
|---:|---|---|
| R01 | 10 safe bookings under lock | confirmed 10; no duplicate seat/PNR |
| R02 | unsafe demo 100 operations | completed in isolated state |
| R03 | repeated unsafe demo | real train unchanged |
| R04 | deadlock demo | returns true/completes |

The unsafe demo is not a full physical race model. It uses atomic counters to
demonstrate stale-check semantics safely.

## 11. Visual snapshot cases

| ID | Setup | Expected |
|---:|---|---|
| V01 | confirmed and waitlist | waitlist_entries size = 1 |
| V02 | two bookings | all_bookings size = 2 |
| V03 | full single coach | available_seats empty |
| V04 | waiting ladder | entries sorted in waitlist order |

## 12. Stress methodology

1. Start with a fresh process and train named `STRESS`.
2. Persistence is disabled for the stress manager.
3. Create counting semaphore with zero permits.
4. Spawn each thread.
5. Release exactly `thread_count` permits.
6. Record steady-clock elapsed time after release.
7. Run real `book()` calls.
8. Join all threads.
9. Read confirmed/waitlisted totals.
10. Write CSV, summary, and log.
11. Return failure if capacity is exceeded or a real booking call fails.

The gate is not a claim of simultaneous wall-clock instruction execution.
It is a controlled release that reduces staggered startup bias.

## 13. Existing benchmark evidence

The repository currently contains records including:

| Threads | Requested | Confirmed | Waitlisted | Elapsed ms |
|---:|---:|---:|---:|---:|
| 20 | 100 | 100 | 0 | 84 |
| 64 | 640 | 100 | 540 | 588–793 |
| 100 | 2000 | 100 | 1900 | 1964–3036 |

The latest `benchmark_summary.txt` reports 2448 ms for 100×20 in the
most recent MinGW validation run. Timing is hardware and scheduling dependent.

## 14. Troubleshooting test failures

| Failure | Check |
|---|---|
| concurrent confirmed != capacity | verify mutex lock scope and train ID |
| waitlist missing | confirm capacity is actually full |
| priority wrong | verify category enum and priority function |
| FIFO wrong | verify sequence is loaded and not reset |
| persistence missing | verify save succeeded and state path |
| malformed state accepted | verify header and numeric range validation |
| test target missing | regenerate CMake build directory |
| Windows executable missing | check MinGW `build\` path vs Release path |
