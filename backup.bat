@rem Comment

@echo off

for %%f in (%cd%) do set myfolder=%%~nxf

set mydate=%date%
set resultname=%myfolder% - %mydate%.rar
set resultname=%resultname:/=.%

@echo %cd%\%resultname%
"%winrar%" a "%resultname%" "%cd%"

MOVE /-Y "%cd%\%resultname%" "%backups_dir%"

pause