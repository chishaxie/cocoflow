@echo off

cd %~dp0

set CCF_UV_VERSION=0.10.27

set CCF_PLATFORM=x86
if "%PROCESSOR_ARCHITECTURE%"=="AMD64"  set CCF_PLATFORM=x64
if "%1"=="x86"                          set CCF_PLATFORM=x86
if "%1"=="x64"                          set CCF_PLATFORM=x64

set CCF_MS_PLATFORM=WIN32
if "%CCF_PLATFORM%"=="x64" set CCF_MS_PLATFORM=WIN64
set CCF_LIB_PATH=
if "%CCF_PLATFORM%"=="x64" set CCF_LIB_PATH=x64\

if exist lib\Release\libuv.lib goto check_msvc

echo Compiling libuv ...
cd deps
unzip gyp.zip > nul
unzip libuv-%CCF_UV_VERSION%.zip > nul
cd libuv-%CCF_UV_VERSION%\
md build
md build\gyp
xcopy /E ..\gyp build\gyp > nul
call vcbuild debug %CCF_PLATFORM%
call vcbuild release %CCF_PLATFORM%

cd %~dp0
md src\uv
xcopy /E deps\libuv-%CCF_UV_VERSION%\include src\uv > nul
md lib\Debug
md lib\Release
xcopy deps\libuv-%CCF_UV_VERSION%\Debug\lib\libuv.lib lib\Debug\ > nul
xcopy deps\libuv-%CCF_UV_VERSION%\Release\lib\libuv.lib lib\Release\ > nul

:check_msvc
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
devenv /upgrade vc\cocoflow.sln

:msbuild
echo Compiling libccf ...
msbuild vc\cocoflow.sln /t:libccf /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:libccf /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
xcopy /Y vc\%CCF_LIB_PATH%Debug\libccf.lib lib\Debug\ > nul
xcopy /Y vc\%CCF_LIB_PATH%Release\libccf.lib lib\Release\ > nul

echo Compiling demo ...
msbuild vc\cocoflow.sln /t:demo_all_sort,demo_any_sort,demo_http_server /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:demo_all_sort,demo_any_sort,demo_http_server /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo

echo Compiling test ...
msbuild vc\cocoflow.sln /t:test_primitive,test_sleep,test_tcp,test_tcp2,test_tcp3,test_udp,test_udp2,test_udp3,unexpected_tcp_timing /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:test_primitive,test_sleep,test_tcp,test_tcp2,test_tcp3,test_udp,test_udp2,test_udp3,unexpected_tcp_timing /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:benchmark_sleep,benchmark_tcp,benchmark_udp,benchmark_udp2 /p:Configuration=Debug /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:benchmark_sleep,benchmark_tcp,benchmark_udp,benchmark_udp2 /p:Configuration=Release /p:Platform="%CCF_MS_PLATFORM%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo

:exit
pause
