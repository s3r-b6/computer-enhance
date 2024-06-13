#!/bin/bash
gcc ./decoder.c -o decoder.out
mkdir -p ./builds

diff=false
for file in *.asm; do
    base_name=$(basename "$file" .asm)
    nasm "$file" -o "./builds/${base_name}.out"
    ./decoder.out "./builds/${base_name}.out" > tmp.asm
    nasm tmp.asm -o "./builds/tmp.out"
    d=$(diff "./builds/tmp.out" "./builds/${base_name}.out")
    if [ -n "$d" ]; then
        printf "DIFF FOUND in %s\n" "$file"
        diff=true
    fi
done

if ! $diff; then
    printf "NO DIFFERENCES FOUND\n"
fi

rm -f ./builds/*
rm -f ./tmp.asm
