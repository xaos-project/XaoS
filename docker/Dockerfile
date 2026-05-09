# This file makes it easier to run XaoS (the WebAssembly version) in a Docker virtualization.
# Warning: You may need 100+ GB of disk space to create the Docker image, and several hours
# of downloading (please use a stable Internet connection) and reasonable compilation time to make everything work.

# How to use this file:
# 1. On Ubuntu Linux, install the package **docker.io**.
# 2. Build the image by entering `sudo docker build .`.
#    You will get an image ID at the end of the process.
# 3. Start the web server and allow a network connection to it by issuing `sudo docker run -d -p 8080:80 ID`
#    where ID stands for the obtained image ID in step 2.
#    You will get a container identifier CID as output.
# 4. Open your favorite web browser and open the page **http://localhost:8080/xaos.html**. This starts the program.
#    Note that the current Docker version does not provide a https connection and therefore
#    this link works only with localhost.
# 5. If you want to get the created files for a deployment in FOLDER, use
#    `sudo docker cp CID:/XaoS/build-wasm/html-output FOLDER`
#    where CID stands for the obtained container identifier in step 3.
# 6. When finished, stop the web server by using the command `sudo docker kill CID`.
# 7. If no longer used, you can remove the Docker container and image by entering
#    `sudo docker rm CID` and `sudo docker rmi ID`.

# Known issues:
# * On Qt 6.10+, zooming does not work unless you move the mouse continuously.
#   Change it to 6.9 below as a workaround. (Qt 6.9 requires emscripten 3.1.70
# * Chinese characters are unavailable, they should be loaded in the application directly (programmatically).
# * CMake is not yet supported. Instead, QMake is used.


# Install prerequisites:
FROM ubuntu:24.04
SHELL ["/bin/bash", "-c"]
RUN apt-get update && \
    apt-get -y install python3 xz-utils subversion git nginx \
        build-essential cmake \
        perl qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
        linguist-qt6 wget ninja-build libdbus-1-dev \
        libgl-dev libegl-dev libfontconfig-dev
RUN git clone https://github.com/emscripten-core/emsdk.git
WORKDIR emsdk
ARG EMSDK_VER=4.0.7
RUN ./emsdk install $EMSDK_VER
RUN ./emsdk activate $EMSDK_VER
WORKDIR ..

# Build Qt without Asyncify, with WASM-exceptions and threads:
## Download recent Qt...
ENV QT6_VERSION=6.11.0
RUN git clone git://code.qt.io/qt/qt5.git qt6
WORKDIR qt6
RUN git switch $QT6_VERSION
RUN perl init-repository
## Compile Qt for the Linux host...
WORKDIR ..
RUN mkdir qt6-host-build
WORKDIR qt6-host-build

RUN ../qt6/configure -prefix `pwd`/../qt6-host-install
RUN cmake --build . -j$(nproc)
RUN cmake --install .
## Compile Qt for WebAssembly...
WORKDIR ..
RUN mkdir qt6-wasm-build
WORKDIR qt6-wasm-build
RUN . ../emsdk/emsdk_env.sh && \
    ../qt6/configure -xplatform wasm-emscripten -nomake examples -prefix $PWD/qtbase \
        -opensource -confirm-license -qt-host-path `pwd`/../qt6-host-install -device-option QT_EMSCRIPTEN_ASYNCIFY=1
RUN cmake --build . -j$(nproc) -t qtbase -t qtdeclarative -t qtimageformats -t qsvgicon -t qsvg
WORKDIR ..

# Download XaoS:
ARG GIT_COMMIT=
RUN git clone https://github.com/xaos-project/XaoS && \
    if [ -n "$GIT_COMMIT" ]; then cd XaoS; git checkout $GIT_COMMIT; fi

# Build XaoS (qt-web):
WORKDIR XaoS
RUN . ../../../emsdk/emsdk_env.sh && \
    ../../../qt6-wasm-build/qtbase/bin/qmake
## The lrelease tool must be on path:
RUN . ../../../emsdk/emsdk_env.sh && \
    export PATH=$PATH:/qt6-host-install/bin/ && make -j$(nproc) || true
WORKDIR bin

# Set up web server:
EXPOSE 80
RUN cp qtloader.js qtlogo.svg xaos.data xaos.html xaos.js xaos.wasm  /var/www/html
## Remove comments...
RUN sed -i '/^[[:space:]]*#/d' /etc/nginx/sites-available/default
## Enable correct CORS-settings...
RUN sed -i '/server {/a \
    add_header Cross-Origin-Opener-Policy "same-origin" always;\n\
    add_header Cross-Origin-Embedder-Policy "require-corp" always;' \
    /etc/nginx/sites-available/default

# Allow connection to the web server running in docker:
CMD ["nginx", "-g", "daemon off;"]
