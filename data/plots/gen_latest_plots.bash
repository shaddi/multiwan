#!/bin/bash

# generate plots from the latest results for each conf

# for each conf, get the results from each exp type. if none, skip the conf. if we have all three, generate a new .plot file with the appropriate info. then run gnuplot.

exptypes=("mwan" "flowlb" "fatpipe")

for c in `cat confs`; do
    output_file="cdf-${c}-all.plot"
    echo -e "set key right bottom\nset title \"conn_dur CDF for ${c}\"\nset xlabel \"Connection Duration\"\nset yrange [0:1]\nset log x\nset term png\nset output \"conn_durs-${c}-all.cdf.png\"" > ${output_file}
    echo -n "plot " >> ${output_file}

    for exptype in "${exptypes[@]}"; do
        stats_path=`ls -l ../${exptype} | grep ${c} | awk '{print $8}' | tail -n 1`
        california_stats="../${exptype}/${stats_path}/results/california/conn_durs.california.stats"
        has_it=`ls ${california_stats} | wc -l`
        if [ ${has_it} -gt 0 ]; then
            echo -n "\"${california_stats}\" using 1:2 title \"${exptype}\" with linespoints" >> ${output_file}
            if [ ${exptype} != "fatpipe" ]; then
                echo -n ", " >> ${output_file}
            fi
        else
            echo -n "`date`: " >> plots.missing
            echo "No ${c} results for ${exptype}" >> plots.missing
        fi
    done
    echo "" >> ${output_file}
    echo "Plotting ${c} (${output_file})"
    gnuplot ${output_file}
done
