#!/bin/bash



#!/bin/bash
#for filename in tests/test* ; do
    #dos2unix ${filename}
#done
for filename in tests/test*.trc; do
    test_num=`echo $filename | cut -d'.' -f1`
    #dos2unix ${filename}
    ./bp_main $filename > ${test_num}Yours.out
    echo "diff for $filename"
    diff -y ${test_num}.out ${test_num}Yours.out | grep "|"
done

for val in {1..5}
do
	echo "test $val"
	./bp_main new_TESTS_HW1/example$val.trc > new_TESTS_HW1/myout$val.out
	diff -y new_TESTS_HW1/myout$val.out new_TESTS_HW1/example$val.out | grep "|"
	echo " "
done

for val in {1..3}
do
	echo "test $val"
	./bp_main input_examples/example$val.trc > input_examples/myout$val.out
	diff -y input_examples/myout$val.out input_examples/example$val.out | grep "|"
	echo " "
done

#for val in {1..20}
#do
#	echo "test $val"
#	./bp_main ../CompArch-hw1-tests/tests/example$val.trc > ../CompArch-hw1-tests/myouts/myout$val.out
#	diff -y ../CompArch-hw1-tests/myouts/myout$val.out ../CompArch-hw1-tests/ref_results/example$val.out | grep "|"
#	echo " "
#
#done