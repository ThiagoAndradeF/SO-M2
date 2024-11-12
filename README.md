# SO-M2
Command for observe page faults in linux: 
REDUCE
<code> 
    while ps -p <PID>  > /dev/null; do
        ps -o pid,min_flt,maj_flt -p <PID>
        sleep 1
    done
</code>


DETAILED
<code> 
    while ps -p <PID>  > /dev/null; do
        ps -o pid,min_flt,maj_flt,rss,vsz -p <PID>
        sleep 1
    done

</code>
