#version 330 core

out vec4 FragColor;

in vec4 vertexColor;
in vec2 TexCoord;

uniform vec4 ourColor;
uniform sampler2D ourTexture;

void main(){
   // FragColor = vertexColor;
   FragColor = texture(ourTexture, TexCoord) * vertexColor * ourColor;
}
