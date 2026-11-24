// Mi proyecto muestra un sistema solar 3D con texturas, controles y una interfaz.
// Evito que Windows cambie los nombres min y max que uso desde C++.
#define NOMINMAX
// Uso funciones de Windows para crear la ventana, dibujar texto y buscar archivos.
#include <windows.h>
// Esta biblioteca de Windows permite controlar la música.
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// GLAD prepara las funciones que uso para dibujar con OpenGL.
#include <glad/glad.h>
// GLFW crea la ventana y recibe el teclado y el mouse.
#include <GLFW/glfw3.h>
// STB Image abre las imágenes que uso como texturas.
#include <stb/stb_image.h>
// Dear ImGui crea botones, listas y controles sobre la escena.
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
// Assimp me ayuda a leer el modelo 3D del astronauta.
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector> // Lista dinámica que puede crecer según la cantidad de datos.
// Guía rápida de tipos que uso:
// int guarda números enteros; float guarda números con decimales; bool guarda sí/no.
// const significa que dejo un valor fijo. std::vector es una lista que puede crecer.
// Los nombres GL* pertenecen a OpenGL: GLuint identifica recursos, GLenum elige opciones.
// El símbolo * se usa para pasar una dirección; & permite usar un dato existente sin copiarlo.

namespace {
// PI sirve para convertir ángulos y para construir círculos y esferas.
const float PI = 3.14159f;
// Tamaño inicial de la ventana. Se puede cambiar sin alterar la geometría.
const int WINDOW_WIDTH = 2048;
const int WINDOW_HEIGHT = 1024;

// Guardo aquí tres números: X, Y y Z. Sirven para posición, dirección o color.
struct Vec3 {
    float x, y, z;
    // Estas funciones me dejan sumar, restar y multiplicar vectores.
    Vec3 operator+(Vec3 b) const { return { x + b.x, y + b.y, z + b.z }; }
    Vec3 operator-(Vec3 b) const { return { x - b.x, y - b.y, z - b.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};




// Un vértice es un punto de la figura: tiene posición, dirección de luz y lugar de la imagen que le toca.
struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};




// Mesh reúne lo que OpenGL necesita para dibujar una figura 3D.
// GLuint es un número que identifica un recurso; GLsizei guarda una cantidad; GLenum indica una opción de dibujo.
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei count = 0;
    GLenum primitive = GL_TRIANGLES;
};




// Cada parte del astronauta guarda su figura, su imagen y su color.
struct AstronautPart {
    Mesh mesh;
    GLuint texture = 0;
    Vec3 color{1.0f, 1.0f, 1.0f};
};




// Junto en esta estructura lo que necesito para mostrar y dibujar un planeta.
struct Planet {
    const wchar_t* name;      // Nombre que aparece en la interfaz.
    const wchar_t* diameter;  // Diámetro real mostrado como dato.
    const wchar_t* day;       // Duración aproximada de un día.
    const wchar_t* fact;      // Dato curioso del planeta.
    float orbit;              // Distancia visual al Sol dentro de la escena.
    float radius;             // Radio visual de la esfera.
    float speed;              // Velocidad visual de la órbita.
    int style;                // Estilo de superficie que usa el programa.
    Vec3 color;               // Color base de la superficie.
};




// Estas operaciones con vectores se usan en distintas partes de la escena.
// El producto punto compara direcciones; el producto cruz encuentra una dirección perpendicular.
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
// Dejo el vector con largo 1 y evito dividir por cero.
Vec3 normalize(Vec3 v) {
    const float length = std::sqrt(std::max(dot(v, v), 0.000001f));
    return v * (1.0f / length);
}
// Datos que aparecen en cada ficha. El orden es el mismo que se usa para recorrer planetas.
// Los tamaños y distancias están comprimidos para que todo quepa en la ventana.
Planet planets[] = {
    //Nombre ,    Diamteroinfo ,duradiainfo , datocurioso                          ,Orbita ,radio ,velocidad , estilo , color
    { L"Mercurio", L"4.879 km", L"58,6 días", L"Es el planeta más cercano al Sol.",  5.0f, 0.38f, 1.6f, 0, {0.62f,0.58f,0.52f} },
    { L"Venus",    L"12.104 km", L"243 días", L"Su atmósfera retiene mucho calor.",   7.3f, 0.62f, 1.18f, 1, {0.88f,0.64f,0.34f} },
    { L"Tierra",   L"12.742 km", L"23 h 56 min", L"Tiene agua líquida en su superficie.", 9.8f, 0.67f, 0.94f, 2, {0.18f,0.48f,0.88f} },
    { L"Marte",    L"6.779 km", L"24 h 37 min", L"Su color rojizo viene del óxido de hierro.", 12.4f, 0.50f, 0.78f, 3, {0.82f,0.30f,0.17f} },
    { L"Júpiter",  L"139.820 km", L"9 h 56 min", L"La Gran Mancha Roja es una tormenta gigante.", 17.0f, 1.55f, 0.43f, 4, {0.78f,0.58f,0.38f} },
    { L"Saturno",  L"116.460 km", L"10 h 42 min", L"Sus anillos están hechos de hielo y roca.", 22.2f, 1.30f, 0.32f, 5, {0.85f,0.73f,0.48f} },
    { L"Urano",    L"50.724 km", L"17 h 14 min", L"Gira inclinado, casi de costado.", 27.0f, 0.98f, 0.23f, 6, {0.43f,0.79f,0.84f} },
    { L"Neptuno",  L"49.244 km", L"16 h 6 min", L"Tiene vientos muy rápidos.", 31.5f, 0.95f, 0.18f, 7, {0.22f,0.38f,0.88f} }
};

// Compilo el shader para que la tarjeta gráfica pueda ejecutarlo.
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Error al compilar shader:\n" << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Averigua la carpeta donde está el .exe. Así encontramos los shaders aunque el programa
// se haya abierto desde otra carpeta o desde un acceso directo.
// Uso std::wstring para guardar rutas de Windows, incluso si tienen caracteres especiales.
std::wstring executableDirectory() {
    wchar_t path[MAX_PATH]{};
    const DWORD length=GetModuleFileNameW(nullptr,path,MAX_PATH);
    std::wstring directory(path,length);
    const size_t separator=directory.find_last_of(L"\\/");
    if (separator!=std::wstring::npos) directory.resize(separator);
    return directory;
}

// Lee todo un archivo de texto y lo devuelve como una cadena
std::string readTextFile(const std::wstring& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) return {};
    std::ostringstream contents; contents << file.rdbuf();
    return contents.str();
}

// Cargo los dos shaders, los preparo y los uno para que trabajen juntos.
GLuint createProgram() {
    const std::wstring shaderDirectory=executableDirectory()+L"\\";
    const std::string vertexSource = readTextFile(shaderDirectory+L"default.vert");
    const std::string fragmentSource = readTextFile(shaderDirectory+L"default.frag");
    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "No se encontraron default.vert y default.frag junto al ejecutable.\n";
        return 0;
    }
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());
    if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return 0; }
    GLuint program = glCreateProgram();
    // Los dos shaders se enlazan juntos: uno calcula posiciones y el otro calcula colores.
    glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok = GL_FALSE; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]{}; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Error al enlazar shaders:\n" << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// Uso la herramienta de audio de Windows para reproducir los archivos MP3.
