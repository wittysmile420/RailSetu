# RailSetu Team Contributions

All team members should understand the complete implementation. Ownership below
identifies primary review, demo, and viva responsibility.

## Team table

| Member | Roll number | Batch | Primary area |
|---|---:|---|---|
| Avi Sharma | 992401030388 | F-9 | Domain model and seat allocation |
| Rhythm Jain | 992401030410 | F-9 | Synchronization and waitlists |
| Sarthak Suman V. | 992401030408 | F-9 | Persistence, tests, benchmarks |
| Harshit Chauhan | 992401030414 | F-9 | Terminal UI, demos, documentation |

## Avi Sharma

### Ownership

- `include/railsetu/Types.hpp`;
- train, coach, booking, status, category structures;
- seed train/coaches;
- `add_train`, `add_coach`, seat-label selection;
- uniqueness of confirmed coach-seat labels.

### Implementation evidence

Inspect `Types.hpp` for `Coach`, `Train`, `Booking`, `BookingStatus`,
`PassengerCategory`, and priority function. Inspect `add_coach()` and both
allocation branches in `src/BookingManager.cpp`.

### Viva responsibilities

- explain domain modeling;
- derive the seat allocation invariant;
- explain coach-label format;
- distinguish confirmed, waitlisted, and cancelled status;
- explain fare and refund fields.

### Demonstration

```text
list
seats RS101
coach RS101 C1 AC 3
book RS101 Avi 1 SENIOR
status RS00000001
```

Point to the coach and seat label in the ticket/status output.

## Rhythm Jain

### Ownership

- `mutex_`, `seats_cv_`, and critical sections;
- priority queue insertion;
- cancellation promotion;
- `include/railsetu/Semaphore.hpp`;
- condition-variable waiting;
- concurrency, FIFO, semaphore tests.

### Implementation evidence

Review `BookingManager::book`, `cancel`, `wait_for_seats`, and `next_pnr` in
`src/BookingManager.cpp`. Review `test_waiting_promotion.cpp`,
`test_concurrent_booking.cpp`, and `test_semaphore.cpp`.

### Viva responsibilities

- explain why mutex is needed;
- prove capacity invariant;
- explain why mutex scheduling is not FIFO;
- explain priority + sequence tie-break;
- compare mutex, condition variable, and semaphore;
- explain predicate rechecking and notify-all.

### Demonstration

```text
book RS101 GeneralPassenger 1 GENERAL
book RS101 SeniorPassenger 1 SENIOR
ladder RS101
cancel RS00000001
status RS00000002
```

The senior entry must precede general and promotes before it.

## Sarthak Suman V.

### Ownership

- `src/Persistence.cpp`;
- `include/railsetu/Persistence.hpp`;
- V1/V2 parser and validation;
- `.tmp/.bak` protocol;
- `src/Logger.cpp`;
- CMake/CTest test registration;
- benchmark stress and test evidence.

### Implementation evidence

Review `Persistence::save/load/backup/restore`, `BookingManager::save/load`,
`src/stress.cpp`, `CMakeLists.txt`, and `tests/test_persistence.cpp`.

### Viva responsibilities

- explain derived-state reconstruction;
- explain atomic write protocol;
- explain backup/restore;
- explain malformed state rejection;
- explain benchmark invariant and stress methodology;
- explain logger serialization.

### Demonstration

```text
save
backup
exit
```

Restart with the same state path. Then:

```text
list
status RS00000001
history
restore
```

Run:

```powershell
.\build\railsetu_stress.exe 100 20
```

Show `benchmark/benchmark_summary.txt`, `benchmark/benchmark.csv`, and
`logs/stress_test.log`.

## Harshit Chauhan

### Ownership

- `src/main.cpp`;
- `src/ConsoleUi.cpp`;
- command parsing and prompts;
- 27 menu options;
- report exports;
- demo/evaluator/screenshot/viva documentation;
- invalid-input and reset workflow.

### Implementation evidence

Review menu dispatch in `src/main.cpp`, `ConsoleUi` APIs, export writer,
and `docs/evaluator_user_guide.md`.

### Viva responsibilities

- explain CLI-to-manager call flow;
- explain why UI is not authoritative;
- explain ANSI fallback;
- explain unsafe/deadlock demo isolation;
- explain report formats and limitations;
- explain invalid numeric input handling.

### Demonstration

```text
dashboard
route
ladder RS101
unsafe-race
deadlock
export
log
```

Open HTML and Mermaid exports.

## Shared delivery checklist

Before evaluation:

1. remove stale build directory;
2. run the exact MinGW build commands;
3. run CTest and verify eight targets;
4. run 100×20 stress;
5. verify benchmark and log files;
6. run one booking/cancel/promotion/restart flow;
7. run export and open HTML;
8. confirm no unfinished marker, secret, merge marker, or unresolved draft text remains;
9. assign a presenter for each demo section;
10. review limitations and future scope.

## Shared source checklist

| Topic | Primary files |
|---|---|
| synchronization | `include/railsetu/BookingManager.hpp`, `src/BookingManager.cpp` |
| priority/seats | `include/railsetu/Types.hpp`, `src/BookingManager.cpp` |
| persistence | `include/railsetu/Persistence.hpp`, `src/Persistence.cpp` |
| logs | `include/railsetu/Logger.hpp`, `src/Logger.cpp` |
| UI | `src/main.cpp`, `src/ConsoleUi.cpp` |
| stress | `src/stress.cpp`, `include/railsetu/Semaphore.hpp` |
| tests | `tests/*.cpp`, `CMakeLists.txt` |
| evidence | `benchmark/*`, `reports/*`, `docs/*` |
