FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive 

RUN apt update

RUN apt install -y build-essential
RUN apt install -y binutils-riscv64-linux-gnu-dbg
RUN apt install -y binutils-riscv64-linux-gnu
RUN apt install -y binutils-riscv64-unknown-elf
RUN apt install -y cpp-10-riscv64-linux-gnu
RUN apt install -y cpp-riscv64-linux-gnu
RUN apt install -y g++-10-riscv64-linux-gnu
RUN apt install -y g++-riscv64-linux-gnu
RUN apt install -y gcc-10-plugin-dev-riscv64-linux-gnu
RUN apt install -y gcc-10-riscv64-linux-gnu-base
RUN apt install -y gcc-10-riscv64-linux-gnu
RUN apt install -y gcc-riscv64-linux-gnu
RUN apt install -y gcc-riscv64-unknown-elf
RUN apt install -y grub-firmware-qemu
RUN apt install -y qemu-block-extra
RUN apt install -y qemu-efi
RUN apt install -y qemu-guest-agent
RUN apt install -y qemu-kvm
RUN apt install -y qemu-slof
RUN apt install -y qemu-system-common
RUN apt install -y qemu-system-data
RUN apt install -y qemu-system-misc
RUN apt install -y qemu-system
RUN apt install -y qemu-user-binfmt
RUN apt install -y qemu-user-static
RUN apt install -y qemu-user
RUN apt install -y qemu-utils
RUN apt install -y qemu
RUN apt install -y u-boot-qemu

RUN mkdir project
COPY ./kernel ./project
WORKDIR project

RUN make
ENTRYPOINT [ "make", "qemu" ]
