#/bin/sh
#Hi my naem is Kirt and I liek anime

ulimit -Sc unlimited

while [ 2 ] ; do
if [ -f .stopserver2 ] ; then
echo servidor desligado >> servlog.txt
else
echo reiniciando servidor em `date +"%m-%d-%H:%M-%S"`>> startlog.txt
./login-server_sql
fi

sleep 5

done
