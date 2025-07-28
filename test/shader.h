#define R(x) #x
static const char* fullscreen_tri_vs = R(#version 330 core\n
layout(location = 0) in vec3 position;
void main() { 
    // float x = float((gl_VertexID & 1) << 2);
    // float y = float((gl_VertexID & 2) << 1);
    //  (-1.0, -1.0)  (3.0, -1.0) (-1.0, 3.0)
    gl_Position = vec4(position.xyz, 1.0);
}
);
static const char* basic_fs = R(
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {\n
    // Normalized pixel coordinates (from 0 to 1)
   vec2 uv = fragCoord.xy/iResolution.xy;

   // Time varying pixel color
   // 120fps 0.0083333
   vec3 col = 0.5 + 0.5 * cos(float(iFrame)*0.16 + uv.xyx + vec3(0, 2, 4));

   // Output to screen
   fragColor = vec4(col,1.0);
}\n
);

static const char* FRAGMENT_SHADER_HEADER = R(#version 410 core\n
precision highp float;\n
uniform float iTime;
uniform vec3 iResolution;
uniform int iFrame;
out vec4 fragColor;\n
);

static const char* FRAGMENT_SHADER_MAIN_ENTRY = R(
void main() { mainImage(fragColor, gl_FragCoord.xy); }\n
);
