set(COMPONENT_ADD_INCLUDEDIRS "." "src")
set(COMPONENT_REQUIRES "shared")
set(COMPONENT_SRCS 
ppu.c
cpu.c
gb_impl.c
mapper/dummy.c
core.c
mmu.c
)
register_component()
rg_setup_compile_options(-O3 -Wno-error=format)
