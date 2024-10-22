#!/bin/bash

# カバレッジ情報を有効にしてコンパイル
gcc -coverage -c *.c
gcc -coverage -o tc *.o

# testディレクトリ内のすべての.mplファイルに対して./tcを実行
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

# カバレッジ情報を出力
gcov -b *.gcda
