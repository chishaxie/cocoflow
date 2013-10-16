@echo off

cd %~dp0

if exist lib\Release\libuv.lib goto check_msvc

echo Compiling libuv ...
cd deps
unzip gyp.zip > nul
unzip libuv-0.10.17.zip > nul
cd libuv-0.10.17\
md build
md build\gyp
xcopy /E ..\gyp build\gyp > nul
call vcbuild debug
call vcbuild release

cd %~dp0
md src\uv
xcopy /E deps\libuv-0.10.17\include src\uv > nul
md lib\Debug
md lib\Release
xcopy deps\libuv-0.10.17\Debug\lib\libuv.lib lib\Debug\ > nul
xcopy deps\libuv-0.10.17\Release\lib\libuv.lib lib\Release\ > nul

:check_msvc
@rem Look for Visual Studio 2012
if not defined VS110COMNTOOLS goto vc-set-2010
if not exist "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2010
call "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat"
goto msbuild

:vc-set-2010
@rem Look for Visual Studio 2010
if not defined VS100COMNTOOLS goto vc-set-2008
if not exist "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2008
call "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat"
goto msbuild

:vc-set-2008
@rem Look for Visual Studio 2008
if not defined VS90COMNTOOLS goto vc-set-notfound
if not exist "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-notfound
call "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat"
goto msbuild

:vc-set-notfound
echo Error: Visual Studio not found
goto exit

:msbuild
echo Compiling libccf ...
msbuild vc\cocoflow.sln /t:libccf /p:Configuration=Debug /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:libccf /p:Configuration=Release /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
xcopy /Y vc\Debug\libccf.lib lib\Debug\ > nul
xcopy /Y vc\Release\libccf.lib lib\Release\ > nul

echo Compiling demo ...
msbuild vc\cocoflow.sln /t:demo_all_sort,demo_any_sort,demo_http_server /p:Configuration=Debug /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:demo_all_sort,demo_any_sort,demo_http_server /p:Configuration=Release /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo

echo Compiling test ...
msbuild vc\cocoflow.sln /t:test_primitive,test_sleep,test_tcp,test_tcp2,test_tcp3,test_udp,test_udp2,test_udp3 /p:Configuration=Debug /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:test_primitive,test_sleep,test_tcp,test_tcp2,test_tcp3,test_udp,test_udp2,test_udp3 /p:Configuration=Release /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:benchmark_sleep,benchmark_tcp,benchmark_udp,benchmark_udp2 /p:Configuration=Debug /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
msbuild vc\cocoflow.sln /t:benchmark_sleep,benchmark_tcp,benchmark_udp,benchmark_udp2 /p:Configuration=Release /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo

:exit
pause
