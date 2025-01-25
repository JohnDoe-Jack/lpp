#!/bin/bash
rm *.gcda *.gcno *.o *.gcov mpplc

gcc -coverage -c *.c *h
gcc -coverage -o mpplc *.o

for file in ../test/*.mpl; do
  if [ -f "$file" ]; then
    echo "Processing $file..."
    ./mpplc "$file" >/dev/null
  else
    echo "No .mpl files found in test directory."
  fi
done

./mpplc
./mpplc test/hoge.mpl

gcov -b *.gcda

lcov -d . -c -o coverage_test.info
genhtml coverage_test.info -o ./info

rm *.csl *.gcda *.gcno *.o *.gcov mpplc *.gch *.info
