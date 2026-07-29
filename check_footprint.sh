#!/bin/bash

# Simple script to check footprints of all newly built emulator apps
echo "Footprint Report"
echo "-------------------------------------"
for core in libxnes smsplus-gx temper walnut_cgb; do
    ELF_FILE="${core}/build/${core}.elf"
    if [ -f "$ELF_FILE" ]; then
        echo "Core: $core"
        # size tool standard output or idf.py size (we can use standard size or xtensa-esp32-elf-size)
        # Often idf.py provides size, but plain `size` on the ELF file gives .text and .bss
        if command -v xtensa-esp32-elf-size >/dev/null 2>&1; then
            xtensa-esp32-elf-size -B "$ELF_FILE"
        else
            size -B "$ELF_FILE"
        fi
        echo ""
    else
        echo "Core: $core - ELF file not found (did it build successfully?)"
    fi
done
