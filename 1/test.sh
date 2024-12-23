#!/bin/bash
rm *.gcda *.gcno *.o *.gcov tc

gcc -coverage -c *.c
gcc -coverage -o tc *.o

for file in test/*.mpl; do
  if [ -f "$file" ]; then
    echo "Processing $file..."
    ./tc "$file"
  else
    echo "No .mpl files found in test directory."
  fi
done

./tc
./tc test/hoge.mpl

gcov -b *.gcda
