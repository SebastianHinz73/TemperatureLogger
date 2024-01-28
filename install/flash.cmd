
@echo off
SET COMPORT=COM3


:loop
cls
echo.
echo Please select one of the following options:
echo. 
echo  1 - Change com port
echo  2 - Get board info on %COMPORT%
echo  3 - Flash Temperature Logger firmware %COMPORT%
echo  4 - Delete flash %COMPORT%
echo. 
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :eof
if "%SELECTED%" == "1" goto :SetComPort
if "%SELECTED%" == "2" goto :BoardInfo
if "%SELECTED%" == "3" goto :TemperatureLogger
if "%SELECTED%" == "4" goto :Delete
goto :errorInput 


:SetComPort
echo  1 - Set com port1
echo  2 - Set com port2
echo  3 - Set com port3
echo  4 - Set com port4
echo  5 - Set com port5
echo  6 - Set com port6
echo  7 - Set com port7
echo  8 - Set com port8
echo  9 - Set com port9
echo  10 - Set com port10
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :loop
if "%SELECTED%" == "1" (
set COMPORT=COM1
goto :loop
)
if "%SELECTED%" == "2" (
set COMPORT=COM2
goto :loop
)
if "%SELECTED%" == "3" (
set COMPORT=COM3
goto :loop
)
if "%SELECTED%" == "4" (
set COMPORT=COM4
goto :loop
)
if "%SELECTED%" == "5" (
set COMPORT=COM5
goto :loop
)
if "%SELECTED%" == "6" (
set COMPORT=COM6
goto :loop
)
if "%SELECTED%" == "7" (
set COMPORT=COM7
goto :loop
)
if "%SELECTED%" == "8" (
set COMPORT=COM8
goto :loop
)
if "%SELECTED%" == "9" (
set COMPORT=COM9
goto :loop
)
if "%SELECTED%" == "10" (
set COMPORT=COM10
goto :loop 
)
goto :errorInput 

pause
goto :loop

:BoardInfo
@echo on
esptool.exe --chip esp32 --port %COMPORT% chip_id
@echo off
echo.
pause
goto :loop

:TemperatureLogger
@echo on
esptool.exe -p %COMPORT% --chip esp32 write_flash 0x0 firmware.factory.bin

@echo off
echo.
pause
goto :loop

:Delete
@echo on
esptool.exe --port %COMPORT% --chip esp32 erase_flash
@echo off
echo.
pause
goto :loop

:errorInput
echo.
echo Illegal input! Please try again!
echo.
pause
goto :loop

