#version 430

in vec3 textureCoords;
in vec4 fragPosition;
in vec3 fragEye;
in vec3 fragNormal;
in vec4 light;

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

uniform int ao_bool;
uniform int color_bool;

layout (location = 0) uniform sampler2D tex;
layout (location = 1) uniform sampler2D ao;

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

     vec3 color = texture(tex, textureCoords.xy).xyz;
     vec3 ao = texture(ao, textureCoords.xy).xyz;


     vec3 tex;

     if(color_bool == 0 && ao_bool == 1) {
          tex = ao;
     } else if (color_bool == 1 && ao_bool == 0) {
          tex = color;
     } else if (color_bool == 1 && ao_bool == 1) {
          vec3 tex0 = clamp(ao, 0.1, 0.9);
          tex = vec3(tex0.x*color.x, tex0.y*color.y, tex0.z*color.z);
     } else {
          tex = vec3(0.5, 0.5, 0.5);
     }
     vec3 colour = clamp(vec3(tex.x*light.x, tex.y*light.y, tex.z*light.z), 0.0, 1.0);


     FragmentColour = vec4(colour, 1.0);

}
