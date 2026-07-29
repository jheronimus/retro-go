set shell := ["bash", "-c"]

default:
    @just --list

build-sdl2:
    @echo "Building SDL2..."
    rm -f launcher.exe emu.exe gmon.out
    gcc -no-pie -DRG_TARGET_SDL2 -DRETRO_GO -DCJSON_HIDE_SYMBOLS -DSDL_MAIN_HANDLED=1 -DRG_BUILD_INFO=\"SDL2\" -Dapp_main=SDL_Main $(sdl2-config --cflags) -Icomponents/shared -Icomponents/shared/libs/cJSON -Icomponents/shared/libs/lodepng -Icomponents/shared/libs/miniz -Icomponents/launcher/main components/shared/*.c components/shared/drivers/audio/*.c components/shared/fonts/*.c components/shared/libs/cJSON/*.c components/shared/libs/lodepng/*.c components/shared/libs/miniz/*.c components/launcher/main/*.c $(sdl2-config --libs) -lstdc++ -o launcher.exe
    gcc -no-pie -DRG_TARGET_SDL2 -DRETRO_GO -DCJSON_HIDE_SYMBOLS -DSDL_MAIN_HANDLED=1 -DRG_BUILD_INFO=\"SDL2\" -Dapp_main=SDL_Main $(sdl2-config --cflags) -Icomponents/shared -Icomponents/shared/libs/cJSON -Icomponents/shared/libs/lodepng -Icomponents/shared/libs/miniz -Icomponents/emu/components/gb -Icomponents/emu/components/nes -Icomponents/emu/components/pce -Icomponents/emu/components/sms -Icomponents/emu/main components/shared/*.c components/shared/drivers/audio/*.c components/shared/fonts/*.c components/shared/libs/cJSON/*.c components/shared/libs/lodepng/*.c components/shared/libs/miniz/*.c components/emu/components/gb/*.c components/emu/components/nes/*.c components/emu/components/pce/*.c components/emu/components/sms/*.c components/emu/main/*.c $(sdl2-config --libs) -lstdc++ -o emu.exe
    @echo "SDL2 build complete! Run ./launcher.exe and ./emu.exe"

build-fw:
    @echo "Building ESP32 Firmware for MRGC-G32..."
    export SDKCONFIG_DEFAULTS={{invocation_directory()}}/components/shared/targets/mrgc-g32/sdkconfig; \
    export IDF_TARGET=esp32; \
    cd components/launcher && idf.py bootloader app -DRG_PROJECT_APP=launcher -DRG_PROJECT_VER=experimental -DRG_BUILD_TARGET=RG_TARGET_MRGC_G32 -DRG_BUILD_RELEASE=0 -DRG_ENABLE_PROFILING=0 -DRG_ENABLE_NETWORKING=1; \
    cd ../emu && idf.py app -DRG_PROJECT_APP=emu -DRG_PROJECT_VER=experimental -DRG_BUILD_TARGET=RG_TARGET_MRGC_G32 -DRG_BUILD_RELEASE=0 -DRG_ENABLE_PROFILING=0 -DRG_ENABLE_NETWORKING=1
    python tools/mkfw.py --type esplay --name Retro-Go --icon assets/icon.raw --version experimental retro-go_mrgc-g32.fw --target mrgc-g32 --bootloader components/launcher/build/bootloader/bootloader.bin 0 16 1048576 launcher components/launcher/build/launcher.bin 0 17 1048576 emu components/emu/build/emu.bin
    @echo "Firmware retro-go_mrgc-g32.fw built!"

check-footprint:
    @echo "Footprint Report"
    @echo "-------------------------------------"
    @for core in components/emu/components/*; do \
        core_name=$$(basename $$core); \
        ELF_FILE="components/emu/build/emu.elf"; \
        if [ -f "$$ELF_FILE" ]; then \
            echo "Core: $$core_name"; \
            if command -v xtensa-esp32-elf-size >/dev/null 2>&1; then \
                xtensa-esp32-elf-size -B "$$ELF_FILE"; \
            else \
                size -B "$$ELF_FILE"; \
            fi; \
            echo ""; \
        else \
            echo "Core: $$core_name - ELF file not found (did it build successfully?)"; \
        fi \
    done

clean:
    @echo "Cleaning build artifacts..."
    rm -f launcher.exe emu.exe gmon.out retro-go_mrgc-g32.fw
    rm -rf components/launcher/build components/emu/build
    rm -f components/launcher/sdkconfig components/emu/sdkconfig