// Esta lista relaciona cada opción del reproductor con el nombre real del MP3 en minisoundtrack.
// musicLabels, justo debajo, contiene los nombres fáciles de leer que muestro en ImGui.
const wchar_t* musicFiles[] = {
    L"holaespacio.mp3", L"BeepBox-Song.mp3", L"BeepBox-Song (2).mp3",
    L"BeepBox-Song (4).mp3", L"BeepBox-Song (5).mp3", L"BeepBox-Song (8).mp3",
    L"BeepBox-Song (10).mp3", L"BeepBox-Song (15).mp3", L"BeepBox-Song (16).mp3",
    L"BeepBox-Song (18).mp3", L"BeepBox-Song (20).mp3", L"BeepBox-Song (23).mp3",
    L"BeepBox-Song (24).mp3"
};
const char* musicLabels[] = {
    "Hola Espacio", "BeepBox Song", "BeepBox Song 2", "BeepBox Song 4",
    "BeepBox Song 5", "BeepBox Song 8", "BeepBox Song 10", "BeepBox Song 15",
    "BeepBox Song 16", "BeepBox Song 18", "BeepBox Song 20", "BeepBox Song 23",
    "BeepBox Song 24"
};
//reproductor de musica sacado de youtube
bool startMusicTrack(int index, int volume) {
    mciSendStringW(L"close solarMusic", nullptr, 0, nullptr);
    std::vector<std::wstring> directories;
    wchar_t workingPath[MAX_PATH]{};
    const DWORD workingLength = GetCurrentDirectoryW(MAX_PATH, workingPath);
    if (workingLength > 0 && workingLength < MAX_PATH)
        directories.push_back(std::wstring(workingPath) + L"\\minisoundtrack\\");
    std::wstring executablePath = executableDirectory();
    for (int level = 0; level < 3 && !executablePath.empty(); ++level) {
        directories.push_back(executablePath + L"\\minisoundtrack\\");
        const size_t separator = executablePath.find_last_of(L"\\/");
        if (separator == std::wstring::npos) break;
        executablePath.resize(separator);
    }
    std::wstring path;
    for (const std::wstring& directory : directories) {
        const std::wstring candidate = directory + musicFiles[index];
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            path = candidate;
            break;
        }
    }
    if (path.empty()) {
        std::wcerr << L"No encuentro el archivo de musica: " << musicFiles[index] << L"\n";
        return false;
    }
    const std::wstring openCommand = L"open \"" + path + L"\" type mpegvideo alias solarMusic";
    MCIERROR error = mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr);
    if (error != 0) {
        wchar_t message[256]{};
        mciGetErrorStringW(error, message, 256);
        std::wcerr << L"Error abriendo la musica: " << message << L"\n";
        return false;
    }
    const std::wstring volumeCommand = L"setaudio solarMusic volume to " + std::to_wstring(volume);
    mciSendStringW(volumeCommand.c_str(), nullptr, 0, nullptr);
    error = mciSendStringW(L"play solarMusic repeat", nullptr, 0, nullptr);
    if (error != 0) {
        wchar_t message[256]{};
        mciGetErrorStringW(error, message, 256);
        std::wcerr << L"Error reproduciendo la musica: " << message << L"\n";
        mciSendStringW(L"close solarMusic", nullptr, 0, nullptr);
        return false;
    }
    return true;
}


//es el tamaño del vértice completo; offset marca dónde empieza cada dato.
void setVertexLayout() {
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
}

// Preparo la figura para dibujarla con OpenGL.
// makeSphere crea los puntos y aquí los copio a la tarjeta gráfica.


// El VBO guarda los puntos; el VAO recuerda cómo leerlos. Mesh guarda sus identificadores.
Mesh makeMesh(const std::vector<Vertex>& vertices, GLenum primitive = GL_TRIANGLES, GLenum usage = GL_STATIC_DRAW) {
    Mesh mesh; mesh.count = static_cast<GLsizei>(vertices.size()); mesh.primitive = primitive;
    glGenVertexArrays(1, &mesh.vao); glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao); glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage);
    setVertexLayout(); glBindVertexArray(0);
    return mesh;
}

// Libero la memoria de la figura cuando ya no la necesito.


void destroyMesh(Mesh& mesh) {
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    mesh = {};
}

// Esta función calcula un punto de la esfera usando dos ángulos.
// Como esta esfera tiene radio 1, la posición también indica hacia dónde mira su superficie.
Vertex spherePoint(float latitude, float longitude, float u, float v) {
    const float x = std::cos(latitude) * std::cos(longitude);
    const float y = std::sin(latitude);
    const float z = std::cos(latitude) * std::sin(longitude);
    return Vertex{ x, y, z, x, y, z, u, v };
}
// Construyo una esfera unitaria dividiéndola en franjas. La misma forma sirve para el Sol, planetas y fondo.
// stacks la divide de arriba abajo y slices alrededor; más divisiones producen más detalle y usan más vértices.
// Cada cuadrilátero de la esfera se representa con dos triángulos.
Mesh makeSphere(int stacks = 32, int slices = 48) {
    std::vector<Vertex> vertices;
    for (int i = 0; i < stacks; ++i) {
        const float v0 = static_cast<float>(i) / stacks, v1 = static_cast<float>(i + 1) / stacks;
        const float lat0 = -PI * 0.5f + v0 * PI, lat1 = -PI * 0.5f + v1 * PI;
        for (int j = 0; j < slices; ++j) {
            const float u0 = static_cast<float>(j) / slices, u1 = static_cast<float>(j + 1) / slices;
            const float lon0 = u0 * 2.0f * PI, lon1 = u1 * 2.0f * PI;
            Vertex a = spherePoint(lat0, lon0, u0, v0), b = spherePoint(lat0, lon1, u1, v0);
            Vertex c = spherePoint(lat1, lon1, u1, v1), d = spherePoint(lat1, lon0, u0, v1);
            // Un cuadradito de la esfera se divide en dos triángulos, porque OpenGL dibuja triángulos.
            vertices.insert(vertices.end(), { a,b,c,a,c,d });
        }
    }
    return makeMesh(vertices);
}

// Dibujo la órbita conectando varios puntos en un círculo.
// El último punto se conecta con el primero para cerrar el círculo.
// radius indica el tamaño de la órbita alrededor del Sol.
Mesh makeOrbit(float radius) {
    std::vector<Vertex> vertices;
    const int segments = 180;
    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * PI * static_cast<float>(i) / segments;
        vertices.push_back({ radius * std::cos(angle), 0.0f, radius * std::sin(angle), 0,1,0,0,0 });
    }
    return makeMesh(vertices, GL_LINE_LOOP);
}

// Construyo los anillos como una banda entre un borde interior y uno exterior.
// Divido la banda en triángulos.

Mesh makeSaturnRings() {
    std::vector<Vertex> vertices;
    const int segments = 144;
    for (int i = 0; i < segments; ++i) {
        const float a0 = 2.0f * PI * static_cast<float>(i) / segments;
        const float a1 = 2.0f * PI * static_cast<float>(i + 1) / segments;
        const float inner = 1.45f, outer = 2.35f;
        Vertex p0{ inner * std::cos(a0),0,inner * std::sin(a0),0,1,0,0,0 };
        Vertex p1{ outer * std::cos(a0),0,outer * std::sin(a0),0,1,0,1,0 };
        Vertex p2{ outer * std::cos(a1),0,outer * std::sin(a1),0,1,0,1,1 };
        Vertex p3{ inner * std::cos(a1),0,inner * std::sin(a1),0,1,0,0,1 };
        vertices.insert(vertices.end(), { p0,p1,p2,p0,p2,p3 });
    }
    return makeMesh(vertices);
}


