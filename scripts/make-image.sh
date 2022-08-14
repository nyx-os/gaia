#!/usr/bin/bash

cd ..
rm -rf iso_root
mkdir -p iso_root
cp $1/kernel sysroot/* \
	thirdparty/limine/limine.sys thirdparty/limine/limine-cd.bin thirdparty/limine/limine-cd-efi.bin iso_root/
xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o nyx.iso
	thirdparty/limine/limine-deploy nyx.iso
	rm -rf iso_root