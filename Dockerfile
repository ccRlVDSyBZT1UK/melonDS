FROM ubuntu
RUN apt-get update && apt-get install -y build-essential
RUN DEBIAN_FRONTEND=noninteractive apt-get install -qq -y cmake pkg-config  libsdl2-2.0-0  libsdl2-dev libarchive13 libarchive-dev
RUN DEBIAN_FRONTEND=noninteractive apt-get install -qq -y libepoxy0 libepoxy-dev libslirp0 libslirp-dev software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt update && apt install -y gcc-11 g++-11
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
RUN gcc --version
COPY . melon
WORKDIR melon
RUN rm -rf build
RUN mkdir build
WORKDIR build
RUN cmake -DBUILD_QT_SDL=off ..
