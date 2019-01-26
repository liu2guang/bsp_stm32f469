@echo off
del /Q .sconsign.dblite >nul 2>nul
del /Q *.uvguix.* >nul 2>nul
del /Q rtconfig.pyc >nul 2>nul
del /Q rtthread.map >nul 2>nul
del /Q rtthread.elf >nul 2>nul
rmdir /S /Q DebugConfig >nul 2>nul