void identity(float* m) { for (int i=0;i<16;++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f; }



// La matriz model coloca el objeto en la escena, le da tamaño y lo gira.

void modelMatrix(float* m, Vec3 position, float scale, float rotationY = 0.0f, float rotationX = 0.0f) {
    const float cy = std::cos(rotationY), sy = std::sin(rotationY), cx = std::cos(rotationX), sx = std::sin(rotationX);
    m[0]=cy*scale; m[1]=sx*sy*scale; m[2]=-cx*sy*scale; m[3]=0;
    m[4]=0; m[5]=cx*scale; m[6]=sx*scale; m[7]=0;
    m[8]=sy*scale; m[9]=-sx*cy*scale; m[10]=cx*cy*scale; m[11]=0;
    m[12]=position.x; m[13]=position.y; m[14]=position.z; m[15]=1;
}

// La perspectiva hace que lo lejano se vea más pequeño.
// fov controla cuánto ve la cámara; aspect es ancho dividido por alto; near y far limitan la distancia visible.


void perspective(float* m, float fov, float aspect, float nearPlane, float farPlane) {
    for (int i=0;i<16;++i) m[i]=0.0f;
    const float t = std::tan(fov * 0.5f);
    m[0]=1.0f/(aspect*t); m[5]=1.0f/t;
    m[10]=-(farPlane+nearPlane)/(farPlane-nearPlane); m[11]=-1.0f;
    m[14]=-(2.0f*farPlane*nearPlane)/(farPlane-nearPlane);
}

// Esta función orienta la cámara desde eye hacia el punto center.
// up marca qué dirección de la escena cuenta como arriba.


void lookAt(float* m, Vec3 eye, Vec3 center, Vec3 up) {
    const Vec3 f = normalize(center-eye), s = normalize(cross(f,up)), u = cross(s,f);
    m[0]=s.x; m[4]=s.y; m[8]=s.z; m[12]=-dot(s,eye);
    m[1]=u.x; m[5]=u.y; m[9]=u.z; m[13]=-dot(u,eye);
    m[2]=-f.x; m[6]=-f.y; m[10]=-f.z; m[14]=dot(f,eye);
    m[3]=0; m[7]=0; m[11]=0; m[15]=1;
}

// Creo una imagen con los datos del planeta y la convierto en una textura.
GLuint makeInfoTexture(const Planet& planet) {
    const int width = 640, height = 190;
    // Creamos un lienzo de 640x190 píxeles. La altura negativa indica que empieza arriba,
    // así el texto no queda patas arriba al cargarlo como textura.
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=width;
    info.bmiHeader.biHeight=-height; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr; HBITMAP bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if (!bitmap || !pixels) return 0;
    HDC dc=CreateCompatibleDC(nullptr); HGDIOBJ oldBitmap=SelectObject(dc,bitmap);
    // Primero pintamos el fondo oscuro y el borde azul del panel.
    RECT panel{0,0,width,height}; HBRUSH bg=CreateSolidBrush(RGB(11,22,40)); FillRect(dc,&panel,bg); DeleteObject(bg);
    HPEN border=CreatePen(PS_SOLID,3,RGB(61,159,229)); HGDIOBJ oldPen=SelectObject(dc,border);
    HGDIOBJ oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); RoundRect(dc,2,2,width-2,height-2,18,18);
    SelectObject(dc,oldBrush); SelectObject(dc,oldPen); DeleteObject(border);
    SetBkMode(dc,TRANSPARENT);
    HFONT titleFont=CreateFontW(42,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HFONT bodyFont=CreateFontW(30,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HGDIOBJ previousFont=SelectObject(dc,titleFont); SetTextColor(dc,RGB(112,210,255));
    // Escribimos el nombre grande y debajo los datos que corresponden a este planeta.
    TextOutW(dc,22,15,planet.name,static_cast<int>(wcslen(planet.name)));
    SelectObject(dc,bodyFont); SetTextColor(dc,RGB(232,239,248));
    const std::wstring line1=L"Diámetro: "+std::wstring(planet.diameter);
    const std::wstring line2=L"Duración del día: "+std::wstring(planet.day);
    TextOutW(dc,24,67,line1.c_str(),static_cast<int>(line1.size()));
    TextOutW(dc,24,101,line2.c_str(),static_cast<int>(line2.size()));
    TextOutW(dc,24,145,planet.fact,static_cast<int>(wcslen(planet.fact)));
    SelectObject(dc,previousFont); DeleteObject(titleFont); DeleteObject(bodyFont);

    // Windows dibuja el texto en la imagen. Aquí hago que todos sus puntos sean opacos.
    // como opacos para que OpenGL no descarte el texto y el fondo del panel.
    auto* argb = static_cast<unsigned int*>(pixels);
    for (int i=0;i<width*height;++i) argb[i] |= 0xFF000000u;

    // Subimos la imagen a OpenGL. GL_BGRA coincide con el orden de colores que usa Windows.
    GLuint texture=0; glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    SelectObject(dc,oldBitmap); DeleteObject(bitmap); DeleteDC(dc);
    return texture;
}

// Cargo una imagen rectangular que se envuelve sobre la esfera del planeta.
// Cargo una imagen del disco con STB  y la convierto en una textura 2D de OpenGL.
GLuint loadPlanetTexture(const std::wstring& path) {
    const int pathLength = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> utf8PathBuffer(static_cast<size_t>(pathLength));
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8PathBuffer.data(), pathLength, nullptr, nullptr);
    const std::string utf8Path(utf8PathBuffer.data());
    stbi_set_flip_vertically_on_load(1);
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(utf8Path.c_str(), &width, &height, &channels, STBI_rgb);
    if (!pixels) {
        std::cerr << "No se pudo cargar la textura " << utf8Path << ": " << stbi_failure_reason() << '\n';
        return 0;
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // STB entrega filas RGB compactas. OpenGL usa 4 bytes de alineación por defecto,
    // lo que puede leer fuera de la imagen si su ancho no genera filas múltiplos de 4.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    return texture;
}

// Busco el color y la textura que Assimp encontró dentro del material del modelo GLB.
GLuint loadAstronautMaterialTexture(const aiScene* scene, const aiMaterial* material, Vec3& color) {
    aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
    if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &diffuse) != AI_SUCCESS)
        aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
    color = { diffuse.r, diffuse.g, diffuse.b };

    aiString texturePath;
    aiReturn result = material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath);
    if (result != AI_SUCCESS) result = material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
    if (result != AI_SUCCESS) return 0;
    const aiTexture* embedded = scene->GetEmbeddedTexture(texturePath.C_Str());
    if (!embedded) return 0;

    int width = 0, height = 0, channels = 0;
    unsigned char* rgba = nullptr;
    if (embedded->mHeight == 0) {
        rgba = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(embedded->pcData),
            static_cast<int>(embedded->mWidth), &width, &height, &channels, STBI_rgb_alpha);
    } else {
        width = static_cast<int>(embedded->mWidth);
        height = static_cast<int>(embedded->mHeight);
        rgba = new unsigned char[static_cast<size_t>(width) * height * 4];
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            rgba[i * 4 + 0] = embedded->pcData[i].r;
            rgba[i * 4 + 1] = embedded->pcData[i].g;
            rgba[i * 4 + 2] = embedded->pcData[i].b;
            rgba[i * 4 + 3] = embedded->pcData[i].a;
        }
    }
    if (!rgba) return 0;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (embedded->mHeight == 0) stbi_image_free(rgba);
    else delete[] rgba;
    return texture;
}

// Busco un recurso tanto junto al ejecutable como en las carpetas del proyecto.
// Así el modelo y otros archivos se encuentran aunque Visual Studio inicie el programa desde otra carpeta.
std::wstring findProjectAsset(const wchar_t* folder, const wchar_t* filename) {
    std::vector<std::wstring> roots;
    wchar_t workingPath[MAX_PATH]{};
    const DWORD workingLength = GetCurrentDirectoryW(MAX_PATH, workingPath);
    if (workingLength > 0 && workingLength < MAX_PATH) roots.emplace_back(workingPath);
    std::wstring root = executableDirectory();
    for (int level = 0; level < 3 && !root.empty(); ++level) {
        roots.push_back(root);
        const size_t separator = root.find_last_of(L"\\/");
        if (separator == std::wstring::npos) break;
        root.resize(separator);
    }
    for (const std::wstring& candidateRoot : roots) {
        const std::wstring candidate = candidateRoot + L"\\" + folder + L"\\" + filename;
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) return candidate;
    }
    return {};
}

