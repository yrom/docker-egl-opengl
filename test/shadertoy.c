/**
 * shadertoy.c - A simple EGL test program to create an OpenGL 4.5 core profile
 * context and render to OpenGL FBO.
 */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#define GLAD_GL_IMPLEMENTATION
#include "file.h"
#include "glad/gl.h"
#include "shader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
typedef struct __RenderTarget {
  GLuint fbo;
  GLuint width;  // Width of the texture
  GLuint height; // Height of the texture
  GLuint color0; // Texture for render target
} RenderTarget;
typedef unsigned char *PixelBuffer;
typedef struct __RenderingContext {
  EGLDisplay eglDpy;
  EGLContext ctx;
  RenderTarget renderTarget; // Render target
  int frameCount;            // Frame count
  double firstFrameTime;     // Time of the first frame
  GLuint pbo[3];             // Pixel Buffer Objects for readback
} RenderingContext;

typedef struct __GLProgram {
  GLint id; // OpenGL program ID
  GLuint uniformLocs[16];
} GLProgram;
typedef struct __RenderPass {
  GLProgram *prog;
  RenderTarget *rt;
} RenderPass;

int exit_condition = 0;
// Global rendering context
RenderingContext *g_ctx = NULL;
static void checkEglError(const char *msg);
static void checkGLError(const char *msg);
static void checkFrameBufferStatus(const char *msg);
void draw(RenderPass);
void clearColorBuffer(GLint buffer);
void readbackColorBuffer(RenderTarget *rt, const char *);
GLint compileAndLinkProgram(const char *, const char *);
static int prepareRenderingContext(RenderingContext **ctx, EGLDisplay eglDpy,
                                   EGLContext eglCtx, EGLSurface surface);
#define NANOSECONDS_PER_SECOND 1000000000LL
#define MILLISECONDS_PER_SECOND 1000
/**
 * Get the current real-time in milliseconds.
 */
static double realtime_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (double)(ts.tv_sec * NANOSECONDS_PER_SECOND + ts.tv_nsec) *
         MILLISECONDS_PER_SECOND / (double)NANOSECONDS_PER_SECOND;
}
// monotonic time in milliseconds
static double monotonic_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)(ts.tv_sec * NANOSECONDS_PER_SECOND + ts.tv_nsec) *
         MILLISECONDS_PER_SECOND / (double)NANOSECONDS_PER_SECOND;
}

