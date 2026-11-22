# RailSetu Synopsis

## Title

**RailSetu: A Multi-threaded Railway Reservation System with
Synchronization-Safe Seat Allocation**

## Institution

Jaypee Institute of Information Technology, Sector 128  
Operating Systems Laboratory  
Supervisor: Mr. Satya Prakash Patel

## Team

| Member | Roll number | Batch |
|---|---:|---|
| Avi Sharma | 992401030388 | F-9 |
| Rhythm Jain | 992401030410 | F-9 |
| Sarthak Suman V. | 992401030408 | F-9 |
| Harshit Chauhan | 992401030414 | F-9 |

## Abstract

Railway reservation is a concurrent allocation problem. Passenger threads can
request identical finite seat resources, while cancellations and waitlist
promotions change capacity. A check followed by an update outside a critical
section can cause lost updates and duplicate allocations.

RailSetu implements the reservation domain in C++17. `BookingManager` uses a
single mutex to serialize PNR assignment, capacity validation, coach-seat
selection, waitlist insertion, cancellation, promotion, and refund accounting.
A condition variable supports availability waiting, and a counting semaphore
coordinates the stress workload.

The project adds priority-aware FIFO waitlists, V1/V2 state loading, atomic
temporary-file persistence with a backup, structured activity logging,
coach-seat visualization, benchmark CSV and text reports, and SVG/HTML/Mermaid
exports.

## Problem statement

Given a finite train capacity and concurrent passenger requests:

1. no confirmed total may exceed capacity;
2. no coach-seat label may be assigned twice;
3. PNR values must remain unique;
4. waitlist entries must preserve priority and FIFO semantics;
5. cancellation must release exact seats and promote eligible entries;
6. state must survive an application restart.

The mutex implementation provides safety for these invariants. The stress
executable provides empirical evidence under repeated concurrent load.

## Objectives

- demonstrate mutual exclusion and critical sections;
- demonstrate condition-variable predicate waits;
- compare mutex and counting semaphore behavior;
- allocate real coach-seat labels under concurrency;
- implement priority tiers and equal-category FIFO;
- promote eligible waitlist entries after cancellation;
- implement confirmation/refund/cancellation history;
- persist and restore state safely;
- record repeatable stress and benchmark evidence;
- provide a terminal UI and evaluator-ready reports.

## Scope

RailSetu is a single-process C++ laboratory prototype. It supports:

- in-memory trains, coaches, and bookings;
- one thread-safe manager instance;
- standard input terminal interaction;
- direct C++ tests and CTest;
- local text-file persistence;
- local benchmark and log artifacts.

It does not implement authentication, payment processing, distributed
process coordination, SQL transactions, or remote network clients.

## Core synchronization design

For request count `N` and confirmed count `U`:

```text
confirm only if N <= train.capacity - U
U' = U + N
require U' <= train.capacity
```

The read and update happen under `mutex_`. Cancellation subtracts seats and
promotion recalculates capacity inside the same critical section.

## Waitlist policy

| Category | Priority |
|---|---:|
| SENIOR | 3 |
| LADIES | 2 |
| TATKAL | 1 |
| GENERAL | 0 |

Higher priority wins. Equal priority uses monotonically increasing booking
sequence. Loading reconstructs this order by sorting waitlisted records.

## Persistence and recovery

The state file is V2 text. Save writes:

1. adjacent `.tmp`;
2. close and verify;
3. copy target to `.bak`;
4. replace target through rename.

Load validates header, train references, status/category fields, PNR records,
and capacity overflow.

## Expected outcomes

- stress requests stay at or below train capacity;
- failed normal booking calls remain zero under the benchmark;
- PNR and seat labels remain unique;
- waitlist order survives restart;
- cancellation promotes FIFO/priority entries;
- CTest reports pass.

## Contributions

Team area ownership is detailed in
[team_contributions.md](team_contributions.md). Core ownership is:

- Avi Sharma: domain model and seats;
- Rhythm Jain: synchronization and waitlists;
- Sarthak Suman V.: persistence, logs, tests, benchmarks;
- Harshit Chauhan: terminal UI and documentation/demos.
