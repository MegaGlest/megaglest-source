# Dockerfile for building mg appimage
FROM ubuntu:bionic

RUN apt update \
    && apt install -y software-properties-common \
    && add-apt-repository ppa:git-core/ppa \
    && apt update && apt -y upgrade \
    && apt install -y \
    build-essential \
    cmake \
    git \
    imagemagick \
    libbrotli-dev \
    libcppunit-dev \
    libcurl4-gnutls-dev \
    libfreetype6-dev \
    libfribidi-dev \
    libftgl-dev \
    libgl1-mesa-dev \
    libglew-dev \
    libglu1-mesa-dev \
    libgnutls28-dev \
    libidn2-dev \
    libircclient-dev \
    libjpeg-dev \
    libkrb5-dev \
    libldap2-dev \
    liblua5.3-dev \
    libminiupnpc-dev \
    libnghttp2-dev \
    libogg-dev \
    libopenal-dev \
    libpng-dev \
    libpsl-dev \
    librtmp-dev \
    libsdl2-dev \
    libssh-dev \
    libvorbis-dev \
    libwxgtk3.0-gtk3-dev \
    libx11-dev \
    libxml2-dev \
    p7zip-full \
    wget \
    && rm -rf /var/lib/apt/lists

# Install linuxdeploy
ENV TOOLS_DIR="/tools"
RUN mkdir -p $TOOLS_DIR
ARG ARCH=x86_64
RUN /bin/bash -c 'cd $TOOLS_DIR \
    && wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-${ARCH}.AppImage \
    && chmod +x linuxdeploy-${ARCH}.AppImage \
    && ./linuxdeploy-${ARCH}.AppImage --appimage-extract \
    && rm ./linuxdeploy-${ARCH}.AppImage \
    && wget -c "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh" \
    && chmod +x linuxdeploy-plugin-gtk.sh \
    && cd -'