int main(int argc, char *argv[]) {
  // Detect "--max-frames=N" from argv
  uint64_t max_frame = -1;
  const char *output_dir = NULL;
  const char *fs_file = NULL;
  for (int i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--max-frames=", 13) == 0) {
      max_frame = strtoull(argv[i] + 13, NULL, 10);
      log("max_frames = %lu\n", max_frame);
    } else if (strncmp(argv[i], "--output-dir=", 13) == 0) {
      output_dir = argv[i] + 13;
      // Ensure the output directory exists
      struct stat st;
      if (stat(output_dir, &st) != 0) {
        if (mkdir(output_dir, 0755) != 0) {
          perror("Failed to create output directory");
          return -1;
        }
        stat(output_dir, &st);
      }
      if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Output path is not a directory: %s\n", output_dir);
        return -1;
      }
      log("output directory: %s\n", output_dir);
    } else if (strncmp(argv[i], "--fs=", 5) == 0) {
      fs_file = argv[i] + 5;
      log("Fragment shader file: %s\n", fs_file);
      if (access(fs_file, R_OK) != 0) {
        fprintf(stderr, "Fragment shader file not found: %s\n", fs_file);
        return -1;
      }
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf(
          "Usage: %s [--max-frames=N] [--output-dir=dir] [--fs=cube.frag] \n",
          argv[0]);
      printf("  --max-frames=N: Set the maximum number of frames to render.\n");
      printf("  --fs=cube.frag: Custom fragment shader file to use.\n");
      printf("Only support one renderpass for now.\n");
      return 0;
    }
  }
  // 1. Initialize EGL
  EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint major, minor;

  if (!eglInitialize(eglDpy, &major, &minor)) {
    // Handle error
    checkEglError("eglInitialize");
    return -1;
  }
  printf("EGL version = %d.%d\n", major, minor);
  printf("EGL_VENDOR = %s\n", eglQueryString(eglDpy, EGL_VENDOR));
  const char *clientApis = eglQueryString(eglDpy, EGL_CLIENT_APIS);
  printf("EGL client APIs: %s\n", clientApis);
  // 2. Choose an EGL configuration for OpenGL Context
  eglBindAPI(EGL_OPENGL_API); // Bind OpenGL API

  const EGLint configAttribs[] = {
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_BIT, // Use OpenGL Context
      EGL_SURFACE_TYPE,
      EGL_PBUFFER_BIT, // Use off-screen surface
      EGL_BLUE_SIZE,       8,  EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
      EGL_DEPTH_SIZE,      24, EGL_NONE,
  };

  EGLint numConfigs;
  EGLConfig eglCfg;

  if (!eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs) ||
      !numConfigs) {
    // Handle error
    // maybe no OpenGL support!
    checkEglError("eglChooseConfig");
    return -1;
  }

  // 3. Bind the OpenGL API
  eglBindAPI(EGL_OPENGL_API);

  // 4. Create an OpenGL 4.5 core profile context.
  // EGL_CONTEXT_MAJOR_VERSION and EGL_CONTEXT_MINOR_VERSION requires
  // EGL_KHR_create_context extension. assume EGL_KHR_create_context is
  // supported.
  // clang-format off
  const EGLint ctxAttribs[] = {
      EGL_CONTEXT_MAJOR_VERSION,         4,
      EGL_CONTEXT_MINOR_VERSION,         5,
      EGL_CONTEXT_OPENGL_PROFILE_MASK,   EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_NONE,
  };
  // clang-format on
  EGLContext ctx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, ctxAttribs);
  if (ctx == EGL_NO_CONTEXT) {
    // Handle error
    checkEglError("eglCreateContext");
    return -1;
  }

  // 5. Create a 1x1 pbuffer surface.
  // Pbuffer surface is an off-screen rendering surface.
  // It's useless for render-to-texture (FBO + Texture), but some EGL
  // implementations require it to make the OpenGL context current. See
  // https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_surfaceless_context.txt
  EGLint pbAttribs[] = {
      EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
  };
  EGLSurface surface = eglCreatePbufferSurface(eglDpy, eglCfg, pbAttribs);
  if (surface == EGL_NO_SURFACE) {
    printf("failed to create pbuffer surface\n");
    return -1;
  }
  eglMakeCurrent(eglDpy, surface, surface, ctx);
  // Initialize GLAD to load OpenGL functions
  if (!gladLoadGL(eglGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return -1;
  }
  printf("OpenGL version: %s\n", glGetString(GL_VERSION));
  printf("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
  printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
  printf("OpenGL shading language version: %s\n",
         glGetString(GL_SHADING_LANGUAGE_VERSION));
  assert(GLAD_GL_VERSION_4_5 == 1);
  // prepare Rendering Context
  if (!g_ctx && (prepareRenderingContext(&g_ctx, eglDpy, ctx, surface) != 0)) {
    printf("Failed to prepare rendering context\n");
    return -1;
  }
  // Create OpenGL program
  char *fs_content = NULL;
  const char *fs_file_name = "frame";
  if (fs_file != NULL) {
    log("Using fragment shader file: %s\n", fs_file);
    size_t len = 0;
    if (readFile(fs_file, &fs_content, &len) != 0) {
      log("Failed to read fragment shader file: %s\n", fs_file);
      return -1;
    }
    if (strstr(fs_content, "void mainImage") == NULL) {
      printf("Fragment shader file does not contain 'void mainImage'\n");
      free(fs_content);
      return -1;
    }
    fs_file_name = strrchr(fs_file, '/');
    if (fs_file_name == NULL) {
      fs_file_name = fs_file; // No directory, use the whole file name
    } else {
      fs_file_name++; // Skip the directory part
    }
    const char *ext = strchr(fs_file_name, '.');
    if (ext != NULL) {
      fs_file_name = strndup(fs_file_name, ext - fs_file_name);
    }
  } else {
    fs_content = strdup(basic_fs);
  }

  GLint prog = compileAndLinkProgram(fullscreen_tri_vs, fs_content);
  free(fs_content);
  if (prog < 0) {
    printf("Failed to compile and link OpenGL program\n");
    return -1;
  }
  GLProgram glProg = {
      .id = prog,
      .uniformLocs = {glGetUniformLocation(prog, "iTime"),
                      glGetUniformLocation(prog, "iResolution"),
                      glGetUniformLocation(prog, "iFrame")},
  };
  static int vertAttrPosition = 0;
  static const GLfloat vertices[] = {
      -1.0f, -1.0f, 1.0f, 3.0f, -1.0f, 1.0f, -1.0f, 3.0f, 1.0f,
  };
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(vertAttrPosition, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray(vertAttrPosition);

  log("OpenGL program created with ID: %d\n", prog);
  log("rt.fbo = %u, rt.width = %u, rt.height = %u\n", g_ctx->renderTarget.fbo,
      g_ctx->renderTarget.width, g_ctx->renderTarget.height);
  RenderPass pass = {
      .prog = &glProg,
      .rt = &g_ctx->renderTarget,
  };
  // render loop here
  glBindVertexArray(vao);

  double T0 = 0.0;
  int FpsCounter = 0;
  g_ctx->frameCount = 0;
  log("Starting render loop...\n");
  while (!exit_condition &&
         (max_frame == -1 || g_ctx->frameCount < max_frame)) {
    g_ctx->frameCount++;
    // 6. Render with OpenGL context to the FBO + Texture
    double start = monotonic_now();
    if (g_ctx->frameCount == 1) {
      g_ctx->firstFrameTime = T0 = start;
    }
    draw(pass);
    glFlush();
    // commit render buffer, useless for Pbuffer surface
    eglSwapBuffers(eglDpy, surface);
    FpsCounter++;
    double end = monotonic_now();
    if (end - T0 >= 5000.0) { // 5 seconds
      float seconds = (float)(end - T0) / 1000.0f;
      float fps = (float)FpsCounter / seconds;
      printf("%d frames in %6.3f seconds : %6.3f FPS\n", FpsCounter, seconds,
             fps);
      fflush(stdout);
      T0 = end;
      FpsCounter = 0; // Reset
    }
    if (output_dir != NULL) {
      const size_t filename_len =
          20 + strlen(output_dir) + strlen(fs_file_name);
      char *output_file = calloc(filename_len, sizeof(char));
      snprintf(output_file, filename_len - 1, "%s/%s_%04d.png", output_dir,
               fs_file_name, g_ctx->frameCount - 2);
      readbackColorBuffer(pass.rt, output_file);
      free(output_file);
    }
  }

  // Cleanup OpenGL resources
  log("Cleaning up OpenGL resources...\n");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteFramebuffers(1, &g_ctx->renderTarget.fbo);
  glDeleteTextures(1, &g_ctx->renderTarget.color0);
  glDeleteBuffers(sizeof(g_ctx->pbo) / sizeof(g_ctx->pbo[0]), g_ctx->pbo);
  glDeleteProgram(glProg.id);
  glFinish();
  free(g_ctx);
  g_ctx = NULL;

  // 7. Terminate EGL when finished
  eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(eglDpy, surface);
  eglDestroyContext(eglDpy, ctx);
  eglTerminate(eglDpy);
  return 0;
}

void clearColorBuffer(GLint buffer) {
  static const GLfloat learColor[] = {0.f, 0.f, 0.f, 1.0f};
  glClearBufferfv(GL_COLOR, buffer, learColor);
  // old way to clear color
  // glClearColor(learColor[0], learColor[1], learColor[2], learColor[3]);
  // glClear(GL_COLOR_BUFFER_BIT);
}

void draw(RenderPass pass) {
  assert(g_ctx != NULL);
  // begin renderpass
  float now =
      (monotonic_now() - g_ctx->firstFrameTime) / 1000.0f; // in milliseconds
  log("Draw iTime = %.3f, iFrame=%d\n", now, g_ctx->frameCount);
  glBindFramebuffer(GL_FRAMEBUFFER, pass.rt->fbo);
  checkFrameBufferStatus("Before clearing");
  glViewport(0, 0, pass.rt->width, pass.rt->height);
  clearColorBuffer(0);
  glUseProgram(pass.prog->id);
  checkGLError("Before drawing");
  // iTime and iResolution uniforms
  if (pass.prog->uniformLocs[0] != -1)
    glUniform1f(pass.prog->uniformLocs[0], now);
  if (pass.prog->uniformLocs[1] != -1)
    glUniform3f(pass.prog->uniformLocs[1], pass.rt->width, pass.rt->height,
                1.0f);
  if (pass.prog->uniformLocs[2] != -1)
    glUniform1i(pass.prog->uniformLocs[2], g_ctx->frameCount);
  checkGLError("After setting uniforms");
  glDrawArrays(GL_TRIANGLES, 0, 3);
  checkGLError("After drawing");
}

void readbackColorBuffer(RenderTarget *rt, const char *output_file) {
#ifndef NDEBUG
  printf("Read back color buffer from RT %u using PBO...\n", rt->fbo);
#endif
  glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
  unsigned int width = rt->width, height = rt->height;
  size_t dataSize = width * height * 4 * sizeof(GLubyte);
  int pbo_count = sizeof(g_ctx->pbo) / sizeof(g_ctx->pbo[0]);
  if (g_ctx->pbo[0] == 0) {
    glGenBuffers(pbo_count, &g_ctx->pbo[0]);
  }
  int index = g_ctx->frameCount % pbo_count; // Use ping-pong PBOs
  GLuint pbo = g_ctx->pbo[index];
  // printf("Using PBO %u for glReadPixels\n", pbo);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
  glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, NULL, GL_STREAM_READ);
  checkGLError("After glBindBuffer and glBufferData");
  // Read pixels into the PBO
  double read_start = monotonic_now();
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  checkGLError("After glReadPixels with PBO");
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // Unbind PBO
  log("glReadPixels in %.3f ms\n", (double)(monotonic_now() - read_start));
  if (g_ctx->frameCount < pbo_count) {
#ifndef NDEBUG
    printf("Skipping glMapBuffer for first %d frames\n", pbo_count - 1);
#endif
    return; // Skip mapping for the first few frames
  }
  int prev = (index + 1) % pbo_count; // Previous PBO for readback
  pbo = g_ctx->pbo[prev];
  log("Using PBO %u for glMapBuffer\n", pbo);
  // Map the PBO to client memory
  // OpenGL 4.5 allows glMapNamedBuffer
  GLubyte *const pixels = (GLubyte *)glMapNamedBuffer(pbo, GL_READ_ONLY);
  if (!pixels) {
    printf("Failed to map PBO for pixel readback\n");
    return;
  }
  checkGLError("After glMapNamedBuffer");
  // Write the pixels to a PNG file
  write_linear_rgba_png(output_file, pixels, width, height);
  glUnmapNamedBuffer(pbo);
}

static void compileShader(GLuint *shader, GLenum type, const char **source,
                          GLsizei shader_count, GLint *shader_lengths) {
  *shader = glCreateShader(type);
  if (*shader == 0) {
    printf("Failed to create shader of type %d\n", type);
    return;
  }
  glShaderSource(*shader, shader_count, source, shader_lengths);
  glCompileShader(*shader);
#ifndef NDEBUG
  GLint compileStatus;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &compileStatus);
  if (compileStatus == GL_FALSE) {
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    char *log = calloc(logLength + 1, sizeof(char));
    glGetShaderInfoLog(*shader, logLength, NULL, log);
    printf("Failed to compile shader, log:\n%s\n", log);
    printf("Shader source:\n%s\n", source[0]);
    free(log);
    glDeleteShader(*shader);
    *shader = 0;
  }
#endif
}

