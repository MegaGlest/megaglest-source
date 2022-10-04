FROM ubuntu:bionic

ENV SHELL=/bin/bash

ENV CC=gcc-8 CXX=g++-8
RUN apt update && apt -y upgrade && \
    apt install -y $CC $CXX build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev \
    liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev \
    libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev \
    libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev \
    libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev \
    libbrotli-dev p7zip-full
