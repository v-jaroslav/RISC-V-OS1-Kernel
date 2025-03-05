docker build -f .\main.dockerfile --tag risc-v-kernel .
docker run -it risc-v-kernel
ctrl+a+x