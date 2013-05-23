#/bin/sh

ulimit -Sc unlimited

while [ 3 ] ; do
if [ -f .stopserver3 ] ; then
echo server marked down >> servlog.txt
else
echo restarting server at time at `date +"%m-%d-%H:%M-%S"` >> log/startlog.txt
./char-server
fi

sleep 5

done
