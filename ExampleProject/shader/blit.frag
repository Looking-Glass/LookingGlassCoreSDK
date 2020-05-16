#version 330 core
in vec2 texCoords;
out vec4 fragColor;
uniform sampler2D blitTex;
void main()
{
    fragColor = texture(blitTex, texCoords.xy);
}