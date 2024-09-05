@echo off
echo Killing child.exe process...

:: 使用 tasklist 查找 child.exe 进程的 PID
for /f "tokens=2 delims=," %%A in ('tasklist /FI "IMAGENAME eq child.exe" /FO CSV /NH') do (
    echo Terminating process with PID %%A
    taskkill /F /PID %%A
)

echo Process terminated.

