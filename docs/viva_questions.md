# RailSetu Viva Questions and Answers

## Fundamentals

### 1. What problem does RailSetu solve?

It models concurrent railway reservation under finite capacity. It ensures no
confirmed seat count exceeds capacity, no PNR or confirmed seat label duplicates,
waitlist policy is deterministic, and state can be restored.

### 2. Why is a mutex necessary?

PNR generation, capacity read, seat assignment, counter update, and map insertion
form one logical transaction. A mutex makes that transaction mutually exclusive
for threads sharing the manager.

### 3. What is the critical section here?

The lock guard around validation, PNR generation, capacity accounting,
coach-seat allocation, waitlist insertion, booking insertion, and cancellation
promotion.

### 4. Does mutex acquisition guarantee FIFO?

No. Mutexes guarantee mutual exclusion but not fair acquisition order. FIFO
waitlist fairness is implemented explicitly through monotonic booking sequence.

### 5. What is a condition variable used for?

`wait_for_seats` uses it to sleep until capacity becomes available instead of
busy-waiting. A predicate is rechecked after notification or timeout.

### 6. Why does the condition variable need a mutex?

Waiting changes sleeping state and predicate checking atomically with mutex
release/reacquisition. It also prevents a lost notification between predicate
evaluation and sleep.

### 7. What is a counting semaphore?

It is an integer permit resource protected by synchronization primitives.
`acquire` consumes one permit and `release(n)` adds permits. It represents
multiple identical resources, unlike a binary mutex with ownership semantics.

### 8. How is the semaphore used?

`src/stress.cpp` starts each worker on `gate.acquire()`. The main thread calls
`gate.release(thread_count)` after all workers are created, enabling a
coordinated start.

## Synchronization proofs

### 9. Prove the capacity invariant.

Let `C` be capacity and `U` confirmed seats. Entry satisfies `U <= C`.
For `N` seats, confirmation requires `N <= C-U`. Therefore `U+N <= C`.
All updates occur with `mutex_`, so no competing thread interleaves them.

### 10. What happens if capacity is insufficient?

The booking becomes waitlisted. It receives a PNR and sequence but no confirmed
coach-seat labels and no confirmed-counter increment.

### 11. How are seat labels allocated?

The booking logic scans coaches in insertion order and chooses the first coach
with enough free capacity. It scans seats from 1 upward and excludes labels
already assigned to confirmed bookings of that train.

### 12. Can two PNRs share a coach-seat label?

No normal confirmed allocation can because the whole map/seat scan occurs under
the manager lock and only free labels are inserted.

### 13. How is PNR uniqueness ensured?

`sequence_` is incremented while locked. `next_pnr()` formats
`RS` + eight-digit sequence and skips a PNR if already in `bookings_`.

### 14. What happens on PNR overflow or high sequence?

The code has no explicit 64-bit overflow guard; sequences in normal workload
remain far below limits. Persistence parsing rejects malformed numeric fields
using exception handling.

## Priority and FIFO

### 15. What are the priority values?

SENIOR 3, LADIES 2, TATKAL 1, GENERAL 0.

### 16. How does FIFO work?

Within equal priority, booking sequence is ascending. `sequence` is assigned
under lock. Queue insertion preserves category first and sequence second.

### 17. Can a lower-priority user ever promote before a higher-priority user?

No, if both are eligible for capacity. Promotion walks a queue reconstructed
by descending priority and ascending sequence.

### 18. What happens if the queue head needs more seats than available?

Promotion stops at that head and leaves later entries unchanged. This is strict
head-of-line behavior and is one documented limitation.

### 19. Does the project implement preemption?

No. New high-priority waitlisted entries do not demote or reallocate confirmed
passengers. Preemption is not done by `book`.

## Cancellation and refund

### 20. What occurs during confirmed cancellation?

It subtracts seats, sets 80% refund, sets UTC cancellation text, sets status
cancelled, appends history, notifies waiters, and promotes eligible entries.

### 21. What occurs during waitlist cancellation?