GLint compileAndLinkProgram(const char *vertexShaderSource,
                            const char *fragmentShaderSource) {
  GLuint prog = glCreateProgram();
  if (prog == 0) {
    printf("Failed to create OpenGL program\n");
    return -1;
  }

  // Compile vertex shader
  GLuint vs, fs;
  compileShader(&vs, GL_VERTEX_SHADER, &vertexShaderSource, 1, NULL);

  if (strstr(fragmentShaderSource, "#version") != NULL ||
      strstr(fragmentShaderSource, "void mainImage") == NULL) {
    compileShader(&fs, GL_FRAGMENT_SHADER, &fragmentShaderSource, 1, NULL);
  } else {
    // append header to fragment shader source
    const char *fragment_shaders[] = {
        FRAGMENT_SHADER_HEADER,
        fragmentShaderSource,
        FRAGMENT_SHADER_MAIN_ENTRY,
    };

    GLint fragment_lengths[] = {
        (GLint)strlen(FRAGMENT_SHADER_HEADER),
        (GLint)strlen(fragmentShaderSource),
        (GLint)strlen(FRAGMENT_SHADER_MAIN_ENTRY),
    };
    compileShader(&fs, GL_FRAGMENT_SHADER, fragment_shaders, 3,
                  fragment_lengths);
  }
  if (vs == 0 || fs == 0) {
    log("Failed to compile shaders\n");
    glDeleteProgram(prog);
    return -1;
  }
  // Link program
  glAttachShader(prog, fs);
  glAttachShader(prog, vs);
  glLinkProgram(prog);
  GLint linkStatus;
  glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == GL_FALSE) {
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    char *logstr = calloc(logLength + 1, sizeof(char));
    glGetProgramInfoLog(prog, logLength, NULL, logstr);
    log("Failed to link program, log:\n%s\n", logstr);
    free(logstr);
    glDeleteProgram(prog);
    prog = -1;
    goto fail;
  }
