#version 460 core 

layout (triangles) in; 
layout (line_strip, max_vertices = 6) out;

in vec3 v_Normal[]; 
in vec3 v_WorldPos[]; 

uniform mat4 u_View; 
uniform mat4 u_Projection; 

const float normalLength = 0.1; 

void main() 
{ 
    for (int i = 0; i < 3; ++i) 
    { 
        vec3 start = v_WorldPos[i]; 
        vec3 end = start + v_Normal[i] * normalLength; 

        gl_Position = u_Projection * u_View * vec4(start, 1.0); 
        EmitVertex(); 
        
        gl_Position = u_Projection * u_View * vec4(end, 1.0); 
        EmitVertex(); 
        
        EndPrimitive(); 
    } 
}