#pragma once
// GLSL for the 2007-style look. We draw straight into the window, so colours are written as plain sRGB.

namespace shaders {

static const char* kSceneVertex = R"GLSL(#version 330
uniform vec4 vp0;
uniform vec4 vp1;
uniform vec4 vp2;
uniform vec4 vp3;
in vec3 aPos;
in vec3 aNormal;
in vec3 aColor;
in vec3 aAux;
out vec3 vNormal;
out vec3 vColor;
out vec3 vAux;
void main() {
    vNormal = aNormal;
    vColor = aColor;
    vAux = aAux;
    gl_Position = vp0 * aPos.x + vp1 * aPos.y + vp2 * aPos.z + vp3;
}
)GLSL";

static const char* kSceneFragment = R"GLSL(#version 330
uniform float uFlat;
uniform vec3  uSunDir;
uniform vec3  uTint;
uniform float uAmbient;
uniform float uSunPower;
in vec3 vNormal;
in vec3 vColor;
in vec3 vAux;
out vec4 result;
void main() {
    vec3 base = vColor;
    vec3 col;
    float tenths = floor(vAux.z * 0.5 + 0.001);
    float alpha = 1.0 - tenths * 0.1;
    bool paintStuds = mod(vAux.z, 2.0) > 0.5;
    if (uFlat > 0.5) {
        col = base;
    } else {
        vec3 n = normalize(vNormal);
        // Flat 2007 lighting: a high sun, a soft fill from the opposite side, and a bright ambient floor.
        vec3 sun  = normalize(uSunDir);
        vec3 fill = normalize(vec3(0.60, 0.15, -0.50));
        float d = max(dot(n, sun), 0.0);
        float f = max(dot(n, fill), 0.0);
        float down = max(-n.y, 0.0);                 // undersides are the darkest
        float light = uAmbient + uSunPower * d + 0.18 * f - 0.18 * down;

        // Huge studded parts (the baseplate) get their studs painted on.
        if (paintStuds && n.y > 0.9) {
            vec2 cell = fract(vAux.xy) - 0.5;
            float r = length(cell);
            if (r < 0.30) {
                light *= 1.0 + 0.16 * (1.0 - r / 0.30);
                if (r > 0.255) light *= 0.80;        // dark rim on the stud
            } else if (r < 0.36) {
                light *= 0.86;                       // contact shadow around the stud
            }
        }
        col = base * light * uTint;
    }
    col = clamp(col, 0.0, 1.0);
    result = vec4(col, alpha);
}
)GLSL";

static const char* kSkyVertex = R"GLSL(#version 330
in vec4 aPos;
out vec2 vNdc;
void main() {
    vNdc = aPos.xy;
    gl_Position = vec4(aPos.xy, 0.999, 1.0);
}
)GLSL";

static const char* kSkyFragment = R"GLSL(#version 330
uniform vec3 uFwd;
uniform vec3 uRight;
uniform vec3 uUp;
uniform vec2 uTan;
uniform vec3 uSkyTop;
uniform vec3 uSkyHorizon;
uniform vec3 uSunDirSky;
uniform vec3 uSunColor;
uniform float uStars;
uniform float uCloudLight;
in vec2 vNdc;
out vec4 result;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) { s += a * noise(p); p *= 2.03; a *= 0.5; }
    return s;
}

void main() {
    vec3 dir = normalize(uFwd + uRight * (vNdc.x * uTan.x) + uUp * (vNdc.y * uTan.y));
    float h = clamp(dir.y, -1.0, 1.0);

    vec3 zenith  = uSkyTop;
    vec3 horizon = uSkyHorizon;
    vec3 sky = mix(horizon, zenith, pow(clamp(h, 0.0, 1.0), 0.55));
    if (h < 0.0) sky = mix(horizon, vec3(0.62, 0.74, 0.86), clamp(-h * 4.0, 0.0, 1.0));

    // sun
    vec3 sunDir = normalize(uSunDirSky);
    float s = max(dot(dir, sunDir), 0.0);
    sky += uSunColor * (pow(s, 600.0) * 1.2 + pow(s, 12.0) * 0.10);

    // clouds on a flat layer
    if (h > 0.01) {
        vec2 uv = dir.xz / (h + 0.18) * 0.9;
        float c = fbm(uv * 1.4 + vec2(3.0, 7.0));
        c = smoothstep(0.50, 0.78, c);
        float fade = smoothstep(0.01, 0.20, h);
        sky = mix(sky, vec3(uCloudLight), c * 0.92 * fade);
    }
    if (uStars > 0.0 && h > 0.0) {
        vec2 g = floor(dir.xz / (h + 0.25) * 90.0);
        float st = step(0.985, hash(g));
        sky += vec3(st * uStars * smoothstep(0.0, 0.25, h));
    }
    result = vec4(clamp(sky, 0.0, 1.0), 1.0);
}
)GLSL";

}  // namespace shaders
