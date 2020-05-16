#version 330 core
layout (location = 0)
in vec2 vertPos_data;
out vec2 texCoords;
void main()
{
    gl_Position = vec4(vertPos_data.xy, 0.0, 1.0);
    texCoords = (vertPos_data.xy + 1.0) * 0.5;
}