@echo off
rem Writen by Jbain
:end
login-server_sql.exe
echo .
echo .
echo Login server crashed! Reiniciando em 15 segundos! Precione ctrl+C para cancelar!
PING -n 15 127.0.0.1 >nul
goto end