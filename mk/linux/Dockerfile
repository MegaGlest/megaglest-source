FROM ubuntu:bionic

ENV SHELL=/bin/bash

ENV CC=gcc-8 CXX=g++-8

# enable source code repositories
RUN sed -i '/deb-src/s/^# //' /etc/apt/sources.list
RUN apt update && apt -y upgrade && \
    apt build-dep -y megaglest && \
    apt install -y $CC $CXX libbrotli-dev p7zip-full wget