// Assimp lee el archivo GLB y me entrega sus figuras, colores e información de texturas.
// Centro y escalo el modelo para que tenga un tamaño conocido dentro de mi escena.
// Cargo la forma del astronauta, pero este código no reproduce animaciones de sus articulaciones.
std::vector<AstronautPart> loadAstronautModel(const std::wstring& path) {
    if (path.empty()) {
        std::cerr << "No se encontró Modelo\\Astronaut.glb.\n";
        return {};
    }
    const int pathLength = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> utf8Path(static_cast<size_t>(pathLength));
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8Path.data(), pathLength, nullptr, nullptr);
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(utf8Path.data(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices | aiProcess_ValidateDataStructure);
    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        std::cerr << "No se pudo cargar el astronauta: " << importer.GetErrorString() << '\n';
        return {};
    }

    aiVector3D minimum(FLT_MAX, FLT_MAX, FLT_MAX), maximum(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& p = mesh->mVertices[v];
            minimum.x = std::min(minimum.x, p.x); minimum.y = std::min(minimum.y, p.y); minimum.z = std::min(minimum.z, p.z);
            maximum.x = std::max(maximum.x, p.x); maximum.y = std::max(maximum.y, p.y); maximum.z = std::max(maximum.z, p.z);
        }
    }
    const float height = std::max(maximum.y - minimum.y, 0.0001f);
    const float scale = 2.4f / height;
    const float centerX = (minimum.x + maximum.x) * 0.5f;
    const float centerZ = (minimum.z + maximum.z) * 0.5f;

    std::vector<AstronautPart> parts;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* source = scene->mMeshes[m];
        std::vector<Vertex> vertices;
        vertices.reserve(static_cast<size_t>(source->mNumFaces) * 3);
        for (unsigned int f = 0; f < source->mNumFaces; ++f) {
            const aiFace& face = source->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                const unsigned int index = face.mIndices[j];
                const aiVector3D& p = source->mVertices[index];
                const aiVector3D n = source->HasNormals() ? source->mNormals[index] : aiVector3D(0, 1, 0);
                const aiVector3D uv = source->HasTextureCoords(0) ? source->mTextureCoords[0][index] : aiVector3D(0, 0, 0);
                vertices.push_back({ (p.x - centerX) * scale, (p.y - minimum.y) * scale,
                    (p.z - centerZ) * scale, n.x, n.y, n.z, uv.x, 1.0f - uv.y });
            }
        }
        if (vertices.empty()) continue;
        AstronautPart part;
        part.mesh = makeMesh(vertices);
        if (source->mMaterialIndex < scene->mNumMaterials) {
            part.texture = loadAstronautMaterialTexture(scene, scene->mMaterials[source->mMaterialIndex], part.color);
            // Este GLB incluye un mapa WebP y un PNG de respaldo. Cargamos los PNG
            // directamente cuando STB no puede leer el WebP seleccionado por Assimp.
            if (!part.texture) {
                const wchar_t* fallbackTextures[] = { L"Astronaut_BaseColor.png", L"Astronaut_Suit.png" };
                const unsigned int fallbackIndex = std::min(source->mMaterialIndex, 1u);
                part.texture = loadPlanetTexture(findProjectAsset(L"Modelo", fallbackTextures[fallbackIndex]));
            }
        }
        parts.push_back(part);
    }
    std::cout << "Astronauta cargado: " << parts.size() << " partes.\n";
    return parts;
}

// Dos triángulos forman el rectángulo de la ficha. 
Mesh makePanelQuad() {
    const Vertex a{-1,-0.5f,0,0,0,1,0,1}, b{1,-0.5f,0,0,0,1,1,1};
    const Vertex c{1,0.5f,0,0,0,1,1,0}, d{-1,0.5f,0,0,0,1,0,0};
    return makeMesh({a,b,c,a,c,d});
}

// La posición y orientación de la cámara se guardan aquí para que el mouse y teclado las cambien.
void mouseCallback(GLFWwindow*, double x, double y);
float cameraX=0.0f, cameraY=24.0f, cameraZ=59.0f, yaw=-90.0f, pitch=-19.0f;
float lastMouseX=0.0f,lastMouseY=0.0f; bool firstMouse=true;
bool cameraControl=true, followPlanet=false;
int followedPlanet=0;
Vec3 followOffset{};
bool astronautRouteActive=false, showAstronaut=true, showAstronautPath=true;
bool astronautCameraView=false;
bool cameraTourActive=false, showCameraTourPath=true;
int cameraTourPlanet=2, cameraTourTargetPlanet=3;
Vec3 cameraTourStart{}, cameraTourControl1{}, cameraTourControl2{}, cameraTourEnd{};
float cameraTourProgress=0.0f, cameraTourDuration=8.0f, cameraTourSpeed=1.0f;
// Índice del planeta actual en el recorrido circular de ocho planetas.
int astronautWaypoint=2;
Vec3 astronautPosition{0.0f, 1.0f, 9.8f};
Vec3 astronautRouteStart{}, astronautRouteControl1{}, astronautRouteControl2{}, astronautRouteEnd{};
float astronautRouteProgress=0.0f, astronautRouteDuration=8.0f, astronautRouteSpeed=1.0f, astronautScale=0.75f;
Vec3 astronautTint{0.68f,0.88f,1.0f};
// Cada movimiento del mouse cambia hacia dónde mira la cámara.
void mouseCallback(GLFWwindow*, double x, double y) {
    if (!cameraControl || followPlanet || astronautCameraView || cameraTourActive) return;
    // El primer movimiento solo fija la posición inicial del mouse; no debe girar la cámara.
    if (firstMouse) { lastMouseX=static_cast<float>(x); lastMouseY=static_cast<float>(y); firstMouse=false; }
    // yaw gira a los lados; pitch mira arriba y abajo. El número 0.10 ajusta la sensibilidad.
    yaw += (static_cast<float>(x)-lastMouseX)*0.10f; pitch += (lastMouseY-static_cast<float>(y))*0.10f;
    lastMouseX=static_cast<float>(x); lastMouseY=static_cast<float>(y);
    pitch=std::max(-85.0f,std::min(pitch,85.0f));
}