It erases the PNR from its deque, sets cancelled, appends history, and grants
zero refund. It does not change confirmed capacity.

### 22. Why is repeated cancellation rejected?

The manager checks the status and returns `Booking already cancelled`, preventing
refund and counter changes twice.

### 23. Why are refunds 80%?

The project explicitly configures confirmed cancellation refund to 80% of
`fare_paid`; the remaining 20% is a cancellation charge convention.

## Deadlock and races

### 24. What is a race condition?

An execution outcome depends on relative scheduling of operations on shared
state. The unsafe demo shows stale availability read/check/update sequences.

### 25. How does RailSetu prevent the real booking race?

All real state updates use one `mutex_` critical section. A thread cannot interleave
its capacity and seat update with another transaction.

### 26. How is the unsafe demo safe?

It uses isolated atomic counters and no real booking/state API. It demonstrates
logical stale-read behavior without undefined C++ shared-memory access or disk
mutation.

### 27. What is a deadlock?

Two or more threads hold resources while cyclically waiting for resources held
by each other, so none progress.

### 28. How does the deadlock demo avoid one?

It calls `std::lock(first, second)` to acquire both mutexes as one ordered
operation and uses `adopt_lock` to transfer ownership to guards.

### 29. How does the real application avoid deadlock?

It uses one manager mutex in normal booking/cancellation, and disk I/O/logging
is not performed under that mutex. There is no application lock cycle.

## Persistence

### 30. What happens during save?

The manager copies state under lock, then `Persistence::save` writes V2 text to
`.tmp`, copies prior target to `.bak`, removes target, and renames temporary.

### 31. Why is a temporary file used?

It preserves the prior target until complete serialization succeeds. A partial
temp file does not overwrite the previous valid state.

### 32. How does load validate data?

It requires a supported header, validates numeric conversion, train references,
status/category ranges, PNR/booking fields, and confirmed capacity.

### 33. What state is not persisted directly?

Derived `confirmed_by_train_` and waitlist deque order are rebuilt from raw
train/booking records. `cancellation_history` is persisted separately.

### 34. What state survives restart?

Trains, coaches, bookings, PNR sequence, category, seat labels, fare/refund,
cancelled status, waitlist members, and cancellation history.

### 35. What is V1/V2 compatibility?

V1 supports train/booking basics. V2 adds fare, coach, category, refund,
cancellation time, seat labels, and cancellation history. Loader accepts both.

## Logging and UI

### 36. Why is logger mutex-protected?

Multiple threads may log concurrently. A logger mutex serializes append writes
and avoids interleaved lines.

### 37. Is the terminal UI thread-safe?

The UI methods render data; manager queries and mutations use synchronization.
The demo’s spawned threads do not render, avoiding simultaneous terminal writes.

### 38. How are reports generated?

Manager snapshots feed occupancy, priority, HTML, Mermaid, and text writers.
SVG and HTML are derived at export time.

### 39. Are latency values measured?

The latency SVG is illustrative. Real measured stress timing is stored in CSV
and summary text.

## Stress

### 40. What does stress prove?

It empirically validates real concurrent booking calls against the 100-seat
STRESS train. It verifies no capacity overflow and no unexpected failures.

### 41. What does it not prove?

A finite test suite does not prove all possible schedules or formally verify
every state machine transition. It is empirical and repeatable evidence.

### 42. What does failure=0 mean?

The `stress.cpp` result-success condition held for every request. Waitlisted
operations are valid successes, not failures.

### 43. What is the stress start barrier?

A semaphore gate. It helps workers start together but does not replace the
manager mutex.

## Limitations

### 44. What are the project limitations?

Single-process memory/state; coarse single mutex; text persistence; no remote
authentication/API; no database constraints; no berth preferences; head-of-line
waitlist behavior; illustrative latency report.

### 45. What future changes are proposed?

SQLite/PostgreSQL transactions, cross-process file locks, REST/gRPC API,
striped locks, real wait/delay metrics, wait-for graph detection,
Shamir-like distributed consensus not needed for lab, and frontend integration.
