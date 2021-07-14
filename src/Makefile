
PWD := $(shell pwd)
gra-dri-path = $(PWD)/graphene-sgx-driver
sgx-dri-path = $(PWD)/linux-sgx-driver

$(gra-dri-path)/gsgx.ko:
	cd $(gra-dri-path) && make

$(sgx-dri-path)/isgx.ko:
	cd $(sgx-dri-path) && make

installDriver: $(gra-dri-path)/gsgx.ko $(sgx-dri-path)/isgx.ko
	sudo insmod $(gra-dri-path)/gsgx.ko
	sudo insmod $(sgx-dri-path)/isgx.ko

installDocker:
	docker run --rm --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -it --privileged -d --device=/dev/gsgx --device=/dev/isgx -v ~/workspace/ATsgx:/Users/root/ATsgx --ipc=host -p 6006:22 --name=ATSGX panzhanghust/atsgx:simple /bin/zsh

.PHONY: install
install: installDriver installDocker

clean:
	make -C $(gra-dri-path) clean
	make -C $(sgx-dri-path) clean
