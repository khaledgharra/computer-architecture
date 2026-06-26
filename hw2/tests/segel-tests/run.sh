#!/bin/bash

# num=1
# var=$( cat ../examples/example${num}_command )
# echo $var
# ./cacheSim "../examples/example${num}_trace" ${var} > 1.txt
# gdb --args ./cacheSim "../examples/example${num}_trace" ${var}



for num in {1..3}
do
    echo ${num}
    var=$( cat ../examples/example${num}_command )
    echo $var
    ./cacheSim "../examples/example${num}_trace" ${var} > ${num}.txt
    diff ${num}.txt ../examples/example${num}_output
    echo 
done


for num in {1..20}
do
    echo ${num}
    var=$( cat ../test2/example${num}_command )
    ./cacheSim "../test2/example${num}_trace" ${var} > ${num}.txt
    diff ${num}.txt ../test2/example${num}_output
    echo 
done

    # ./cacheSim ../test2/example1_trace --mem-cyc 300 --bsize 5 --wr-alloc 0 --l1-size 8 --l1-assoc 0 --l1-cyc 1 --l2-size 15 --l2-assoc 6 --l2-cyc 30 > 1.txt
    # ./cacheSim ../test2/example2_trace --mem-cyc 300 --bsize 3 --wr-alloc 1 --l1-size 9 --l1-assoc 1 --l1-cyc 9 --l2-size 21 --l2-assoc 9 --l2-cyc 100 > 2.txt
    # ./cacheSim ../test2/example3_trace --mem-cyc 300 --bsize 7 --wr-alloc 1 --l1-size 8 --l1-assoc 1 --l1-cyc 7 --l2-size 12 --l2-assoc 2 --l2-cyc 20 > 3.txt


    # ./cacheSim ../test2/example4_trace --mem-cyc 100 --bsize 3 --wr-alloc 1 --l1-size 5 --l1-assoc 2 --l1-cyc 9 --l2-size 13 --l2-assoc 5 --l2-cyc 30 > 4.txt
    # ./cacheSim ../test2/example5_trace --mem-cyc 100 --bsize 8 --wr-alloc 0 --l1-size 9 --l1-assoc 0 --l1-cyc 10 --l2-size 12 --l2-assoc 0 --l2-cyc 70 > 5.txt

    # ./cacheSim ../test2/example6_trace --mem-cyc 100 --bsize 3 --wr-alloc 0 --l1-size 9 --l1-assoc 2 --l1-cyc 10 --l2-size 14 --l2-assoc 10 --l2-cyc 20 > 6.txt
    # ./cacheSim ../test2/example7_trace --mem-cyc 300 --bsize 4 --wr-alloc 0 --l1-size 9 --l1-assoc 5 --l1-cyc 5 --l2-size 18 --l2-assoc 6 --l2-cyc 40 > 7.txt
    # ./cacheSim ../test2/example8_trace --mem-cyc 300 --bsize 4 --wr-alloc 0 --l1-size 7 --l1-assoc 2 --l1-cyc 6 --l2-size 18 --l2-assoc 2 --l2-cyc 30 > 8.txt

    # ./cacheSim ../test2/example9_trace --mem-cyc 100 --bsize 7 --wr-alloc 1 --l1-size 8 --l1-assoc 0 --l1-cyc 3 --l2-size 13 --l2-assoc 1 --l2-cyc 80 > 9.txt
    # ./cacheSim ../test2/example10_trace --mem-cyc 100 --bsize 6 --wr-alloc 0 --l1-size 8 --l1-assoc 2 --l1-cyc 9 --l2-size 20 --l2-assoc 6 --l2-cyc 60 > 10.txt
    # ./cacheSim ../test2/example11_trace --mem-cyc 200 --bsize 3 --wr-alloc 1 --l1-size 8 --l1-assoc 5 --l1-cyc 8 --l2-size 14 --l2-assoc 10 --l2-cyc 60 > 11.txt


    # ./cacheSim ../test2/example12_trace --mem-cyc 300 --bsize 5 --wr-alloc 1 --l1-size 8 --l1-assoc 0 --l1-cyc 2 --l2-size 20 --l2-assoc 0 --l2-cyc 40 > 12.txt
    # ./cacheSim ../test2/example13_trace --mem-cyc 300 --bsize 3 --wr-alloc 0 --l1-size 8 --l1-assoc 3 --l1-cyc 5 --l2-size 15 --l2-assoc 7 --l2-cyc 30 > 13.txt
    # ./cacheSim ../test2/example14_trace --mem-cyc 300 --bsize 3 --wr-alloc 0 --l1-size 4 --l1-assoc 0 --l1-cyc 1 --l2-size 18 --l2-assoc 3 --l2-cyc 80 > 14.txt


    # ./cacheSim ../test2/example15_trace --mem-cyc 100 --bsize 6 --wr-alloc 0 --l1-size 10 --l1-assoc 1 --l1-cyc 10 --l2-size 19 --l2-assoc 6 --l2-cyc 50 > 15.txt
    # ./cacheSim ../test2/example16_trace --mem-cyc 200 --bsize 6 --wr-alloc 0 --l1-size 6 --l1-assoc 0 --l1-cyc 1 --l2-size 18 --l2-assoc 6 --l2-cyc 60 > 16.txt
    # ./cacheSim ../test2/example17_trace --mem-cyc 300 --bsize 6 --wr-alloc 0 --l1-size 7 --l1-assoc 0 --l1-cyc 2 --l2-size 14 --l2-assoc 4 --l2-cyc 20 > 17.txt


    # ./cacheSim ../test2/example18_trace --mem-cyc 200 --bsize 4 --wr-alloc 1 --l1-size 4 --l1-assoc 0 --l1-cyc 6 --l2-size 14 --l2-assoc 7 --l2-cyc 90 > 18.txt
    # ./cacheSim ../test2/example19_trace --mem-cyc 300 --bsize 5 --wr-alloc 0 --l1-size 7 --l1-assoc 0 --l1-cyc 4 --l2-size 14 --l2-assoc 9 --l2-cyc 90 > 19.txt
    # ./cacheSim ../test2/example20_trace --mem-cyc 300 --bsize 7 --wr-alloc 0 --l1-size 8 --l1-assoc 0 --l1-cyc 5 --l2-size 21 --l2-assoc 13 --l2-cyc 50 > 20.txt


# diff 1.txt ../test2/example1_output
# diff 2.txt ../test2/example2_output
# diff 3.txt ../test2/example3_output
# diff 4.txt ../test2/example4_output
# diff 5.txt ../test2/example5_output
# diff 6.txt ../test2/example6_output
# diff 7.txt ../test2/example7_output
# diff 8.txt ../test2/example8_output
# diff 9.txt ../test2/example9_output
# diff 10.txt ../test2/example10_output
# diff 11.txt ../test2/example11_output
# diff 12.txt ../test2/example12_output
# diff 13.txt ../test2/example13_output
# diff 14.txt ../test2/example14_output
# diff 15.txt ../test2/example15_output
# diff 16.txt ../test2/example16_output
# diff 17.txt ../test2/example17_output
# diff 18.txt ../test2/example18_output
# diff 19.txt ../test2/example19_output
# diff 20.txt ../test2/example20_output


