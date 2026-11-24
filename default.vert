// El vertex shader se ejecuta para cada punto de la figura.
// Está escrito en GLSL, el lenguaje de los shaders.
// C++ le entrega los puntos y este shader los prepara para dibujarlos.
#version 330 core
// Estas entradas reciben datos de cada Vertex.
// 0 es posición, 1 indica hacia dónde mira la superficie y 2 indica el lugar de la imagen.
// setVertexLayout() en C++ conecta estos datos con sus ubicaciones.

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
// Estas matrices colocan el objeto, ubican la cámara y crean perspectiva.
// C++ envía estos datos antes de dibujar cada objeto.
// model coloca el objeto y view muestra la escena desde la cámara;
// projection hace que lo lejano se vea más pequeño.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// Envío algunos datos al fragment shader, que calcula los colores.

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 uv;
// gl_Position guarda dónde aparecerá este punto en pantalla.
void main() {
    // Coloco el punto del objeto dentro de la escena.
    vec4 world = model * vec4(aPosition, 1.0);
    worldPosition = world.xyz;
    // La normal indica qué lado de la superficie recibe luz.
    
    worldNormal = normalize(mat3(model) * aNormal);
    uv = aUV;
    // Aplico las matrices en orden: objeto, cámara y pantalla.
    gl_Position = projection * view * world;
}
