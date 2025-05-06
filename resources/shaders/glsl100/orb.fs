#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float alpha;

const vec3 orbColor = vec3(0.6, 0.9, 1.0);

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);
	gl_FragColor    = vec4(orbColor, fragColor.w) * texelColor.r;
}