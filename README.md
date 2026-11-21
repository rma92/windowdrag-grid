# WindowDrag Grid

Grid feedback build: `build-grid/windowdrag-grid64.exe` (or the matching x86 executable). During a native left-button window move, press right to show the monitor work-area grid and highlighted selection. Release right to fix the anchor, move across grid lines to extend the selection, and release left to commit. Escape cancels and hides the grid. The region-shaped GDI overlay uses the configured grid/selection colors and thickness without covering cell interiors. Cell membership uses exact integer boundaries; crossing a cell midpoint does not change selection.

Current crash fix builds are in `build-ready`. Both architectures passed controller startup/shutdown checks and the `--self-test` activation/timer regression. The reproduced timer crash was caused by a TCC code-generation issue when a function call appeared in the same argument list as a rectangle passed by value. Button state is now evaluated separately. The x86 controller also resolves TCC's decorated hook exports.

Work in progress: native C89 implementation of the approved grid and PowerMenu plan.

Run `build-tcc.bat` to build and run geometry/gesture tests with both installed TCC architectures and compile the matching hook DLL prototypes. Override `TCC32` and `TCC64` to use other compiler paths.

Implemented source includes a handle-independent grid and gesture core, portable INI settings functions, target identity checks, and an in-process system-menu subclass hook prototype. The hook allocates aligned command IDs below 0xF000 and dispatches commands to a controller protocol.

The build now includes controller prototypes, `build/windowdrag-grid64.exe` and `build/windowdrag-grid32.exe`. Run one controller at a time. A minimized control window provides Pause/Resume Grid, Save settings, and Exit through its right-click menu. The controller connects native move takeover and gesture polling, topmost toggling, and process priority commands. Settings load from `windowdrag-grid.ini` beside the executable. These paths have compiled but have not been interactively validated.

Missing work includes GDI feedback, window size constraints, DPI/frame adjustment, capture-loss handling, transparency, capability detection, tray registration and crash recovery, matching helper launch, settings controls, packaging, PE/import auditing, modern diagnostics builds, and integration tests. Outline mode currently delays placement until commit without drawing a preview. Hook compilation does not establish runtime correctness. Menu repair and safe subclass teardown still require validation and review. Do not run this prototype against important application sessions.

Latest increment: `build-next/windowdrag-grid64.exe` and its x86 counterpart include dynamically resolved transparency commands, with conditional restoration on normal exit. Existing layered windows are accepted only when their readable mode is alpha-only. Application changes to the last applied transparency prevent automatic restoration. These paths require interactive validation. The older missing-work list above describes the initial prototype; transparency is now implemented, but tray and recovery remain outstanding.

Set `WG_OUT` before running `build-tcc.bat` to choose a fresh output directory when hook DLLs are still loaded. All generated executables and matching DLLs must remain together. Avoid running different builds concurrently.

No Windows version compatibility is verified. NT 3.51/NewShell and Windows 10/11 acceptance require recorded interactive tests against the approved plan. Do not distribute these prototypes as a finished utility.