// Leo el teclado para cerrar o mover la cámara en primera persona.
// Uso el tiempo entre cuadros para que la cámara avance parecido en computadoras rápidas y lentas.
void processInput(GLFWwindow* window,float dt) {
    // Esc cierra la ventana. dt es cuánto duró el cuadro; usarlo hace que la velocidad
    // sea parecida en computadoras rápidas y lentas.
    if (glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(window,GL_TRUE);
    if (!cameraControl || followPlanet || astronautCameraView || cameraTourActive) return;
    const float yr=yaw*PI/180.0f, pr=pitch*PI/180.0f;
    // forward apunta hacia donde mira la cámara. right es la dirección lateral para A y D.
    const Vec3 forward=normalize({std::cos(yr)*std::cos(pr),std::sin(pr),std::sin(yr)*std::cos(pr)});
    const Vec3 right=normalize(cross(forward,{0,1,0}));
    const float speed=18.0f*dt;
    if (glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS) { cameraX+=forward.x*speed; cameraY+=forward.y*speed; cameraZ+=forward.z*speed; }
    if (glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS) { cameraX-=forward.x*speed; cameraY-=forward.y*speed; cameraZ-=forward.z*speed; }
    if (glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS) { cameraX-=right.x*speed; cameraZ-=right.z*speed; }
    if (glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS) { cameraX+=right.x*speed; cameraZ+=right.z*speed; }
    if (glfwGetKey(window,GLFW_KEY_SPACE)==GLFW_PRESS) cameraY+=speed;
    if (glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS) cameraY-=speed;
}

// Dibuja una malla con una posición, color y modo de shader concretos.
// mode le dice al shader si es una superficie con luz, el Sol, un planeta o una ficha.
// Envío a los shaders los datos del objeto y le pido a OpenGL que dibuje su malla.
// mode elige cómo dibujar: 0 con luz, 1 brillante, 2 planeta, 3 ficha, 4 imagen sin luz.
// style elige el aspecto del planeta; texture es la imagen de su superficie si está cargada.
void drawMesh(GLuint program,const Mesh& mesh,const float* model,Vec3 color,int mode,int style=0,GLuint texture=0) {
    glUniformMatrix4fv(glGetUniformLocation(program,"model"),1,GL_FALSE,model);
    glUniform3f(glGetUniformLocation(program,"baseColor"),color.x,color.y,color.z);
    glUniform1i(glGetUniformLocation(program,"renderMode"),mode);
    glUniform1i(glGetUniformLocation(program,"planetStyle"),style);
    glUniform1i(glGetUniformLocation(program,"usePlanetTexture"),texture ? 1 : 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,texture);
    glBindVertexArray(mesh.vao); glDrawArrays(mesh.primitive,0,mesh.count);
}

// Calculo la posición del planeta en su órbita con seno y coseno:
// así se mueve en círculo. index separa a los planetas al inicio.

Vec3 planetPosition(const Planet& p,float time,int index) {
    
    const float angle=time*p.speed+index*(2.0f*PI/8.0f);
    return { p.orbit*std::cos(angle),0.0f,p.orbit*std::sin(angle) };
}

Vec3 planetCameraPosition(int index,float time) {
    const Planet& p=planets[index];
    return planetPosition(p,time,index)+Vec3{0.0f,p.radius+1.5f,p.radius+3.0f};
}

// Calculo un punto de la curva Bézier. 
Vec3 cubicBezier(Vec3 p0,Vec3 p1,Vec3 p2,Vec3 p3,float t) {
    const float u=1.0f-t;
    return p0*(u*u*u)+p1*(3.0f*u*u*t)+p2*(3.0f*u*t*t)+p3*(t*t*t);
}

//los puntos de control de bezier hacen que la curva se incline mas y asi poder ver de una mejor manera los planetas
void bezierControlPoints(Vec3 start,Vec3 end,Vec3& control1,Vec3& control2) {
    const Vec3 delta=end-start;
    const float distance=std::sqrt(std::max(dot(delta,delta),0.0001f));
    const Vec3 direction=normalize(delta);
    Vec3 side=normalize(cross(direction,{0.0f,1.0f,0.0f}));
    if (dot(side,side)<0.01f) side={1.0f,0.0f,0.0f};
    // Mantiene el trayecto casi recto y solo le da una curva suave de vuelo.
    const float height=std::max(0.12f,std::min(distance*0.012f,0.55f));
    const float sway=std::min(distance*0.012f,0.45f);
    control1=start+delta*0.33f+Vec3{0.0f,height,0.0f}+side*sway;
    control2=start+delta*0.67f+Vec3{0.0f,height,0.0f}-side*sway;
}

// Calculo hacia dónde apunta la curva y giro el astronauta para que mire hacia allí.
Vec3 bezierTangent(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t) {
    const float u = 1.0f - t;
    return normalize((p1 - p0) * (3.0f * u * u) + (p2 - p1) * (6.0f * u * t) + (p3 - p2) * (3.0f * t * t));
}

// Muestreo la curva en muchos puntos y conecto esos vértices como una línea para mostrar la ruta en pantalla.
Mesh makeBezierPath(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3) {
    std::vector<Vertex> vertices;
    const int samples = 96;
    vertices.reserve(samples + 1);
    for (int i = 0; i <= samples; ++i) {
        const Vec3 p = cubicBezier(p0, p1, p2, p3, static_cast<float>(i) / samples);
        vertices.push_back({ p.x, p.y, p.z, 0, 1, 0, 0, 0 });
    }
    return makeMesh(vertices, GL_LINE_STRIP);
}

// Preparo un tramo nuevo hacia el planeta indicado y calculo cuánto dura el vuelo.
void beginAstronautSegment(int toPlanet, float time) {
    const Planet& to = planets[toPlanet];
    astronautRouteStart = astronautPosition;
    astronautRouteEnd = planetPosition(to, time, toPlanet) + Vec3{0.0f, to.radius + 0.12f, 0.0f};
    bezierControlPoints(astronautRouteStart, astronautRouteEnd, astronautRouteControl1, astronautRouteControl2);
    // Uso la distancia directa entre el inicio y el final para calcular la duración del tramo.
    const Vec3 difference = astronautRouteEnd - astronautRouteStart;
    const float distance = std::sqrt(std::max(dot(difference, difference), 0.0001f));
    float duration = distance / 5.0f;
    if (duration < 4.0f) duration = 4.0f;
    if (duration > 16.0f) duration = 16.0f;
    astronautRouteDuration = duration;
    astronautRouteProgress = 0.0f;
}

// Inicio el recorrido en el planeta elegido y preparo la curva hacia el siguiente.
void startAstronautRoute(int startingPlanet, float time, Mesh& path) {
    astronautWaypoint = startingPlanet;
    const Planet& planet = planets[astronautWaypoint];
    astronautPosition = planetPosition(planet, time, astronautWaypoint)
        + Vec3{0.0f, planet.radius + 0.12f, 0.0f};
    beginAstronautSegment((astronautWaypoint + 1) % 8, time);
    astronautRouteActive = true;

    if (path.vao) destroyMesh(path);
    path = makeBezierPath(astronautRouteStart, astronautRouteControl1,
        astronautRouteControl2, astronautRouteEnd);
}

// Al llegar al planeta siguiente, preparo la curva hacia el próximo de la lista.
void advanceAstronautRoute(float time, Mesh& path) {
    astronautPosition = astronautRouteEnd;
    astronautWaypoint = (astronautWaypoint + 1) % 8;
    beginAstronautSegment((astronautWaypoint + 1) % 8, time);

    if (path.vao) destroyMesh(path);
    path = makeBezierPath(astronautRouteStart, astronautRouteControl1,
        astronautRouteControl2, astronautRouteEnd);
}
// Elijo una posición de cámara un poco elevada y alejada de cada planeta.
Vec3 cameraTourViewpoint(int planetIndex, float time) {
    const Planet& planet = planets[planetIndex];
    const Vec3 center = planetPosition(planet, time, planetIndex);
    return center + Vec3{0.0f, planet.radius + 2.0f, planet.radius + 5.0f};
}

// Preparo una curva amplia para que la cámara se mueva con suavidad entre dos planetas.
void beginCameraTourSegment(int targetPlanet, float time, Mesh& path) {
    cameraTourStart = {cameraX, cameraY, cameraZ};
    cameraTourTargetPlanet = targetPlanet;
    cameraTourEnd = cameraTourViewpoint(targetPlanet, time);

    const Vec3 delta = cameraTourEnd - cameraTourStart;
    const float distance = std::sqrt(std::max(dot(delta, delta), 0.0001f));
    const Vec3 direction = normalize(delta);
    Vec3 side = normalize(cross(direction, {0.0f, 1.0f, 0.0f}));
    if (dot(side, side) < 0.01f) side = {1.0f, 0.0f, 0.0f};

    float arcHeight = distance * 0.18f;
    if (arcHeight < 2.5f) arcHeight = 2.5f;
    if (arcHeight > 8.0f) arcHeight = 8.0f;
    const float sideAmount = std::min(distance * 0.06f, 2.5f);

    cameraTourControl1 = cameraTourStart + delta * 0.25f
        + Vec3{0.0f, arcHeight, 0.0f} + side * sideAmount;
    cameraTourControl2 = cameraTourStart + delta * 0.75f
        + Vec3{0.0f, arcHeight, 0.0f} - side * sideAmount;

    float duration = distance / 7.0f;
    if (duration < 5.0f) duration = 5.0f;
    if (duration > 18.0f) duration = 18.0f;
    cameraTourDuration = duration;
    cameraTourProgress = 0.0f;

    if (path.vao) destroyMesh(path);
    path = makeBezierPath(cameraTourStart, cameraTourControl1,
        cameraTourControl2, cameraTourEnd);
}

// Al llegar a un planeta, comienzo el tramo de cámara hacia el siguiente.
void advanceCameraTour(float time, Mesh& path) {
    cameraTourPlanet = cameraTourTargetPlanet;
    cameraX = cameraTourEnd.x;
    cameraY = cameraTourEnd.y;
    cameraZ = cameraTourEnd.z;
    beginCameraTourSegment((cameraTourPlanet + 1) % 8, time, path);
}

// Inicio el recorrido panorámico desde el planeta que está seleccionado en ImGui.
void startCameraTour(int startingPlanet, float time, Mesh& path) {
    cameraTourActive = true;
    cameraTourPlanet = startingPlanet;
    // Primero vuelo desde donde esté la cámara hasta el planeta elegido.
    // Al llegar, el recorrido continúa con los demás planetas.
    beginCameraTourSegment(startingPlanet, time, path);
}
// Apunto la cámara hacia el planeta seleccionado.
void orientCameraToward(Vec3 target) {
    const Vec3 direction=normalize(target-Vec3{cameraX,cameraY,cameraZ});
    yaw=std::atan2(direction.z,direction.x)*180.0f/PI;
    pitch=std::asin(direction.y)*180.0f/PI;
}

} // namespace

