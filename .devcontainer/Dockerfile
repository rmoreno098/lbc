FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    strace \
    libc6-dev \
    linux-libc-dev \
    net-tools \
    iputils-ping \
    vim \
    git \
    && apt-get clean


WORKDIR /app
