:: Ini ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	@echo off
	cls

	:: �t�@�C�����̓\�[�X�Ɠ���
	set fn=%~n0
	set src=%fn%.c
	set exec=%fn%.exe
	set cc=gcc.exe
	set lib=lib_iwmutil.a
	set option=-Os -Wall -lgdi32 -luser32 -lshlwapi

:: Make ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

	echo --- Compile -S ------------------------------------
	for %%s in (%src%) do (
		%cc% %%s -S %option%
		echo %%~ns.s
	)
	echo.

	echo --- Make ------------------------------------------
	for %%s in (%src%) do (
		%cc% %%s -c %option%
	)
	%cc% *.o %lib% -o %exec% %option%
	echo %exec%
	echo.

	:: �㏈��
	strip %exec%
	rm *.o

	:: ���s
	if not exist "%exec%" goto end

	:: ����
	echo.
	pause

:: Test ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	cls
	set t=%time%
	echo [%t%]

	%exec%
	%exec% sleep 1

	echo [%t%]
	echo [%time%]

:: Quit ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:end
	echo.
	pause
	exit
