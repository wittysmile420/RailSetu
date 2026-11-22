# RailSetu User Manual

## 1. Starting the application

Run from the workspace root.

### Windows MinGW build

```powershell
.\build\railsetu_app.exe
```

Custom state and log paths:

```powershell
.\build\railsetu_app.exe demo.state logs\demo.log
```

Arguments are positional:

1. persistence state path;
2. activity log path.

### Linux/macOS build

```sh
./build/railsetu_app
./build/railsetu_app demo.state logs/demo.log
```

On startup, the UI clears the terminal, prints the banner, startup diagnostics,
and menu.

## 2. Terminal behavior

ANSI colors are used where detected. Unix-like systems can disable them with:

```sh
NO_COLOR=1 ./build/railsetu_app
```

Windows terminal color depends on `TERM`, `WT_SESSION`, or `ANSICON`.
Plain text remains readable if colors are not available.

## 3. Menu options

| Option | Name | Action |
|---:|---|---|
| 1 | Search trains | Prompts for a train ID and lists the matching train |
| 2 | List trains | Displays all trains, capacities, and waitlists |
| 3 | Train details | Shows capacity, fare, and coaches |
| 4 | Search seats | Prompts for train ID and available seat labels |
| 5 | Book ticket | Interactive name, seats, and category prompts |
| 6 | Concurrent booking | Creates 100 threads against a DEMO train of 20 seats |
| 7 | Cancel | Prompts for PNR and confirmation |
| 8 | PNR lookup | Shows full PNR details |
| 9 | Confirmed bookings | Interactive passenger search |
| 10 | Waiting list | Displays priority ladder for a train |
| 11 | Promote manually | Informational: promotion is cancellation-driven |
| 12 | Remove waiting PNR | Cancels/waits as a regular PNR |
| 13 | Real-time seat map | Shows confirmed seat labels |
| 14 | Activity/cancellation | Shows history and bounded recent log |
| 15 | Stress test | Prints stress executable command |
| 16 | Fairness test | Prints priority and FIFO policy |
| 17 | Benchmark suite | Prints stress command and local ASCII chart |
| 18 | Deadlock prevention | Runs isolated ordered-lock demo |
| 19 | Unsafe race demo | Runs isolated stale-read counter demo |
| 20 | Export report | Writes text, SVG, HTML, and Mermaid |
| 21 | Reset demo state | Attempts restore of `.bak`, then manual restart advice |
| 22 | Exit | Confirms and saves state |
| 23 | Live dashboard | Aggregate totals and per-train occupancy |
| 24 | Route | Prints a generic route visualization |
| 25 | Waitlist ladder | Prompts for train ID |
| 26 | Log viewer | Shows the final 20 log lines |
| 27 | Captured demo help | Lists suggested captured commands |

## 4. Command syntax

Commands are parsed case-sensitively. Category names and PNRs should be entered
exactly in uppercase style.

| Command | Syntax | Example |
|---|---|---|
| list | `list` | `list` |
| coach | `coach TRAIN COACH TYPE SEATS` | `coach RS101 C1 AC 3` |
| seats | `seats TRAIN` | `seats RS101` |
| book | `book TRAIN NAME SEATS [CATEGORY]` | `book RS101 Alice 1 SENIOR` |
| wait | `wait TRAIN SEATS TIMEOUT_MS` | `wait RS101 1 10000` |
| cancel | `cancel PNR` | `cancel RS00000001` |
| refund | `refund PNR` | `refund RS00000001` |
| status | `status PNR` | `status RS00000001` |
| search | `search NAME` | `search Alice` |
| history | `history` | `history` |
| save | `save` | `save` |
| backup | `backup` | `backup` |
| restore | `restore` | `restore` |
| unsafe-race | `unsafe-race` | `unsafe-race` |
| deadlock | `deadlock` | `deadlock` |
| dashboard | `dashboard` | `dashboard` |
| route | `route` | `route` |
| ladder | `ladder TRAIN` | `ladder RS101` |
| log | `log` | `log` |
| export/report | `export`, `report`, `svg`, `html`, or `mermaid` | `export` |
| exit | `exit` or `quit` | `exit` |

Names containing spaces must be entered as a quoted/continuous sequence after
the train ID in command form. The parser joins remaining name tokens before it
extracts the final seat number and optional category.

## 5. Categories and priority

| Category | Numeric priority | Promotion behavior |
|---|---:|---|
| SENIOR | 3 | First among eligible categories |
| LADIES | 2 | Before TATKAL and GENERAL |
| TATKAL | 1 | Before GENERAL |
| GENERAL | 0 | Lowest priority |

Within the same category, entries are promoted by monotonically assigned booking
sequence, not by list position or mutex scheduling.

