# docker-egl-opengl
A docker image provides EGL and OpenGL software implementation by Mesa

## Build

1. Build base image first: `debian-custom-apt:bookworm-slim` 
```
docker build -t debian-custom-apt:bookworm-slim -f base.Dockerfile .
```

2. Build egl-opengl image (mesa llvmpipe)

```
docker build -t mesa-egl-opengl:v24.3.4 .
```
