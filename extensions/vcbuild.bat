@echo off

cd %~dp0

set CCF_PLATFORM=x86
if "%PROCESSOR_ARCHITECTURE%"=="AMD64"  set CCF_PLATFORM=x64
if "%1"=="x86"                          set CCF_PLATFORM=x86
if "%1"=="x64"                          set CCF_PLATFORM=x64

set CCF_MS_PLATFORM=WIN32
if "%CCF_PLATFORM%"=="x64" set CCF_MS_PLATFORM=WIN64
set CCF_LIB_PATH=
if "%CCF_PLATFORM%"=="x64" set CCF_LIB_PATH=x64\

@rem Look for Visual Studio 2013
if not defined VS120COMNTOOLS goto vc-set-2012
if not exist "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2012
call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" %CCF_PLATFORM%
goto upgrade

:vc-set-2012
@rem Look for Visual Studio 2012
if not defined VS110COMNTOOLS goto vc-set-2010
if not exist "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2010
call "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" %CCF_PLATFORM%
goto upgrade

:vc-set-2010
@rem Look for Visual Studio 2010
if not defined VS100COMNTOOLS goto vc-set-2008
if not exist "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2008
call "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" %CCF_PLATFORM%
goto upgrade

:vc-set-2008
@rem Look for Visual Studio 2008
if not defined VS90COMNTOOLS goto vc-set-notfound
if not exist "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-notfound
call "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat" %CCF_PLATFORM%
goto msbuild

:vc-set-notfound
echo Error: Visual Studio not found
goto exit

:upgrade
echo Upgrading for Visual Studio ...
devenv /upgrade vc\cocoflow-extensions.sln

:msbuild
echo Compiling libccf-http ...
msbuild vc\cocoflow-extensions.sln /t:libccf-http /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow-extensions.sln /t:libccf-http /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
xcopy /Y vc\%CCF_LIB_PATH%Debug\libccf-http.lib ..\lib\Debug\ > nul
xcopy /Y vc\%CCF_LIB_PATH%Release\libccf-http.lib ..\lib\Release\ > nul

echo Compiling test ...
msbuild vc\cocoflow-extensions.sln /t:test_http_get,test_http_post /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow-extensions.sln /t:test_http_get,test_http_post /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo

:exit
pause
