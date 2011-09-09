@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by MAME32.HHP. >"src\ui\help\Mame32.hm"
echo. >>"src\ui\help\Mame32.hm"
echo // Commands (ID_* and IDM_*) >>"src\ui\help\Mame32.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 src\ui\resource.h >>"src\ui\help\Mame32.hm"
echo. >>"src\ui\help\Mame32.hm"
echo // Prompts (IDP_*) >>"src\ui\help\Mame32.hm"
makehm IDP_,HIDP_,0x30000 src\ui\resource.h >>"src\ui\help\Mame32.hm"
echo. >>"src\ui\help\Mame32.hm"
echo // Resources (IDR_*) >>"src\ui\help\Mame32.hm"
makehm IDR_,HIDR_,0x20000 src\ui\resource.h >>"src\ui\help\Mame32.hm"
echo. >>"src\ui\help\Mame32.hm"
echo // Dialogs (IDD_*) >>"src\ui\help\Mame32.hm"
makehm IDD_,HIDD_,0x20000 src\ui\resource.h >>"src\ui\help\Mame32.hm"
echo. >>"src\ui\help\Mame32.hm"
echo // Frame Controls (IDW_*) >>"src\ui\help\Mame32.hm"
makehm IDW_,HIDW_,0x50000 src\ui\resource.h >>"src\ui\help\Mame32.hm"
REM -- Make help for Project MAME32


echo Building MAME32 Help files
start /wait hhc "src\ui\help\mame32.hhp"
REM if errorlevel 1 goto :Error
if not exist "src\ui\help\mame32.chm" goto :Error
echo.
:if exist obj\nul copy "src\ui\help\mame32.chm" Debug
:if exist obj\nul copy "src\ui\help\mame32.chm" Release
copy "src\ui\help\mame32.chm" .
echo.
goto :done

:Error
echo src\ui\help\mame32.hhp(1) : error: Problem encountered creating help file

:done
echo.