## 6. Coach and seat allocation

At startup:

| Train | Coach | Type | Seat count |
|---|---|---|---:|
| RS101 | A1 | AC | 60 |
| RS101 | S1 | SLEEPER | 60 |
| RS205 | G1 | GENERAL | 80 |
| RS330 | A1 | AC | 60 |

The booking logic selects the first coach that can hold the complete request,
then assigns lowest currently free seat labels. Confirmed booking seat labels
are stored in `coach-seat` form, for example `A1-1`.

The CLI `coach` command adds a coach but does not move or delete existing
coaches. It rejects an empty coach ID or zero seat count.

## 7. Reservation workflow

```text
1. list
2. seats RS101
3. book RS101 Name 1 SENIOR
4. status RS00000001
5. cancel RS00000001
6. history
```

For a confirmed booking:

- fare = train base fare × requested seats;
- PNR is unique;
- coach and seat labels are assigned;
- confirmed count increases.

For a waitlisted booking:

- confirmed count does not increase;
- PNR is stored in the train's priority deque;
- no coach-seat labels are assigned;
- cancellation can promote it later.

## 8. Cancellation and refunds

`cancel PNR`:

- returns an error for unknown or already cancelled PNR;
- confirmed cancellation refunds 80% of `fare_paid`;
- waitlist cancellation refunds zero;
- stores UTC cancellation text and history;
- promotes eligible queue entries immediately.

Promotion is automatic and occurs while the manager mutex is held. It stops at
the first waitlisted request that cannot fit. This preserves queue-head
consistency.

## 9. Condition-variable waiting

```text
wait RS101 1 10000
```

- timeout is milliseconds;
- a successful wait means an availability predicate is true at wakeup;
- it does not create a reservation;
- timeout returns `Timed out`;
- cancellation or confirmation triggers notifications.

Use a second terminal or menu 7 cancellation to demonstrate a release before
timeout.

## 10. Persistence

Default runtime state:

```text
data/railsetu.state
data/railsetu.state.tmp
data/railsetu.state.bak
```

Default log:

```text
logs/activity.log
```

`save` writes the V2 format. `backup` copies state to `.bak`.
`restore` renames `.bak` back to the state file and reloads it.
Every normal mutation can save automatically; menu 22 calls a final save.

Do not edit a live `.state` file while the application is running.

## 11. Exports

Menu 20 or `export` creates:

- `benchmark/railsetu_report.txt`
- `reports/occupancy.svg`
- `reports/latency.svg`
- `reports/priority.svg`
- `reports/railsetu_report.html`
- `reports/architecture.mmd`

Open HTML in a browser. Open Mermaid in VS Code or GitHub.
Re-run export after booking/cancellation changes.

## 12. Stress and benchmarking

### Windows

```powershell
.\build\railsetu_stress.exe 64 10
.\build\railsetu_stress.exe 100 20
```

### Linux

```sh
./build/railsetu_stress 64 10
./build/railsetu_stress 100 20
```

The benchmark writes:

- `benchmark/benchmark.csv`
- `benchmark/benchmark_summary.txt`
- `logs/stress_test.log`

Arguments are `thread_count` and `bookings_per_thread`.

## 13. Troubleshooting

| Problem | Diagnosis | Resolution |
|---|---|---|
| CMake generator error on Windows | g++/CMake environment mismatch | Reopen PowerShell and verify `cmake --version`, `g++ --version`; use MinGW command |
| `g++ was not found` | Compiler not on PATH | Activate MSYS2 UCRT64 or install MinGW, or use Visual Studio |
| Executable not in `build` | Built with Visual Studio/Ninja | Check `build\Debug` or `build\Release` |
| State load fails | Header/numeric/reference corruption | Restore `.bak`, or delete state only if seed reset is acceptable |
| PNR not found | PNR is absent or command was typed wrong | Use `search NAME` or `list`/history |
| No available seats | Coach labels occupied or no coaches | Add coach or cancel PNR |
| Waitlist not promoted | Remaining capacity cannot fit head entry | Cancel enough/coherent seats and recheck |
| Export missing | Export command not run or working directory wrong | Run from project root and invoke menu 20/export |
| ANSI text looks wrong | Unsupported terminal variable | Set a compatible terminal or NO_COLOR |
| Stress executable returns 1 | Invariant or unexpected failure | Read summary/log and rerun CTest |

## 14. Safe reset procedure

1. Export or backup important work.
2. Exit the application.
3. Rename or remove demonstration state:

```powershell
Remove-Item demo.state, demo.state.bak, demo.state.tmp -ErrorAction SilentlyContinue
```

4. Restart. No state means seeded trains are recreated.
