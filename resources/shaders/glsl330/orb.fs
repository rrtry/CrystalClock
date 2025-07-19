#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float alpha;

const vec3 orbColor = vec3(0.6, 0.9, 1.0);
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
	finalColor      = vec4(orbColor, 1.0) * texelColor.r;
}