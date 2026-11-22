# RailSetu Screenshot and Terminal Capture Guide

## 1. Capture policy

The repository should not depend on opaque binary images. Presentation captures
can be pasted directly from the terminal into slides or saved as text. The
examples below show exact information a reviewer should expect, not fabricated
live PNR values.

## 2. Windows capture setup

Use Windows Terminal or PowerShell 7 and set the window to at least 100×35
columns/rows. Build and run:

```powershell
.\scripts\build-and-test.ps1
.\build\railsetu_app.exe
```

Use menu or commands while recording terminal output. Windows Terminal supports
screen capture and text selection. Save captures in a submission folder outside
source paths if required.

## 3. Linux capture setup

```sh
./scripts/build.sh
./build/railsetu_app
```

Use terminal selection or `script` for a text transcript. ANSI escape sequences
can be disabled:

```sh
NO_COLOR=1 ./build/railsetu_app
```

## 4. Startup capture

Commands:

```text
dashboard
list
```

Expected form:

```text
  [STARTUP] C++17 | Standard library only | Mutex/CV/Semaphore
  [STARTUP] State=data/railsetu.state | Log=logs/activity.log
  [STARTUP] Invariant: confirmed seats <= capacity | FIFO priority waitlist

  LIVE DASHBOARD
  +----------------+----------------+----------------+----------------+
  | TRAINS 3       | CONFIRMED 0    | WAITLIST 0     | BOOKINGS 0     |
  +----------------+----------------+----------------+----------------+
```

Counts vary when a state file already contains bookings.

## 5. Seat grid capture

```text
seats RS101
```

Expected form:

```text
  -- AVAILABLE SEATS --
  Legend: [C] confirmed [R] reserved [P] promotion [. ] available
  [C:A1-1] [C:A1-2] [. ]
```

The grid shows only confirmed labels in the current implementation. Available
labels are printed beneath and coach details are printed by train details.

## 6. Booking ticket capture

```text
book RS101 Alice 1 SENIOR
status RS00000001
```

Expected form:

```text
  -- RAILSETU E-TICKET --
  +==============================================================+
  | PNR  RS00000001                                  RAILSETU |
  +--------------------------------------------------------------+
  | Passenger : Alice
  | Train     : RS101 / Rajdhani Express
  | Route     : Origin ===> Destination
  | Coach     : A1
  | Seats     : A1-1
  | Category  : SENIOR
  | Status    : CONFIRMED
  | Fare      : INR 100
  +==============================================================+
```

The PNR is illustrative for a state that starts at sequence zero.

## 7. Waitlist ladder capture

Fill capacity or inspect an existing waitlisted train:

```text
ladder RS101
```

Expected category hierarchy:

```text
  -- WAITLIST PRIORITY LADDER --
  #1 [SENIOR] RS00000004 SeniorPassenger
  #2 [LADIES] RS00000005 LadiesPassenger
  #3 [TATKAL] RS00000006 TatkalPassenger
  #4 [GENERAL] RS00000007 GeneralPassenger
```

PNRs and passengers vary. Equal-priority entries appear in sequence/FIFO order.

## 8. Cancellation and history capture

```text
cancel RS00000001
refund RS00000001
history
```

Expected form:

```text
  [OK] Booking cancelled
  [INFO] Refund INR 80
  2026-...Z | RS00000001 | Alice | Refund INR 80
```

The exact UTC timestamp comes from the system clock.

## 9. Persistence capture

```text
save
backup
exit
```

Restart and:

```text
status RS00000001
list
```

Expected evidence:

- PNR remains in state;
- confirmed counters equal loaded booking totals;
- waitlist entries appear in reconstructed order;
- `data/railsetu.state.bak` is available after backup.

## 10. Stress capture

```powershell
.\build\railsetu_stress.exe 100 20
```

Representative shape:

```text
threads=100 requested=2000 confirmed=100 waitlisted=1900 failures=0 elapsed_ms=2663
```

Elapsed milliseconds are non-deterministic. The repository currently stores
benchmark rows in `benchmark/benchmark.csv`.

## 11. Report captures

Run:

```text
export
```

Capture browser output:

- `reports/railsetu_report.html`
- occupancy SVG
- priority SVG
- architecture Mermaid

Capture shell output:

```text
[OK] Export complete: text, SVG, HTML, and Mermaid reports
```

## 12. Race/deadlock captures

```text
unsafe-race
deadlock
```

Expected:

```text
  Unsafe result=<count> (isolated; real state untouched)
  [OK] Circular-wait demo avoided; ordered lock acquisition completed
```

The unsafe count varies; it must not exceed the isolated simulation bound.

## 13. Slide layout recommendation

Use four slides:

1. architecture/locking diagram;
2. booking and ticket evidence;
3. waitlist/cancellation/persistence evidence;
4. stress/export and test evidence.

Add captions that identify source file and command. Do not crop out the PNR,
confirmed/waitlisted totals, or test result.
