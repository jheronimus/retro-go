set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES "shared")
set(COMPONENT_SRCS 
nes.c
ppu.c
controller.c
apu.c
cpu.c
cartridge.c
mapper/mapper3.c
mapper/mapper7.c
mapper/mapper140.c
mapper/mapper225.c
mapper/mapper113.c
mapper/mapper0.c
mapper/mapper66.c
mapper/mapper4.c
mapper/mapper1.c
mapper/mapper177.c
mapper/mapper79.c
mapper/mapper2.c
mapper/mapper87.c
mapper/mapper15.c
mapper/mapper241.c
dma.c
)
register_component()
rg_setup_compile_options(-O3 -Wno-error=format)
