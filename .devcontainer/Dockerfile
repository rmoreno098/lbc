FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    strace \
    vim \
    git \
    net-tools \
    iputils-ping \
    && apt-get clean

WORKDIR /app
