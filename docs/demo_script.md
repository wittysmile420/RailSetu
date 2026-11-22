# RailSetu Demonstration Script

## 1. Ten-minute run sheet

| Time | Action | Evidence |
|---:|---|---|
| 0:00–1:00 | Clean build and CTest | eight CTest targets pass |
| 1:00–2:00 | Start app and list | seeded trains and state path |
| 2:00–3:30 | Book and inspect PNR | unique PNR and seat |
| 3:30–5:00 | Show waitlist priority | SENIOR before GENERAL |
| 5:00–6:00 | Cancel/promote/refund | 80% refund and promotion |
| 6:00–7:00 | Save/restart/restore | state recovery |
| 7:00–8:00 | Race/deadlock demos | isolated and safe demonstrations |
| 8:00–9:00 | Stress test | capacity and failure output |
| 9:00–10:00 | Reports/viva closing | SVG/HTML/Mermaid and limitations |

## 2. Presenter opening

“RailSetu is a C++17 railway reservation prototype. It demonstrates mutual
exclusion, condition-variable waiting, counting semaphores, priority-aware FIFO
waitlists, atomic persistence, and an observable terminal UI.”

## 3. Build sequence

Windows:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Point to:

- C++17 in `CMakeLists.txt`;
- eight registered CTest targets;
- passing test count.

## 4. Start sequence

```powershell
.\build\railsetu_app.exe
```

At prompt:

```text
dashboard
list
seats RS101
```

Narration:

- dashboard totals are live snapshots;
- the manager mutex protects snapshots;
- seat labels come from coach data.

## 5. Booking sequence

```text
book RS101 Alice 1 SENIOR
status RS00000001
search Alice
```

Narration:

- PNR is assigned under lock;
- capacity and seat label are updated in one critical section;
- category is part of waitlist policy;
- fare is train base fare times seats.

## 6. Priority sequence

Keep Alice confirmed or fill RS101. Then issue:

```text
book RS101 GeneralPassenger 1 GENERAL
book RS101 SeniorPassenger 1 SENIOR
ladder RS101
```

If capacity is full, both requests waitlist. The priority ladder must show:

```text
SENIOR before GENERAL
```

Within category, sequence is the FIFO key.

## 7. Cancellation sequence

For confirmed PNR:

```text
cancel RS00000001
refund RS00000001
status RS00000001
history
```

For waitlisted PNR:

```text
cancel RS00000002
status RS00000002
```

Narration:

- cancel subtracts confirmed seats;
- it computes 80% confirmed refund;
- waitlist cancel removes and refunds zero;
- promotion happens in the same manager critical section.

## 8. Persistence sequence

```text
save
backup
exit
```

Restart:

```powershell
.\build\railsetu_app.exe
```

```text
status RS00000001
history
restore
list
```

Narration:

- state uses V2 text and adjacent `.tmp`;
- prior state is copied to `.bak`;
- load rebuilds counters and waitlists;
- sequence is preserved.

## 9. OS demo sequence

```text
unsafe-race
deadlock
```

Explain unsafe race is isolated. It uses local atomic counters and never touches
real state. It illustrates stale read/check/write behavior.

Explain deadlock uses `std::lock(first, second)` and `adopt_lock`.
The real application uses one manager lock and does not acquire it while
performing disk I/O.

## 10. Stress sequence

Windows:

```powershell
.\build\railsetu_stress.exe 100 20
```

Linux:

```sh
./build/railsetu_stress 100 20
```

Expected shape:

```text
threads=100 requested=2000 confirmed=100 waitlisted=1900 failures=0
```

The exact elapsed ms is machine-dependent.

## 11. Reports sequence

```text
export
```

Open:

```text
reports/railsetu_report.html
reports/occupancy.svg
reports/architecture.mmd
benchmark/benchmark_summary.txt
```

Explain that occupancy and priority exports reflect live state; the latency
graphic is illustrative and the CSV stores actual benchmark rows.

## 12. Closing evaluator statements

The presenter should state:

- the mutex proves capacity safety;
- the condition variable provides availability waits;
- the semaphore coordinates benchmark threads;
- waitlist fairness is explicit;
- persistence is crash-oriented, not a distributed database;
- the unsafe demo is isolated;
- limitations include single-process state and coarse lock granularity.
