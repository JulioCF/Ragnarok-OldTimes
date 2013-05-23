@echo off
rem Writen by Jbain
:end
char-server.exe
echo .
echo .
echo Char server crashed! Reiniciando em 15 segundos! Precione ctrl+C para cancelar!
PING -n 15 127.0.0.1 >nul
goto end