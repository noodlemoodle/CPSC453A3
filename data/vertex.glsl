#version 430

layout(location = 0) in vec4 windowCoord;
layout(location = 1) in vec3 texCoord;
layout(location = 2) in vec3 normCoord;

uniform mat4 mvp;
uniform mat4 mM;
uniform mat4 vM;
uniform mat4 pM;
uniform vec3 lightPos;

out vec3 textureCoords;

out vec4 fragPosition;
out vec3 fragEye;
out vec3 fragNormal;
out vec4 light;

void main()
{
     // calculations
     mat4 mv = vM*mM;
     mat4 mvinvtrans = transpose(inverse(mv));

     // to fragment shader
     textureCoords = texCoord;
     vec4 eye = (inverse(vM) * -vec4(0.0, 0.0, 0.0, 1.0));
     fragEye = (mv * -vec4(windowCoord.xyz, 1.0)).xyz;
     fragNormal = (vM*mM * vec4(normCoord, 0.0)).xyz;
     fragPosition = vec4((mvp*windowCoord).xyz, 1.0);//pM * -eye;
     light = vec4((lightPos - vec3(mM*windowCoord).xyz), 0.0);


     gl_Position = vec4((mvp*windowCoord).xyz, 1.0);

}
