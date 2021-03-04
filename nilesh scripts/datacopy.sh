n=12



while [ $n -lt 36 ]; do
   # echo "$args "
#   echo "$n"
   cat mix$n-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt | grep  "CPU 1 cumulative IPC:" | head -1 | cut -d " " -f5
#   cat mix$n-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt | grep -A 30 "CPU 1 cumulative IPC:" |head -27 | egrep -o "MPKI: [[:digit:]]+.[[:digit:]]+" | cut -d " " -f2 | sed -n  -e 6p -e 11p
#    echo ""
                n=$((n+1))
done

