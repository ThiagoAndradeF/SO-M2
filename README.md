# SO-M2
Command for observe page faults in linux: 
REDUCE
while ps -p <PID>  > /dev/null; do
    ps -o pid,min_flt,maj_flt -p <PID>
    sleep 1
done

DETAILED
while ps -p <PID>  > /dev/null; do
    ps -o pid,min_flt,maj_flt,rss,vsz -p <PID>
    sleep 1
done
