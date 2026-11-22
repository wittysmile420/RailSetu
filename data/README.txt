RailSetu data directory

The JSON files are portable seeded examples for documentation and reset operations.
The running application uses a versioned atomic state file selected at startup.
Runtime state files and backups are ignored by Git.

trains.json       Seed train, route, fare, and coach information
bookings.json     Empty confirmed/cancelled booking seed
waiting_list.json Empty FIFO waiting-list seed
cancellation_history.json Empty cancellation/refund history seed
