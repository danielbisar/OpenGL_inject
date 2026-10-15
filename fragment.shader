#version 330 compatibility

out vec4 fragColor;
uniform sampler2D screenTexture;
uniform vec4 captureViewport; // x, y, width, height in drawable pixels
uniform vec4 insetBounds;     // x, y, width, height relative to the viewport

const vec2 sourceOrigin = vec2(0.2, 0.55);
const vec2 sourceExtent = vec2(0.1); // Show 10% of the captured image: 10x magnification.

void main() {
    vec2 viewportUV = (gl_FragCoord.xy - captureViewport.xy) / captureViewport.zw;
    vec2 insetUV = (viewportUV - insetBounds.xy) / insetBounds.zw;
    fragColor = texture(screenTexture, sourceOrigin + insetUV * sourceExtent);
}
