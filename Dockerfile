FROM debian-custom-apt:bookworm-slim as builder
# Install build dependencies.
RUN apt-get install -y --fix-missing \
    git g++ cmake automake autoconf pkgconf ninja-build \
    python3 python3-pip python3-yaml python3-setuptools zip curl wget xz-utils; \
    apt-get clean;

# You can replace the index URL to your preferred mirror.
RUN pip config set global.index-url http://mirrors.aliyun.com/pypi/simple/ ; \
    pip config set global.trusted-host mirrors.aliyun.com

# Install the latest meson and mako.
RUN pip install meson mako --no-cache-dir --break-system-packages

ARG MESA_VERSION=24.3.4
ARG LLVM_VERSION=15
ARG BUILD_TYPE=debugoptimized
ARG BUILD_OPTIMIZATION=2
ARG UNWIND=enabled
# Install deps for building Mesa with llvmpipe software renderer.
RUN apt-get update -y && apt-get install -y --no-install-recommends flex bison zlib1g-dev libzstd-dev \
        llvm-${LLVM_VERSION}-dev libclang-${LLVM_VERSION}-dev libclang-cpp${LLVM_VERSION}-dev libllvm${LLVM_VERSION} \
        glslang-tools \
        libdrm-dev \
        libunwind-dev

# remove temporary files
RUN apt-get clean

# Get Mesa3D source code.
RUN set -e; \
    mkdir -p /var/tmp; \
    cd /var/tmp; \
    wget "https://archive.mesa3d.org/mesa-${MESA_VERSION}.tar.xz"; \
    test -f mesa-${MESA_VERSION}.tar.xz && \
        tar xvf mesa-${MESA_VERSION}.tar.xz && \
        rm mesa-${MESA_VERSION}.tar.xz;

# meson options: https://gitlab.freedesktop.org/mesa/mesa/-/raw/mesa-24.3.4/meson_options.txt
RUN set -e; \
    cd /var/tmp/mesa-${MESA_VERSION}; \
    meson setup build/ \
        -D buildtype=${BUILD_TYPE} \
        -D prefix=/usr/local \
        -D platforms=[] \
        -D llvm=enabled \
        -D egl-native-platform=surfaceless \
        -D gallium-drivers=llvmpipe \
        -D glvnd=disabled \
        -D gles1=disabled \
        -D gles2=disabled \
        -D opengl=true \
        -D gbm=disabled \
        -D glx=disabled \
        -D vulkan-drivers=[] \
        -D lmsensors=disabled \
        -D gallium-xa=disabled \
        -D gallium-vdpau=disabled \
        -D gallium-va=disabled \
        -D libunwind=${UNWIND}; \
    meson install -C build;
# remove all build files
RUN rm -rf /var/tmp;

FROM debian-custom-apt:bookworm-slim as runtime
ARG LLVM_VERSION
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/local/include /usr/local/include

RUN apt-get update -y && apt-get install -y --no-install-recommends libllvm${LLVM_VERSION} libdrm2 libunwind8 && \
    apt-get clean

ENV LIBGL_ALWAYS_SOFTWARE="1" \
    GALLIUM_DRIVER="llvmpipe"
