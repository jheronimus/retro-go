set(COMPONENT_ADD_INCLUDEDIRS "." "src")
set(COMPONENT_REQUIRES "retro-go")
set(COMPONENT_SRCS 
src/xnes.c
src/ppu.c
src/controller.c
src/apu.c
src/cpu.c
src/cartridge.c
src/mapper/mapper3.c
src/mapper/mapper7.c
src/mapper/mapper140.c
src/mapper/mapper225.c
src/mapper/mapper113.c
src/mapper/mapper0.c
src/mapper/mapper66.c
src/mapper/mapper4.c
src/mapper/mapper1.c
src/mapper/mapper177.c
src/mapper/mapper79.c
src/mapper/mapper2.c
src/mapper/mapper87.c
src/mapper/mapper15.c
src/mapper/mapper241.c
src/dma.c
)
register_component()
rg_setup_compile_options(-O3 -Wno-error=format)