// main es el punto de entrada del programa: inicializo ventana, OpenGL, recursos y la escena.
// Después ejecuto el ciclo de actualización/dibujo hasta que el usuario cierre la ventana.
int main() {
    // 1) Iniciamos GLFW y pedimos una ventana con OpenGL 3.3.
    if (!glfwInit()) { std::cerr<<"No se pudo iniciar GLFW.\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window=glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"Sistema Solar 3D | WASD mueve la cámara",nullptr,nullptr);
    if (!window) { std::cerr<<"No se pudo crear la ventana.\n"; glfwTerminate(); return -1; }
    // OpenGL dibuja en esta ventana. swapInterval(1) limita la actualización a la pantalla.
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) { std::cerr<<"No se pudo iniciar GLAD.\n"; glfwDestroyWindow(window); glfwTerminate(); return -1; }

    // Activamos los controles y las opciones básicas de dibujo 3D.
    // GL_DEPTH_TEST permite que un planeta cercano tape a otro que está detrás.
    glfwSetCursorPosCallback(window,mouseCallback); glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    // ImGui crea la interfaz de controles encima de la escena 3D.
    // ImGui usa GLFW para leer entradas y OpenGL para dibujarse encima de la escena.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle& uiStyle=ImGui::GetStyle();
    uiStyle.WindowRounding=10.0f; uiStyle.FrameRounding=6.0f; uiStyle.GrabRounding=6.0f;
    uiStyle.Colors[ImGuiCol_WindowBg]=ImVec4(0.025f,0.045f,0.085f,0.96f);
    uiStyle.Colors[ImGuiCol_Header]=ImVec4(0.10f,0.30f,0.52f,1.0f);
    uiStyle.Colors[ImGuiCol_Button]=ImVec4(0.08f,0.28f,0.48f,1.0f);
    uiStyle.Colors[ImGuiCol_ButtonHovered]=ImVec4(0.12f,0.44f,0.70f,1.0f);
    uiStyle.Colors[ImGuiCol_FrameBg]=ImVec4(0.07f,0.12f,0.20f,1.0f);
    if (!ImGui_ImplGlfw_InitForOpenGL(window,true)) {
        std::cerr << "No se pudo iniciar el backend GLFW de ImGui.\n";
        ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        std::cerr << "No se pudo iniciar el backend OpenGL de ImGui.\n";
        ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    // OpenGL oculta lo que queda detrás de un objeto cercano y puede mezclar partes transparentes.
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // Activo los shaders y busco los datos que C++ les enviará.
    // Un uniform es un dato que C++ manda al shader, por ejemplo el color, la luz o la cámara.
    
    GLuint program=createProgram();
    if (!program) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
    glUseProgram(program);
    const GLint viewLoc=glGetUniformLocation(program,"view"), projectionLoc=glGetUniformLocation(program,"projection");
    const GLint lightLoc=glGetUniformLocation(program,"lightPosition"), cameraLoc=glGetUniformLocation(program,"cameraPosition");
    // Asigno las unidades de textura: 0 para fichas y 1 para planetas, Sol y fondo.
    glUniform1i(glGetUniformLocation(program,"panelTexture"),0);
    glUniform1i(glGetUniformLocation(program,"planetTexture"),1);

    
    
    // Preparo las figuras al inicio y las reutilizo en cada cuadro.
    
    Mesh sphere=makeSphere(); Mesh rings=makeSaturnRings(); Mesh panel=makePanelQuad();
    std::vector<Mesh> orbits; for (const Planet& p:planets) orbits.push_back(makeOrbit(p.orbit));
    // Creo las fichas con información de cada planeta.
    std::vector<GLuint> panels; for (const Planet& p:planets) panels.push_back(makeInfoTexture(p));
    if (std::find(panels.begin(),panels.end(),0) != panels.end()) {
        std::cerr << "No se pudieron crear los paneles de información.\n";
        for (GLuint texture:panels) if (texture) glDeleteTextures(1,&texture);
        for (Mesh& orbit:orbits) destroyMesh(orbit);
        destroyMesh(sphere); destroyMesh(rings); destroyMesh(panel); glDeleteProgram(program);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    
    // Cada planeta usa la imagen que aparece en la misma posición de esta lista.
    const wchar_t* planetTextureFiles[] = {
        L"mercurio (2).jpg", L"venus (1).jpg", L"tierra.jpg", L"marte.jpg", L"jupiter.jpg",
        L"saturno.jpg", L"urano.jpg", L"neptuno.jpg"
    };
    std::vector<GLuint> planetTextures(8, 0);
    const std::wstring textureDirectory = executableDirectory() + L"\\Texturas\\";
    for (int i = 0; i < 8; ++i) {
        planetTextures[i] = loadPlanetTexture(textureDirectory + planetTextureFiles[i]);
    }
    const GLuint sunTexture = loadPlanetTexture(textureDirectory + L"sol.jpg");
    const GLuint spaceTexture = loadPlanetTexture(textureDirectory + L"espacioexteriorskycube.jpg");
    std::vector<AstronautPart> astronautParts = loadAstronautModel(findProjectAsset(L"Modelo", L"Astronaut.glb"));
    Mesh astronautPath{};
    Mesh cameraTourPath{};

    
    // Guardo la pista, el volumen y si la música está sonando.
    int selectedTrack = 0;
    int musicVolume = 350;
    bool musicPlaying = startMusicTrack(selectedTrack, musicVolume);
    if (!musicPlaying) std::cerr << "No se pudo iniciar minisoundtrack\\holaespacio.mp3.\n";

    // La tecla I muestra u oculta las fichas.
    std::cout<<"Controles: WASD mover, mouse mirar, Espacio subir, Shift bajar, I mostrar/ocultar todas las fichas, Esc salir.\n";
    float lastTime=static_cast<float>(glfwGetTime());
    float simulationTime=0.0f, simulationSpeed=1.0f;
    bool showInfo=true, showOrbits=true, showSky=true, paused=false, iWasDown=false, tabWasDown=false, fWasDown=false;
    int selectedPlanet=2;
    const char* planetLabels[] = {"Mercurio","Venus","Tierra","Marte","Jupiter","Saturno","Urano","Neptuno"};
    std::vector<Vec3> planetTints(8,Vec3{1.0f,1.0f,1.0f});
    if (!astronautParts.empty()) {
        startAstronautRoute(selectedPlanet, 0.0f, astronautPath);
    }
    //render loop
    while (!glfwWindowShouldClose(window)) {
    const float frameTime=static_cast<float>(glfwGetTime());
        const float dt=std::min(frameTime-lastTime,0.05f); lastTime=frameTime;
        if (!paused) simulationTime+=dt*simulationSpeed;
        if (astronautRouteActive && !paused) {
            astronautRouteProgress += dt * astronautRouteSpeed / astronautRouteDuration;
            if (astronautRouteProgress >= 1.0f) {
                advanceAstronautRoute(simulationTime * 0.30f, astronautPath);
            }
            astronautPosition = cubicBezier(astronautRouteStart, astronautRouteControl1,
                astronautRouteControl2, astronautRouteEnd, astronautRouteProgress);
        }
        // Pongo la cámara en la altura de los ojos y la apunto hacia donde avanza el astronauta.
        if (astronautCameraView && astronautRouteActive) {
            const Vec3 direction = bezierTangent(astronautRouteStart, astronautRouteControl1,
                astronautRouteControl2, astronautRouteEnd, astronautRouteProgress);
            cameraX = astronautPosition.x;
            cameraY = astronautPosition.y + astronautScale * 2.1f;
            cameraZ = astronautPosition.z;
            yaw = std::atan2(direction.z, direction.x) * 180.0f / PI;
            pitch = std::asin(std::max(-1.0f, std::min(1.0f, direction.y))) * 180.0f / PI;
        }
        // Recorro la curva y miro suavemente entre el planeta actual y el siguiente.
        if (cameraTourActive) {
            if (!paused) {
                cameraTourProgress += dt * cameraTourSpeed / cameraTourDuration;
                if (cameraTourProgress >= 1.0f)
                    advanceCameraTour(simulationTime * 0.30f, cameraTourPath);
            }

            const Vec3 position = cubicBezier(cameraTourStart, cameraTourControl1,
                cameraTourControl2, cameraTourEnd, cameraTourProgress);
            cameraX = position.x;
            cameraY = position.y;
            cameraZ = position.z;

            const Vec3 from = planetPosition(planets[cameraTourPlanet],
                simulationTime * 0.30f, cameraTourPlanet);
            const Vec3 to = planetPosition(planets[cameraTourTargetPlanet],
                simulationTime * 0.30f, cameraTourTargetPlanet);
            const Vec3 focus = from * (1.0f - cameraTourProgress) + to * cameraTourProgress;
            orientCameraToward(focus);
        }
        if (followPlanet) {
            const Vec3 target=planetPosition(planets[followedPlanet],simulationTime*0.30f,followedPlanet);
            cameraX=target.x+followOffset.x; cameraY=target.y+followOffset.y; cameraZ=target.z+followOffset.z;
            orientCameraToward(target);
        }
        const bool fDown=glfwGetKey(window,GLFW_KEY_F)==GLFW_PRESS;
        if (followPlanet && fDown && !fWasDown) {
            followPlanet=false;
            firstMouse=true;
        }
        fWasDown=fDown;
        const bool tabDown=glfwGetKey(window,GLFW_KEY_TAB)==GLFW_PRESS;
        if (tabDown && !tabWasDown) {
            cameraControl=!cameraControl;
            glfwSetInputMode(window,GLFW_CURSOR,cameraControl ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            firstMouse=true;
        }
        tabWasDown=tabDown;
        processInput(window,dt);
        const bool iDown=glfwGetKey(window,GLFW_KEY_I)==GLFW_PRESS;
        if (iDown && !iWasDown) showInfo=!showInfo;
        iWasDown=iDown;

        // Inicio un nuevo cuadro de interfaz y declaro sus controles antes de pedirle a ImGui que los dibuje.
    ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Los controles que agrego entre Begin y End aparecen en esta ventana.
    // Al mover un control, cambio una variable que la escena usa al actualizarse o dibujarse.
    ImGui::Begin("Controles del sistema solar");
        ImGui::Text("Tab: alternar entre camara y raton de la interfaz");
        ImGui::Text("Camara: %s",cameraControl ? (followPlanet ? "Siguiendo planeta" : "Libre") : "Interfaz");
        if (followPlanet) {
            ImGui::Text("Siguiendo a: %s  |  F: liberar camara",planetLabels[followedPlanet]);
        }
        if (followPlanet) {
            if (ImGui::Button("Dejar de seguir")) {
                followPlanet=false; firstMouse=true;
            }
        }
        // Estos controles afectan el reloj general: puedo pausar, reiniciar o cambiar la velocidad visual.
    ImGui::SeparatorText("Simulacion");
        if (ImGui::Button(paused ? "Reanudar" : "Pausar")) paused=!paused;
        ImGui::SameLine();
        if (ImGui::Button("Reiniciar tiempo")) simulationTime=0.0f;
        ImGui::SliderFloat("Velocidad global",&simulationSpeed,0.0f,5.0f,"%.2fx");
        // Esta sección permite elegir una pista, avanzar, pausar y cambiar volumen de la música.
    ImGui::SeparatorText("Reproductor de musica");
        ImGui::Text("Ahora suena: %s%s", musicLabels[selectedTrack], musicPlaying ? "" : " (pausada o no disponible)");
        if (ImGui::BeginCombo("Pista", musicLabels[selectedTrack])) {
            for (int i = 0; i < static_cast<int>(sizeof(musicFiles) / sizeof(musicFiles[0])); ++i) {
                const bool selected = (selectedTrack == i);
                if (ImGui::Selectable(musicLabels[i], selected)) {
                    selectedTrack = i;
                    musicPlaying = startMusicTrack(selectedTrack, musicVolume);
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Anterior")) {
            selectedTrack = (selectedTrack + static_cast<int>(sizeof(musicFiles) / sizeof(musicFiles[0])) - 1) % static_cast<int>(sizeof(musicFiles) / sizeof(musicFiles[0]));
            musicPlaying = startMusicTrack(selectedTrack, musicVolume);
        }
        ImGui::SameLine();
        if (ImGui::Button(musicPlaying ? "Pausar musica" : "Reanudar musica")) {
            if (musicPlaying) {
                mciSendStringW(L"pause solarMusic", nullptr, 0, nullptr);
                musicPlaying = false;
            } else {
                if (mciSendStringW(L"resume solarMusic", nullptr, 0, nullptr) != 0)
                    musicPlaying = startMusicTrack(selectedTrack, musicVolume);
                else musicPlaying = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Siguiente")) {
            selectedTrack = (selectedTrack + 1) % static_cast<int>(sizeof(musicFiles) / sizeof(musicFiles[0]));
            musicPlaying = startMusicTrack(selectedTrack, musicVolume);
        }
        if (ImGui::SliderInt("Volumen", &musicVolume, 0, 1000) && musicPlaying) {
            const std::wstring volumeCommand = L"setaudio solarMusic volume to " + std::to_wstring(musicVolume);
            mciSendStringW(volumeCommand.c_str(), nullptr, 0, nullptr);
        }
        // Selecciono un planeta y ajusto sus variables; el cambio se refleja en la siguiente actualización.
    ImGui::SeparatorText("Planetas");
        if (ImGui::BeginCombo("Planeta",planetLabels[selectedPlanet])) {
            // Para cada planeta calculo su posición, preparo su transformación y dibujo su textura y elementos asociados.
    for (int i=0;i<8;++i) {
                const bool isSelected=(selectedPlanet==i);
                if (ImGui::Selectable(planetLabels[i],isSelected)) selectedPlanet=i;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SliderFloat("Velocidad orbital",&planets[selectedPlanet].speed,0.0f,3.0f,"%.2fx");
        ImGui::ColorEdit3("Tinte del planeta",&planetTints[selectedPlanet].x);
        // Controlo visibilidad, color, escala, velocidad y estado del vuelo Bézier.
    ImGui::SeparatorText("Astronauta");
        if (ImGui::Button(showAstronaut ? "Quitar astronauta" : "Mostrar astronauta"))
            showAstronaut = !showAstronaut;
        if (ImGui::Button(astronautCameraView ? "Salir de vista astronauta" : "Vista desde astronauta")) {
            astronautCameraView = !astronautCameraView;
            if (astronautCameraView) {
                cameraTourActive = false;
                if (!astronautRouteActive)
                    startAstronautRoute(selectedPlanet, simulationTime * 0.30f, astronautPath);
                followPlanet = false;
                cameraControl = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                firstMouse = true;
            }
        }
        ImGui::Checkbox("Mostrar curva de vuelo", &showAstronautPath);
        ImGui::ColorEdit3("Color del traje", &astronautTint.x);
        ImGui::SliderFloat("Velocidad del vuelo", &astronautRouteSpeed, 0.1f, 4.0f, "%.1fx");
        ImGui::SliderFloat("Tamano del astronauta", &astronautScale, 0.2f, 1.8f, "%.2fx");
        if (astronautParts.empty()) {
            ImGui::TextWrapped("No se pudo cargar Modelo/Astronaut.glb. Revisa la consola de Visual Studio.");
        } else if (ImGui::Button(astronautRouteActive ? "Detener recorrido" : "Recorrer planetas")) {
            if (astronautRouteActive) {
                astronautRouteActive = false;
                astronautCameraView = false;
            } else {
                startAstronautRoute(selectedPlanet, simulationTime * 0.30f, astronautPath);
            }
        }
        if (astronautRouteActive) ImGui::ProgressBar(astronautRouteProgress, ImVec2(-1.0f, 0.0f));
        ImGui::SeparatorText("Recorrido de cámara");
        if (ImGui::Button(cameraTourActive ? "Detener recorrido de cámara" : "Recorrer planetas con cámara")) {
            if (cameraTourActive) {
                cameraTourActive = false;
            } else {
                astronautCameraView = false;
                followPlanet = false;
                cameraControl = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                startCameraTour(selectedPlanet, simulationTime * 0.30f, cameraTourPath);
            }
        }
        if (cameraTourActive)
            ImGui::Text("Visitando: %s", planetLabels[cameraTourTargetPlanet]);
        ImGui::Checkbox("Mostrar curva de cámara", &showCameraTourPath);
        ImGui::SliderFloat("Velocidad del recorrido", &cameraTourSpeed, 0.2f, 3.0f, "%.1fx");
        if (ImGui::Button("Enfocar planeta")) {
            astronautCameraView = false;
            cameraTourActive = false;
            const Planet& targetPlanet=planets[selectedPlanet];
            const Vec3 target=planetPosition(targetPlanet,simulationTime*0.30f,selectedPlanet);
            const Vec3 camera=target+Vec3{0.0f,targetPlanet.radius+1.5f,targetPlanet.radius+3.0f};
            cameraX=camera.x; cameraY=camera.y; cameraZ=camera.z;
            followedPlanet=selectedPlanet;
            followOffset=camera-target;
            followPlanet=true;
            const Vec3 direction=normalize(target-camera);
            yaw=std::atan2(direction.z,direction.x)*180.0f/PI;
            pitch=std::asin(direction.y)*180.0f/PI;
            cameraControl=true; firstMouse=true;
            glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        }
        // Estas casillas controlan elementos visuales y permiten recuperar la posición inicial de la cámara.
    ImGui::SeparatorText("Vista");
        ImGui::Checkbox("Mostrar orbitas",&showOrbits);
        ImGui::Checkbox("Mostrar fondo estrellado",&showSky);
        ImGui::Checkbox("Mostrar fichas informativas",&showInfo);
        if (ImGui::Button("Restablecer camara")) {
            astronautCameraView = false;
            cameraTourActive = false;
            cameraX=0.0f; cameraY=24.0f; cameraZ=59.0f; yaw=-90.0f; pitch=-19.0f; firstMouse=true;
            followPlanet=false;
        }
        ImGui::End();
        //parte ventana glfw
    int width,height; glfwGetFramebufferSize(window,&width,&height); if (height<1) height=1;
        glViewport(0,0,width,height); glClearColor(0.008f,0.012f,0.035f,1.0f); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        const float yr=yaw*PI/180.0f,pr=pitch*PI/180.0f;
        const Vec3 forward=normalize({std::cos(yr)*std::cos(pr),std::sin(pr),std::sin(yr)*std::cos(pr)});
        Vec3 eye{cameraX,cameraY,cameraZ}; float view[16],projection[16];
        // view describe dónde está mirando la cámara; projection añade la perspectiva.
        lookAt(view,eye,eye+forward,{0,1,0}); perspective(projection,45.0f*PI/180.0f,static_cast<float>(width)/height,0.1f,180.0f);
        glUseProgram(program); glUniformMatrix4fv(viewLoc,1,GL_FALSE,view); glUniformMatrix4fv(projectionLoc,1,GL_FALSE,projection);
        // La luz está donde está el Sol y también envío la posición de la cámara
        // para que el shader calcule cómo se ilumina cada superficie.
        glUniform3f(lightLoc,0,0,0); glUniform3f(cameraLoc,cameraX,cameraY,cameraZ);

        // La imagen estrellada es una sola textura, así que se proyecta sobre una esfera
        // grande centrada en la cámara para formar el fondo espacial.
        // Dibujo primero el fondo sin profundidad para que rellene el espacio detrás de todos los objetos.
    if (showSky && spaceTexture) {
            float skyModel[16]; modelMatrix(skyModel,eye,130.0f);
            glDisable(GL_DEPTH_TEST);
            drawMesh(program,sphere,skyModel,{1,1,1},4,0,spaceTexture);
            glEnable(GL_DEPTH_TEST);
        }

        // Primero dibujamos las órbitas como líneas finas. Se desactiva profundidad para
        // que no desaparezcan detrás de los cuerpos mientras sirven de guía visual.
        
    if (showOrbits) {
            glDisable(GL_DEPTH_TEST);
            for (const Mesh& orbit:orbits) {
                float m[16]; identity(m); drawMesh(program,orbit,m,{0.16f,0.22f,0.34f},1);
            }
            glEnable(GL_DEPTH_TEST);
        }

        // El Sol se dibuja en el centro con un modo brillante que no depende de la luz externa.
        float sunModel[16]; modelMatrix(sunModel,{0,0,0},2.5f,simulationTime*0.12f);
        if (sunTexture) drawMesh(program,sphere,sunModel,{1,1,1},4,0,sunTexture);
        else drawMesh(program,sphere,sunModel,{1.0f,0.53f,0.12f},1);

        // Dibujo cada planeta en su posición actual.
        for (int i=0;i<8;++i) {
            const Planet& p=planets[i]; const Vec3 position=planetPosition(p,simulationTime*0.30f,i);
            float m[16]; modelMatrix(m,position,p.radius,simulationTime*p.speed*0.35f);
            const Vec3 surfaceColor = planetTextures[i] ? planetTints[i] : p.color;
            drawMesh(program,sphere,m,surfaceColor,2,p.style,planetTextures[i]);
            // El planeta número 5 es Saturno; también dibujo sus anillos.
            if (i==5) {
                float ringModel[16]; modelMatrix(ringModel,position,1.0f,simulationTime*0.01f,0.42f);
                drawMesh(program,rings,ringModel,{0.78f,0.66f,0.45f},0);
            }
            // Giro cada ficha hacia la cámara para que se pueda leer.
            if (showInfo) {
                const Vec3 cameraRight=normalize(cross(forward,{0,1,0}));
                const Vec3 cameraUp=normalize(cross(cameraRight,forward));
                // Pongo las fichas a distintas alturas y a ambos lados
                // para que se tapen menos entre ellas.
                const float side=(i%2==0)?1.0f:-1.0f;
                const float row=static_cast<float>((i%3)-1)*0.30f;
                const Vec3 labelPosition=position+cameraRight*(side*(p.radius+5.5f))+cameraUp*(p.radius+0.55f+row);
                float billboard[16]={
                    cameraRight.x*4.80f,cameraRight.y*4.80f,cameraRight.z*4.80f,0,
                    cameraUp.x*2.85f,cameraUp.y*2.85f,cameraUp.z*2.85f,0,
                    -forward.x,-forward.y,-forward.z,0,
                    labelPosition.x,labelPosition.y,labelPosition.z,1
                };
                // Dibujo la ficha encima para que un planeta no la esconda.
                glDisable(GL_DEPTH_TEST); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,panels[i]);
                drawMesh(program,panel,billboard,{1,1,1},3); glEnable(GL_DEPTH_TEST);
            }
        }
        if (showCameraTourPath && cameraTourPath.vao) {
            float cameraPathModel[16]; identity(cameraPathModel);
            glLineWidth(2.0f);
            drawMesh(program, cameraTourPath, cameraPathModel, {1.0f, 0.62f, 0.20f}, 1);
            glLineWidth(1.0f);
        }
        if (showAstronautPath && astronautPath.vao) {
            float pathModel[16]; identity(pathModel);
            glLineWidth(2.0f);
            drawMesh(program, astronautPath, pathModel, {0.18f,0.78f,1.0f}, 1);
            glLineWidth(1.0f);
        }
        // muestra el astronauta 
    if (showAstronaut && !astronautCameraView && !astronautParts.empty()) {
            Vec3 direction{0.0f, 0.0f, 1.0f};
            if (astronautRouteActive)
                direction = bezierTangent(astronautRouteStart, astronautRouteControl1,
                    astronautRouteControl2, astronautRouteEnd,
                    astronautRouteProgress);
            const float modelYaw = std::atan2(direction.x, direction.z);
            const float modelPitch = -std::asin(std::max(-1.0f, std::min(1.0f, direction.y)));
            float astronautModel[16];
            modelMatrix(astronautModel, astronautPosition, astronautScale, modelYaw, modelPitch);
            for (const AstronautPart& part : astronautParts) {
                const Vec3 color{ part.color.x * astronautTint.x, part.color.y * astronautTint.y,
                    part.color.z * astronautTint.z };
                drawMesh(program, part.mesh, astronautModel, color, 2, 0, part.texture);
            }
        }
    ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window); glfwPollEvents();
    }

	//liberamos espacio y cerramos la ventana
    for (GLuint texture:panels) glDeleteTextures(1,&texture);
    for (GLuint texture:planetTextures) if (texture) glDeleteTextures(1,&texture);
    if (sunTexture) glDeleteTextures(1,&sunTexture);
    if (spaceTexture) glDeleteTextures(1,&spaceTexture);
    if (astronautPath.vao) destroyMesh(astronautPath);
    if (cameraTourPath.vao) destroyMesh(cameraTourPath);
    for (AstronautPart& part : astronautParts) {
        destroyMesh(part.mesh);
        if (part.texture) glDeleteTextures(1, &part.texture);
    }
    mciSendStringW(L"close solarMusic", nullptr, 0, nullptr);
    for (Mesh& orbit:orbits) destroyMesh(orbit);
    destroyMesh(sphere); destroyMesh(rings); destroyMesh(panel); glDeleteProgram(program);
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(window); glfwTerminate(); return 0;
}
