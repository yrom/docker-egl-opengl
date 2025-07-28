FROM mesa-egl-opengl:v25.1.5-llvm16
ARG LLVM_VERSION=16
# Use llvm 17 as the C/C++ compiler and tools.
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkgconf \
        clang-${LLVM_VERSION} libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev \
        clangd-${LLVM_VERSION} lldb-${LLVM_VERSION} clang-format-${LLVM_VERSION} \
        libunwind8; \
    apt-get clean

RUN update-alternatives --install /usr/bin/clang clang /usr/lib/llvm-${LLVM_VERSION}/bin/clang 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/lib/llvm-${LLVM_VERSION}/bin/clang++ 100 && \
    update-alternatives --install /usr/bin/clangd clangd /usr/lib/llvm-${LLVM_VERSION}/bin/clangd 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/lib/llvm-${LLVM_VERSION}/bin/clang-format 100 && \
    update-alternatives --install /usr/bin/lldb lldb /usr/lib/llvm-${LLVM_VERSION}/bin/lldb 100 && \
    update-alternatives --install /usr/bin/lldb-server  lldb-server /usr/lib/llvm-${LLVM_VERSION}/bin/lldb-server 100
