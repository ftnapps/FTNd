@echo off

rem   $Id: doors.bat,v 1.1 2004/08/11 10:55:56 mbse Exp $

rem   doors.bat started by mbsebbs when starting a door program.
rem   The first parameter must be tha name of the door, in the
rem   c:\doors directory must be a .bat file with the name of
rem   the door.
rem   If the door is named "dos" then no door is started but we
rem   stay in the dos shell. This can be used by the sysop to
rem   manually install and maintain doors. If you don't like that
rem   then comment out the next line.

if %1==dos goto end

c:
cd \doors
if exist %1.bat call %1 %2 %3 %4 %5 %6 %7 %8 %9
c:\dosemu\exitemu

:end
echo Welcome to the DOS (door) shell, type EXITEMU to leave.
