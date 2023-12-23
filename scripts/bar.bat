@echo off
REM BAR = Build & Run, %1 -> Configuration (Release, Debug, ...)
call scripts/build %1
call scripts/run %1
