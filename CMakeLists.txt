cmake_minimum_required(VERSION 3.15)

# Configurable options
set(TOOLCHAIN llvm)
set(ARCH x86_64)
set(GAIA_DIR src/gaia)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${ARCH}-${TOOLCHAIN}.cmake)

project("nyx" VERSION 0.0.1 DESCRIPTION "Simple and fast microkernel")

enable_language(ASM_NASM)

add_executable(kernel ${GAIA_DIR}/main.c ${GAIA_DIR}/config.h src/gaia/arch/x86_64/entry.c)

set(CMAKE_TOOLCHAIN_FILE cmake/x86_64-llvm.cmake)

target_compile_options(kernel PUBLIC $<$<COMPILE_LANGUAGE:C,CXX>:${KERNEL_C_FLAGS}>)

target_link_options(kernel PUBLIC ${KERNEL_LINK_FLAGS})

target_include_directories(kernel PUBLIC src ${GAIA_DIR}/arch/${ARCH} thirdparty/limine)

set(ISO_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.iso)
set(LIMINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/limine)

add_custom_command(OUTPUT ${ISO_IMAGE} DEPENDS kernel
        COMMAND make -C ${LIMINE_DIR}
        COMMAND bash ../scripts/make-image.sh ${CMAKE_CURRENT_BINARY_DIR}
        )

add_custom_target(iso ALL DEPENDS ${ISO_IMAGE})
add_custom_target(run DEPENDS ${ISO_IMAGE} COMMAND qemu-system-x86_64 -enable-kvm -no-reboot -no-shutdown -M q35 -m 2G -boot d -cdrom ${ISO_IMAGE})
