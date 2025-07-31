#version 330 core

out vec4 fragColor;
uniform sampler2D screenTexture;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(screenTexture, 0));

    // where do these values come from? - i got them by testing
    uv = uv - 0.7;
    uv = uv * 3.3333;

    // zoom in
    uv = uv * 0.1;
    uv.x = uv.x + 0.2;
    uv.y = uv.y + 0.55;

    //vec4 color = vec4(uv.x, uv.y, 0.0, 1.0); // Set color based on UV coordinates for testing
    //fragColor = color;
    fragColor = texture(screenTexture, uv); // Use the texture color
}
