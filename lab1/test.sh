#!/bin/bash

ERR=0
TOT=0
echo ""
echo "Running tests...."
echo ""

cat > in.txt <<EOF
This is the first line
this is the second
and this is the third
EOF

echo "Script Test 1..."
cat in.txt | tr "i" "\n" | wc -l > sol.txt 2> err.txt
./simpsh \
    --rdonly in.txt \
    --pipe \
    --pipe \
    --creat --wronly out.txt \
    --creat --wronly err.txt \
    --profile \
    --command 0 2 6 cat \
    --command 1 4 6 tr 'i' '\n' \
    --command 3 5 6 wc -l \
    --close 2 \
    --close 4 \
    --wait \
    --time 
if [[ $? == 0 ]] && diff -u sol.txt out.txt; then
    printf "\tPASSED\n\n"
else
    ERR=`expr $ERR + 1`
    printf "\tFAILED\n\n"
fi
TOT=`expr $TOT + 1`
rm sol.txt

echo "Script Test 2..."
cat in.txt | sort | tr ‘A-Z’ ‘a-z’ >sol.txt 2> err.txt
./simpsh \
    --rdonly in.txt \
    --pipe \
    --pipe \
    --creat --wronly out.txt \
    --creat --wronly err.txt \
    --profile \
    --command 1 4 6 sort \
    --command 0 2 6 cat  \
    --command 3 5 6 tr A-Z a-z \
    --close 2 \
    --close 4 \
    --wait \
    --time 
if [[ $? == 0 ]] && diff -u sol.txt out.txt; then
    printf "\tPASSED\n\n"
else
    ERR=`expr $ERR + 1`
    printf "\tFAILED\n\n"
fi
TOT=`expr $TOT + 1`
rm sol.txt out.txt

echo "Script Test 3..."
cat in.txt | { cat & cat; } | wc -l >> sol.txt 2> err.txt
./simpsh --rdonly in.txt \
    --pipe \
    --creat --append --wronly out.txt \
    --creat --wronly err.txt \
    --profile \
    --command 1 3 4 wc -l \
    --command 0 2 4 cat \
    --command 0 2 4 cat \
    --close 2 \
    --close 4 \
    --wait \
    --profile \
    --time 
if [[ $? == 0 ]] && diff -u sol.txt out.txt; then
    printf "\tPASSED\n\n"
else
    ERR=`expr $ERR + 1`
    printf "\tFAILED\n\n"
fi
TOT=`expr $TOT + 1`
rm in.txt out.txt err.txt sol.txt

######################################### END OF TESTS ########################################

echo ""
if [ $ERR == 0 ]; then
    printf "***ALL TESTS PASSED!***\n"
else
    printf "***`expr $TOT - $ERR` out of $TOT tests passed.***\n"
fi
echo ""
printf "Exiting test script...\n\n"
echo $ERR &> /dev/null

