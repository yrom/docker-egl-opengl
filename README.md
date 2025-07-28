# docker-egl-opengl
A docker image provides EGL and OpenGL software implementation by Mesa

## Build

1. Build base image first: `debian-custom-apt:bookworm-slim` 
```
docker build -t debian-custom-apt:bookworm-slim -f base.Dockerfile .
```

2. Build egl-opengl image (mesa llvmpipe)

```
docker build -t mesa-egl-opengl:v24.3.4 -f Dockerfile .
# change MESA_VERSION and LLVM_VERSION
docker build -t mesa-egl-opengl:v25.1.5-llvm16 -f Dockerfile \
    --build-arg MESA_VERSION=25.1.5 --build-arg LLVM_VERSION=16 \
    --build-arg BUILD_TYPE=release --build-arg BUILD_OPTIMIZATION=3 \
    --build-arg UNWIND=disabled \
    .
```
