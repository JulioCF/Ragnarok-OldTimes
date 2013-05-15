#/bin/sh
#Hi my naem is Kirt and I liek anime

ulimit -Sc unlimited

while [ 3 ] ; do
if [ -f .stopserver3 ] ; then
echo servidor desligado >> servlog.txt
else
echo reiniciando servidor em `date +"%m-%d-%H:%M-%S"`>> startlog.txt
./char-server_sql
fi

sleep 5

done
