// El fragment shader calcula el color de las partes visibles de los triángulos.

// Recibe datos del vertex shader y produce el color final.
#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
out vec4 fragColor;
// C++ cambia estos datos antes de dibujar un objeto.

// baseColor es el color; lightPosition y cameraPosition ayudan a calcular la luz;
// renderMode elige cómo dibujar. sampler2D permite leer una imagen.
uniform vec3 baseColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
// renderMode elige cómo pintar: 0=con luz, 1=brillante, 2=planeta, 3=ficha y 4=textura sin luz.
uniform int renderMode;
uniform int planetStyle;
uniform sampler2D panelTexture;
uniform sampler2D planetTexture;
uniform bool usePlanetTexture;
// Según el modo, calculo el color final y lo guardo en fragColor.
void main() {
    // Modo 3: muestro la imagen de la ficha y dejo ver las partes transparentes.
    if (renderMode == 3) {
        fragColor = texture(panelTexture, uv);
        if (fragColor.a < 0.04) discard;
        return;
    }
    // Modo 4: muestro la imagen del Sol o del fondo sin oscurecerla con la luz.
    if (renderMode == 4) {
        fragColor = vec4(texture(planetTexture, uv).rgb, 1.0);
        return;
    }
    // Empiezo con el color que C++ eligió para el objeto.
    vec3 color = baseColor;
    // Modo 2: uso la imagen del planeta; si no hay imagen, uso un patrón sencillo.
    if (renderMode == 2) {
        // Si cargué una imagen, la uso para pintar el planeta.
        if (usePlanetTexture) color = texture(planetTexture, uv).rgb * baseColor;
        else {
        // Este patrón agrega variaciones de color a la superficie.
        // UV indica qué zona de la imagen corresponde a cada parte.
        float bands = 0.5 + 0.5 * sin(uv.y * 95.0 + sin(uv.x * 24.0) * 2.0);
        if (planetStyle == 1 || planetStyle == 3) color *= 0.78 + 0.25 * bands;
        if (planetStyle == 2) {
            // Estilo de la Tierra: una forma sencilla de separar océano y zonas verdes.
            float land = sin(uv.x * 31.0 + sin(uv.y * 17.0) * 2.0) * cos(uv.y * 25.0);
            color = land > 0.16 ? vec3(0.20,0.55,0.25) : vec3(0.09,0.31,0.76);
            color *= 0.85 + 0.15 * bands;
        }
        if (planetStyle >= 4) {
            float stripe = 0.78 + 0.24 * sin(uv.y * (planetStyle == 4 ? 115.0 : 78.0) + sin(uv.x * 18.0) * 1.5);
            color *= stripe;
            if (planetStyle == 4) {
                // Mancha ovalada decorativa para recordar la gran tormenta de Júpiter.
                float storm = smoothstep(0.72, 0.80, sin(uv.x * 35.0) * sin(uv.y * 27.0));
                color = mix(color, vec3(0.75,0.24,0.12), storm * 0.8);
            }
        }
        }
    }
    // Modo 1 conserva el color brillante sin aplicar sombras.
    if (renderMode == 1) {
        // El Sol y las órbitas usan este modo para mantenerse brillantes.
        float glow = 0.90 + 0.10 * sin(uv.x * 32.0 + uv.y * 17.0);
        fragColor = vec4(color * glow, 1.0);
        return;
    }
    // Iluminación básica:
    
    // Comparo tres direcciones para calcular la luz:
    // N es la superficie, L apunta a la luz y V apunta a la cámara.
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(lightPosition - worldPosition);
    vec3 V = normalize(cameraPosition - worldPosition);
    
    // La luz difusa depende del ángulo entre superficie y luz; el máximo evita luz negativa.
    // El término especular añade un reflejo pequeño cuando la cámara queda en el ángulo correcto.
    float diffuse = max(dot(N, L), 0.0);
    float specular = pow(max(dot(reflect(-L, N), V), 0.0), 28.0);
    // Mantengo un poco de luz en el lado oscuro para que aún se distinga.
    // Combino luz general, luz del Sol y un reflejo para que el planeta parezca redondo.
    vec3 lit = color * (0.13 + 0.92 * diffuse) + vec3(0.38) * specular * 0.28;
    fragColor = vec4(lit, 1.0);
}
