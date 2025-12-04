Nudge — Simple C++ CLI TODO/Reminder App

Overview
- Small command-line TODO/reminder application written in C++ (C++23).
- Uses SQLite3 to persist tasks and completed items.

Compiler and language
- Requires a compiler that supports C++23.

Build (CMake)
1. Configure and build:
```bash
cmake -S . -B build
cmake --build build -j 4
```

Run
- Add a task:
```bash
./build/Nudge add "My new task"
```
- Delete a task by id:
```bash
./build/Nudge delete 3
```
- List pending tasks:
```bash
./build/Nudge list
```
- List all tasks (pending + completed):
```bash
./build/Nudge list -a
```
- List completed tasks:
```bash
./build/Nudge list -c
```
- Mark a task complete by id (or mark the first pending task if no id given):
```bash
./build/Nudge complete 5
./build/Nudge complete
```

- Mark tasks by pattern (non-numeric argument defaults to substring match):
```bash
./build/Nudge complete "task"  
./build/Nudge complete LIKE "demo" 
```

How pattern completion works
- If you run `./build/Nudge complete "text"` and `text` is not a number, the application treats it as a substring pattern and executes a SQL `WHERE task LIKE '%text%'` to find matching tasks. All matching rows are inserted into the `completed` table and removed from `tasks` within a single transaction. Example:

1. Before: `tasks` contains rows with task values `"Finish the demo"`, `"Read task docs"`.
2. Run: `./build/Nudge complete "task"`.
3. Result: Any task containing `task` (like `Read task docs`) is moved to `completed` and deleted from `tasks`.

- Show a desktop notification with pending task count (macOS and Linux only):
```bash
./build/Nudge notification users
```
On macOS, uses `osascript` for native notifications. On Linux, uses `notify-send` (requires libnotify-bin). Falls back to console output on other platforms or if notification tools are unavailable.


Notes
- The application stores timestamps using the device's local timezone (SQLite stores timestamps with the `datetime('now','localtime')` expression).
- The database file is created at runtime under the current user's configuration directory defined in the application (see `paths.hpp`); check that file to find the exact path (commonly `~/.nudge/` on UNIX-like systems).

