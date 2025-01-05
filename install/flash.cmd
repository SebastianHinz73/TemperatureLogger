
@echo off
SET COMPORT=COM3
SET ESP32TYPE=esp32
SET BINFOLDER=generic_esp32

:loop
cls
echo.
echo Please select one of the following options:
echo. 
echo  1 - Change Com port - %COMPORT%
echo  2 - Change Board Type - %ESP32TYPE%
echo  3 - Get board info
echo  4 - Flash Temperature Logger firmware
echo  5 - Delete flash (program, configuration and pin mapping)
echo  6 - Start serial console
echo. 
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :eof
if "%SELECTED%" == "1" goto :SetComPort
if "%SELECTED%" == "2" goto :SelectBoardType
if "%SELECTED%" == "3" goto :BoardInfo
if "%SELECTED%" == "4" goto :TemperatureLogger
if "%SELECTED%" == "5" goto :Delete
if "%SELECTED%" == "6" goto :Serial
goto :errorInput 


:SetComPort
echo  Enter 'x' for exit or COM port e.g. 'COM4'
echo.
set /p SELECTED=Your choice: 


if "%SELECTED%" == "x" goto :loop
set COMPORT=%SELECTED%

goto :loop

:SelectBoardType
echo  1 - Set Type to ESP32
echo  2 - Set Type to ESP32-S3
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :loop
if "%SELECTED%" == "1" (
set ESP32TYPE=esp32
set BINFOLDER=generic_esp32
goto :loop
)
if "%SELECTED%" == "2" (
set ESP32TYPE=esp32-s3
set BINFOLDER=generic_esp32s3_N16R8
goto :loop
)
goto :errorInput 


:BoardInfo
@echo on
esptool.exe --chip %ESP32TYPE% --port %COMPORT% chip_id
@echo off
echo.
pause
goto :loop

:TemperatureLogger
@echo on
esptool.exe -p %COMPORT% --chip %ESP32TYPE% write_flash 0x0 %BINFOLDER%\firmware.factory.bin

@echo off
echo.
pause
goto :loop

:Delete
@echo on
esptool.exe --port %COMPORT% --chip %ESP32TYPE% erase_flash
@echo off
echo.
pause
goto :loop

:Serial
@echo on
Putty\putty.exe -serial %COMPORT% -sercfg 115200,8,n,1,N
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

