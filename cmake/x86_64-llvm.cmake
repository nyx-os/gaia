set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET x86_64-unknown-elf)

set(KERNEL_C_FLAGS -fno-stack-check -ggdb
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -mabi=sysv
        -mno-80387
        -mno-mmx
        -mno-3dnow
        -mno-sse
        -mno-sse2
        -mno-ssse3
        -mno-sse4
        -mno-sse4a
        -mno-sse4.1
        -mno-sse4.2
        -mno-avx
        -mno-avx2
        -mno-avx512f
        -mno-red-zone
        -msoft-float
        -mcmodel=kernel)

set(KERNEL_LINK_FLAGS -T${CMAKE_CURRENT_SOURCE_DIR}/${GAIA_DIR}/arch/${ARCH}/link.ld
        -nostdlib
        -melf_x86_64
        -zmax-page-size=0x1000
        -static)

set(CMAKE_C_FLAGS_RELEASE_INIT "-O2")
set(CMAKE_C_FLAGS_DEBUG_INIT "-Og -ggdb")

set(CMAKE_C_LINK_EXECUTABLE "ld.lld <OBJECTS> -o <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_ASM_NASM_COMPILER "nasm")
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS "asm;nasm;S;s")

set(CMAKE_LINKER ld.lld CACHE INTERNAL STRING)
set(CMAKE_OBJCOPY llvm-objcopy CACHE INTERNAL STRING)
set(CMAKE_AR llvm-ar CACHE INTERNAL STRING)
set(CMAKE_SIZE_UTIL llvm-size CACHE INTERNAL STRING)