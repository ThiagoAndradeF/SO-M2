# SO-M2
Command for observe page faults by PID to process in linux: 



REDUCED
<code> 
    while ps -p -PIDNUMBER-  > /dev/null; do
        ps -o pid,min_flt,maj_flt -p -PIDNUMBER- 
        sleep 1
    done
</code>


DETAILED
<code> 
    while ps -p -PIDNUMBER-   > /dev/null; do
        ps -o pid,min_flt,maj_flt,rss,vsz -p -PIDNUMBER- 
        sleep 1
    done

</code>
