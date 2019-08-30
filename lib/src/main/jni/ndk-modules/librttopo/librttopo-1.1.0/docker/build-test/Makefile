DOCKER=docker
REGISTRY=docker.kbt.io
NAME=$(REGISTRY)/librttopo/build-test

all: debian-sid

push: debian-sid-push

debian-sid:
	$(DOCKER) build -t $(NAME):debian-sid - < Dockerfile.debian-sid

debian-sid-push:
	$(DOCKER) push $(NAME):debian-sid
