#!/bin/bash

# Simple script to check footprints of all newly built emulator apps
echo "Footprint Report"
echo "-------------------------------------"
for core in components/emu/components/*; do
    core_name=$(basename $core)
    ELF_FILE="components/emu/build/emu.elf"
    if [ -f "$ELF_FILE" ]; then
        echo "Core: $core_name"
        if command -v xtensa-esp32-elf-size >/dev/null 2>&1; then
            xtensa-esp32-elf-size -B "$ELF_FILE"
        else
            size -B "$ELF_FILE"
        fi
        echo ""
    else
        echo "Core: $core_name - ELF file not found (did it build successfully?)"
    fi
done
