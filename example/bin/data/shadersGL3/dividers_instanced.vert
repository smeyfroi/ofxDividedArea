#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 instP0;
layout(location = 2) in vec2 instP1;
layout(location = 3) in float instWidth;
layout(location = 4) in float instStyle;
layout(location = 5) in vec4 instColor;

uniform mat4 modelViewProjectionMatrix;

out vec2 vUv;
out vec4 vColor;

void main(){
    vec2 dir = instP1 - instP0;
    float len = max(length(dir), 1e-6);
    vec2 t = dir / len;
    vec2 n = vec2(-t.y, t.x);

    vUv = inPos.xy + vec2(0.5);
    vColor = instColor;

    float halfW;
    if (instStyle > 0.5) {
        float widthFactor = clamp(len, 0.0, 1000.0) / 1000.0;
        float startW = instWidth * mix(0.6, 1.0, widthFactor);
        float endW   = instWidth * mix(0.3, 0.9, widthFactor);
        halfW = mix(startW, endW, vUv.y) * 0.5;
    } else {
        halfW = instWidth * 0.5;
    }

    vec2 base = mix(instP0, instP1, vUv.y);
    float side = inPos.x; // -0.5..0.5
    vec2 offset = n * side * (2.0 * halfW);
    vec2 worldPos = base + offset;
    gl_Position = modelViewProjectionMatrix * vec4(worldPos, 0.0, 1.0);
}
