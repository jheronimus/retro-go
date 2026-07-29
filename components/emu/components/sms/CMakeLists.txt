set(COMPONENT_ADD_INCLUDEDIRS "." "source")
set(COMPONENT_REQUIRES "shared")
set(COMPONENT_SRCS 
source/cpu_cores/z80/z80.c
source/state.c
source/system.c
source/render.c
source/vdp.c
source/tms.c
source/pio.c
source/sound_output/dummy/sound_output_dummy.c
source/sms.c
source/memz80.c
source/sound/fmintf.c
source/sound/maxim_sn76489/sn76489.c
source/sound/ym2413.c
source/sound/sound.c
source/loadrom.c
)
register_component()
rg_setup_compile_options(-O3 -Wno-error=format)
