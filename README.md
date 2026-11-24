1 Tres Triángulos Cámara

Aplicación de gráficos 3D hecha en C++ y OpenGL. Presenta un sistema solar interactivo con planetas texturizados, controles con Dear ImGui, música propia, un astronauta y recorridos con curvas de Bézier.

2 Características

- Sol y ocho planetas con imágenes como texturas; órbitas y anillos de Saturno.
- Fondo espacial texturizado que rodea la escena.
- Cámara libre, enfoque y seguimiento de planetas.
- Panel ImGui para controlar la simulación, cambiar velocidad orbital y color de planetas, elegir música y mostrar u ocultar elementos.
- Astronauta 3D que se puede mostrar u ocultar, cambiar de tamaño y color, y mover por una trayectoria de Bézier.
- Vista desde el astronauta y recorrido panorámico de cámara por los planetas, con velocidades ajustables.
- Reproductor de música MP3.

3 Herramientas y bibliotecas

Bibliotecas
 - C++17 = Lógica de la aplicación y simulación. 
 - OpenGL = Dibuja las figuras 3D y la interfaz gráfica. 
 - GLFW = Crea la ventana y recibe teclado y ratón. 
 - GLAD = Carga las funciones de OpenGL. 
 - Dear ImGui = Botones, listas, casillas y controles deslizantes. 
 - Assimp = Importa el modelo 3D `.glb` del astronauta. 
 - STB Image = Lee las imágenes y permite cargarlas como texturas. 
 - GLSL = Lenguaje de los shaders `default.vert` y `default.frag`, que procesan vértices y colores de los píxeles. 
 - Windows =`winmm`  Reproducción de música mediante MCI. 

4 Estructura del proyecto

TresTriangulosCamara
├── TresTriangulosCamara.sln       Solución de Visual Studio
├── TresTriangulosCamara.vcxproj   Configuración de compilación
├── main.cpp                       Escena, controles, simulación e interfaz
├── glad.c                         Cargador de OpenGL
├── stb.cpp                        Implementación de STB Image
├── default.vert                   Shader de vértices
├── default.frag                   Shader de fragmentos
├── libraries                      Encabezados y bibliotecas de terceros
├── Texturas                       Imágenes para planetas y fondo
├── Modelo                         Modelo y texturas del astronauta
├── minisoundtrack                 Música MP3

5 Controles de cámara

- `W`, `A`, `S`, `D`: desplazar la cámara.
- Ratón: mirar alrededor cuando la cámara está activa.
- `Espacio`: subir.
- `Shift`: bajar.
- `Tab`: alternar entre manejar la cámara y usar el ratón sobre ImGui.
- `Esc`: cerrar la aplicación.
- Al enfocar un planeta, la cámara lo sigue. Usa `F` o **Dejar de seguir** para soltarlo.

6 Controles de ImGui

La ventana Controles del sistema solar contiene:

- Simulación: pausar/reanudar, reiniciar el tiempo y cambiar la velocidad global.
- Reproductor de música: escoger pista, anterior, siguiente, pausar/reanudar y cambiar volumen.
- Planetas: escoger un planeta, ajustar su velocidad orbital y cambiar su color.
- Astronauta: mostrar/ocultar, activar su vista, mostrar la curva, ajustar tamaño y velocidad, y empezar/detener su recorrido.
- Recorrido de cámara: empezar/detener el paseo por los planetas, mostrar su curva y ajustar velocidad.
- Vista: mostrar u ocultar órbitas, fondo estrellado y fichas informativas; restablecer la cámara.

Pulsa `Tab` para liberar el cursor cuando esté capturado por la cámara. **Enfocar planeta** apunta la cámara al planeta seleccionado.

7 Cómo se dibuja la escena

8 Malla y buffers

Una malla (`Mesh`) reúne los buffers que OpenGL utiliza para dibujar una figura. Los vértices guardan datos como posición y normal 

- VAO: recuerda cómo están organizados los datos de los vértices.
- VBO: almacena los datos de los vértices.
- EBO: almacena índices para reutilizar vértices al construir triángulos (si la figura lo necesita).

9 Texturas y shaders

Una textura es una imagen aplicada a la superficie de una malla. Las coordenadas UV indican qué parte de la imagen corresponde a cada punto de la superficie. STB lee los píxeles y OpenGL los guarda en una textura.

El shader de vértices (`default.vert`) transforma las posiciones 3D para verlas desde la cámara. El shader de fragmentos (`default.frag`) determina el color final de cada píxel, combinando textura, iluminación y color del objeto. Ambos archivos son necesarios para dibujar la escena.

10 Curvas de Bézier

La curva cúbica usa cuatro puntos: inicio, dos controles y final. Los controles dan forma a la ruta. El programa calcula posiciones progresivas para que el astronauta o la cámara recorran trayectorias suaves entre zonas del sistema solar. Los recorridos del astronauta y de cámara se manejan por separado.

11 Interfaz ImGui

Los botones y deslizadores cambian variables del programa. En cada cuadro, el ciclo principal lee esas variables, actualiza posiciones y dibuja la escena con los nuevos valores.

12 Recursos

Las rutas y los nombres escritos en el código deben coincidir con los archivos. Texturas utilizadas:

| Objeto | Archivo |
| --- | --- |
| Sol | `Texturas/sol.jpg` |
| Mercurio | `Texturas/mercurio (2).jpg` |
| Venus | `Texturas/venus (1).jpg` |
| Tierra | `Texturas/tierra.jpg` |
| Marte | `Texturas/marte.jpg` |
| Júpiter | `Texturas/jupiter.jpg` |
| Saturno | `Texturas/saturno.jpg` |
| Urano | `Texturas/urano.jpg` |
| Neptuno | `Texturas/neptuno.jpg` |
| Fondo espacial | `Texturas/espacioexteriorskycube.jpg` |

El modelo que carga el código es `Modelo/Astronaut.glb`. `AstronautAnimado.glb` está guardado en la carpeta, pero aún no se usa para reproducir animaciones de articulaciones. El movimiento actual del astronauta se produce recorriendo la curva y girando para seguir la dereccion de las curvas de bezier

La música predeterminada es `minisoundtrack/holaespacio.mp3`. Las demás canciones están en esa misma carpeta y se seleccionan desde ImGui. La reproducción depende de funciones de Windows, así que el proyecto está pensado para Windows.



Este repositorio puede incluir código, dependencias, modelos, imágenes y música. Antes de redistribuir esos recursos públicamente, comprueba sus licencias y permisos de uso. Los archivos generados por Visual Studio se excluyen mediante `.gitignore`.
