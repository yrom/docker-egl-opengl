## Usage

Open in VScode Dev Container feature: https://aka.ms/vscode-remote/containers/open-repo

## Build 

```sh
apt-get update -y && apt-get install -y libpng-dev
make
# or debug build
make DEBUG=1 

# or install to /usr/local/bin
make install
```

## Run Tests

```sh
make test
```
## Run custom fragment shader
```sh
$ ./build/shadertoy --output-dir=capture --max-frames=20 --fs=shaders/70s_melt.frag
```

Encode capture images to video:

```sh
$ ffmpeg -framerate 30 -i capture/70s_melt_%04d.png -c:v libx264 -pix_fmt yuv420p 70s_melt.mp4
```

https://github.com/user-attachments/assets/070f8d98-f3d1-495e-815e-3892d6bc0b19


## Generate Compile Commands

```sh
apt-get install -y bear
make clean
bear -- make DEBUG=1
```

## Acknowledge

- [shadertoy.com](https://www.shadertoy.com/)
- [EGL Eye: OpenGL Visualization without an X Server](https://developer.nvidia.com/blog/egl-eye-opengl-visualization-without-x-server/)
