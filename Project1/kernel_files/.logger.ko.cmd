cmd_/home/leo0519/os/kernel_files/logger.ko := ld -r -m elf_x86_64  -z max-page-size=0x200000 -T ./scripts/module-common.lds --build-id  -o /home/leo0519/os/kernel_files/logger.ko /home/leo0519/os/kernel_files/logger.o /home/leo0519/os/kernel_files/logger.mod.o