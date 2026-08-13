# BatchManager
## Overview
BatchManager runs multiple command-line processes in batch while keeping the host machine within safe operating limits. Rather than launching everyprocess at once, BatchManager queues them and starts, pauses ("freezes"), or resumes them automatically so that the total number of running processes and the total memory in use never exceed the limits you set.
This is useful when you have many long-running jobs (simulations, batch conversions, solver runs, etc.) that would otherwise overwhelm the system's CPU or RAM if launched simultaneously.

<img width="1920" height="1032" alt="image" src="https://github.com/user-attachments/assets/21179092-449f-44ff-9b83-6b2746299450" />

## Adding processes
Processes are added to BatchManager by drag and drop onto the process list, where they appear with status Pending. Two types of files are supported:
- Drag an executable or batch file (.exe, .bat) directly — it is added and run as-is.
- Drag a data file whose type is registered in Managed file types (see below) — BatchManager looks up the file's name and extension, and if it matches a registered pattern, launches the associated command with the associated arguments, using the dropped file's path to fill in the placeholders. This lets you, for example, drop a .dat file and have BatchManager automatically run the right solver on it, without having to build thecommand line by hand each time.

## Main window layout
The window is split into two halves:
- Left — Process list: every process BatchManager knows about, its current status, and its resource usage.
- Right — Process detail: the text output and memory-usage graph for whichever process is currently selected in the list.

## Process list (left grid)

| Column | Description | 
|---|---|
| Status | Current state of the process — see Process states below |
| File | Name of the executable/command being run |
| Args | Arguments of the comand |
| Folder | Working folder for the process. |
| Start | Time the process started running. |
| End | Time the process finished (blank while running). |
| MaxTime | Maximum wall-clock time allowed for the process before it is stopped automatically. |
| Time | Elapsed run time so far. |
| Mem Used/RAM | Current memory in use by the process — shown as allocated / physical RAM. |
| Mem Max | Peak memory the process has allocated since it started. |


## Process states
- Pending — queued, waiting for a free process/memory slot before it can start.
- Running — actively executing.
- Paused — was running, but has been frozen by BatchManager because a constraint (process count or memory) was exceeded; it will resume automatically once resources free up.
- Ended — finished (successfully or not).

## Toolbar (below the process list)
- Open Folder — opens the working folder of the selected processes in the file explorer.
- End — stops/terminates the selected processes.
- ReStart — runs the selected processes again from the start.
- Delete — removes the selected processes from the list.
- Delete Ended — removes all processes with status Ended from the list, in one action.

## Priority
The up/down/top/bottom arrow buttons next to the toolbar move the selected process up or down in the list. A process's position in the list determines its priority: when a slot frees up, BatchManager starts the Pending process nearest the top of the list first. Processes can allso be arranged interactively by drag and drop.
The Sort by status checkbox (top right of the grid) sorts and groups processes by their current status.

## Max time for new processes
The Max time for new processes field (bottom left) sets the default MaxTime applied to newly added processes. Enter a value and click Set to apply it.

## Resource constraints
BatchManager enforces two independent constraints. When either is reached, Running processes are Paused (and Pending ones held back) until usage drops again and resources are available.

## Process Constraints
The Available slider sets the maximum number of processes BatchManager is allowed to run simultaneously. The text to the right shows this as "N processes running <= M available" — N is how many are running now, M is the current limit from the slider.
The checkbox next to "Process Constraints" presumably enables/disablesthis limit entirely.

## Memory [GB] Constraints
The Available slider sets the maximum total memory, in GB, that BatchManager's processes are allowed to use together. The text to the right shows this as "The processes use N GB, peak of M GB < P GB available" — N is the memory currently in use by all BatchManager processes combined, M is their highest combined memory usage recorded so far, and P is the the current limit from the slider.
As with Process Constraints, a checkbox enables/disables this limit.

## System hardware (right-hand panel)
Read-only summary of the host machine, for reference while setting the sliders above:
- Processes — Total: total number of cores the system reports as available.
- Memory [GB] — Total: total physical RAM installed.
- Memory [GB] — Used: memory currently in use system-wide (highlighted in yellow or red when running low).

## Process output (top right)
Shows the live text output (stdout/stderr) of the process currentlyselected in the left grid, in the order it was produced.

## Memory usage graph (bottom right)
Plots resource usage over time for the selected process:
- Total Allocated [MB] (brown) — memory the process has allocated.
- Physical RAM [MB] (green) — actual physical RAM the process is using.
- Paging [Paging/s] (magenta, right-hand axis) — page faults per second, an indicator of the process swapping memory to disk, which signals memory pressure.
Options below the graph:
- Auto Zoom — automatically rescales the axes as new data comes in.
- Show RAM / Show Paging — toggle the corresponding curve on or off.
The X axis is elapsed time in seconds since the process started; the two Y axes are memory (MB, left) and paging rate (right).

## The "..." menu
The "..." button above the process list (top middle) opens a menu with these options:

### Managed file types
Lets you register file patterns so that dropping a matching data file (rather than an executable) onto BatchManager runs a chosen command on it automatically.

| Column | Description |
|---|---|
| Pattern | File-name pattern to match against dropped files, e.g. *.dat or *analysis.dat. More specific patterns can be listed to override a more general one for particular file names. |
| Command | The executable to run when a matching file is dropped. |
| Arguments | The command-line arguments to pass, using placeholders that are substituted with the dropped file's details (see below). |


#### Placeholders available in Arguments:
- #PATH# — the full path of the dropped file.
- #FOLDER# — the folder containing the dropped file.
- #FILE# — the file name without its extension.
Rows are matched in order/specificity, so a pattern like *analysis.dat can be given its own command and arguments distinct from the more general *.dat fallback below it.

### Set PATH
Opens the Add folders to PATH dialog, where you can add extra folders to search for binaries and DLLs. This is useful when a batch file invokes other executables, or a program depends on DLLs, that aren't in the system's normal PATH — add the folder here and BatchManager will include it when launching processes, alongside the original system PATH.

### Help
Shows this help.

## Typical workflow
- Set your Process Constraints and Memory Constraints sliders to match what your machine can safely handle.
- Drag and drop the executables/jobs you want to run onto the process list — they appear as Pending.
- BatchManager starts as many as it can within the limits; the rest wait as Pending.
- If overall memory or process count would exceed the limits, running processes are automatically Paused until resources free up, then resumed.
- Use the priority arrows to reorder which Pending or Paused process should run next.
- Select any process to inspect its live output and memory graph on the right.
- Once processes finish (Ended), use Delete Ended to clear them from the list.

## License
GPL-3.0-or-later
Copyright 2025 - 2026, the BatchManager author
