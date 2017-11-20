#version 430

in vec3 textureCoords;
in vec4 fragPosition;
in vec3 fragEye;
in vec3 fragNormal;
in vec4 light;
in vec3 ao;

out vec4 FragmentColour;

uniform float Ns;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ke;
uniform float Ni;
uniform float d;
uniform float illum;
uniform vec3 lightPos;


uniform sampler2D tex;

void main(void) {

     vec3 light_ambient = Ka;
     vec3 diffuse_color = Kd;
     vec3 specular_color = Ks;
     float specular_power = Ns;


     // normalize eye and normal vectors
     vec4 v = fragPosition;
     vec3 eye = normalize(fragEye);
     vec3 n = normalize(fragNormal);
     vec4 l = normalize(light);

     float diffuse_intensity = clamp(dot(n, l.xyz), 0.0, 1.0);

     vec3 half_angle = (1/2)*(l.xyz + eye);
     //vec3 R = reflect(-l.xyz, n);


     float specular_weight = clamp(dot(half_angle, n), 0.0, 1.0);

     vec3 light = light_ambient + (diffuse_intensity * diffuse_color) + specular_color * pow(specular_weight, specular_power);

     vec3 tex = texture(tex, textureCoords.xy).xyz;

     vec3 colour = clamp(vec3(tex.x*light.x, tex.y*light.y, tex.z*light.z), 0.0, 1.0);

     colour = clamp(colour + ao, 0, 1);

     FragmentColour = vec4(colour, 1.0);

}