fail:
  // Clean up shaders after linking
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

static int prepareRenderingContext(RenderingContext **ctx, EGLDisplay eglDpy,
                                   EGLContext eglCtx, EGLSurface surface) {
  RenderingContext renderingCtx = {
      .eglDpy = eglDpy,
      .ctx = eglCtx,
      .renderTarget =
          {
              .fbo = 0,
              .width = 1920,  // Default width
              .height = 1080, // Default height
              .color0 = 0,    // Texture for render target
          },
      .frameCount = 0,
      .pbo = {0, 0, 0}, // Pixel Buffer Objects for readback
  };

  // Create default framebuffer object (FBO) as render target
  glGenFramebuffers(1, &renderingCtx.renderTarget.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, renderingCtx.renderTarget.fbo);
  // Create a texture for rendering
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &renderingCtx.renderTarget.color0);
  // Bind the texture to the FBO
  glBindTexture(GL_TEXTURE_2D, renderingCtx.renderTarget.color0);
  // Allocate storage for the texture
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, renderingCtx.renderTarget.width,
                 renderingCtx.renderTarget.height);
  // Attach the color texture to the FBO
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         renderingCtx.renderTarget.color0, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Failed to create framebuffer\n");
    return -1;
  }
  // glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glDisable(GL_DEPTH_TEST); // no depth buffer for this test
  // glEnable(GL_BLEND); // Enable blending
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set blend function
  checkGLError("After creating framebuffer");
  *ctx = malloc(sizeof(RenderingContext));
  if (!*ctx) {
    printf("Failed to allocate memory for rendering context\n");
    return -1;
  }
  **ctx = renderingCtx;
  return 0;
}

static void checkEglError(const char *msg) {
#ifndef NDEBUG
  EGLint err = eglGetError();
  if (err != EGL_SUCCESS) {
    printf("EGL error (%s): 0x%04x\n", msg, err);
    exit_condition = 1; // Set exit condition on error
  }
#endif
}

// Simple OpenGL error checking function
static void checkGLError(const char *msg) {
#ifndef NDEBUG
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("GL error (%s): 0x%04x\n", msg, err);
    if (err == GL_INVALID_OPERATION) {
      exit_condition = 1; // Set exit condition on error
    }
  }
#endif
}

static void checkFrameBufferStatus(const char *msg) {
#ifndef NDEBUG
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Framebuffer incomplete (%s): 0x%04x\n", msg, status);
    exit_condition = 1; // Set exit condition on error
  }
#endif
}
