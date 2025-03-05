FROM ubuntu:20.04

# In case some package that we will install requires the user to be prompted to configure something, ignore that prompt, just install the package.
ENV DEBIAN_FRONTEND=noninteractive 


# Update the package index, so that we know what is out there that we can install.
RUN apt update


# Install packages that we need to compile and run the kernel.
# The packages that you see here, were taken with "apt list", from Virtual Machine that was used to develop this project.
# If you ask why don't we have packages.txt that we just pass to apt install, or why don't we have apt install with multiple arguments.
# It is because, first of all, this works, second of all, caching.
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


# Create a new directory project on the linux machine (image more precisely) in the root directory. 
# And copy everything from project in the git repo to the linux machine (image more precisely).
RUN mkdir project
COPY ./project ./project

# Change the working directory, such that every subsequent command we execute is relative to that directory.
# The directory is located on the linux machine (again, on image more precisely).
WORKDIR project

# First build the kernel, and then run it.
RUN make
ENTRYPOINT [ "make", "qemu" ]
