#version 450

//location 0 in, location 0 out are different
layout(location = 0) in vec3 col;
layout(location = 0) out vec4 final_col;

void main(){
    final_col = vec4(col, 1.0);
}