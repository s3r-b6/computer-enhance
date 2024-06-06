#!/bin/bash
gcc ./decoder.c -o decoder.out
mkdir -p ./builds

for file in *.asm; do
    base_name=$(basename "$file" .asm)
    nasm "$file" -o "./builds/${base_name}.out"
    ./decoder.out "./builds/${base_name}.out" > tmp.asm
    nasm tmp.asm -o "./builds/tmp.out"
    diff "./builds/tmp.out" "./builds/${base_name}.out"
done

rm -f ./builds/*
rm -f ./tmp.asm
