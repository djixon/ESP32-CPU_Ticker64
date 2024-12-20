v3.0

- made non-windowed versions of routines for direct ipc grabs from another core
- now both 64bit tickers (on dual cores) can run in parallel and even be nested
- added deinitialization routines, which free allocated interrupts
- added another auto calibration variable for cases of ipc-s
- without RAM usage for storage of upper part, ISR and other assembler routines extra optimized
- added example of ISR deinitialization
- performance improved
