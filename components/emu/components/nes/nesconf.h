#ifndef __NES_CONF_H__
#define __NES_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#ifndef nes_malloc
#define nes_malloc		malloc
#endif

#ifndef nes_free
#define nes_free		free
#endif

#ifndef nes_memcpy
#define nes_memcpy		memcpy
#endif

#ifndef nes_memset
#define nes_memset		memset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NES_CONF_H__ */
