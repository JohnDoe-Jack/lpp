#!/bin/bash
rm *.gcda *.gcno *.o *.gcov pp

gcc -coverage -c *.c *h
gcc -coverage -o pp *.o

for file in ../test/*.mpl; do
  if [ -f "$file" ]; then
    echo "Processing $file..."
    ./pp "$file" >/dev/null
  else
    echo "No .mpl files found in test directory."
  fi
done

./pp
./pp test/hoge.mpl

gcov -b *.gcda

lcov -d . -c -o coverage_test.info
genhtml coverage_test.info -o ./info
