set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES "shared")
set(COMPONENT_SRCS 
cpu_cores/z80/z80.c
state.c
system.c
render.c
vdp.c
tms.c
pio.c
sound_output/dummy/sound_output_dummy.c
sms.c
memz80.c
sound/fmintf.c
sound/maxim_sn76489/sn76489.c
sound/ym2413.c
sound/sound.c
loadrom.c
)
register_component()
rg_setup_compile_options(-O3 -Wno-error=format)
