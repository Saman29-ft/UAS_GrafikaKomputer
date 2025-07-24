#include <glew.h>                       // Untuk ekstensi OpenGL
#include <glfw3.h>                      // Untuk window dan input
#include <iostream>                     // Untuk error logging
#include <glm.hpp>                      // Untuk operasi matematika (matriks 4x4, vektor 3D)    
#include <gtc/matrix_transform.hpp>     // Fungsi transformasi (rotate, translate, scale)
#include <gtc/type_ptr.hpp>             // Konversi GLM ke pointer OpenGL

//menghubungkan library secara otomatis di Windows
#ifdef _WIN32
#include <windows.h>                    // inisialisasi OpenGL
#pragma comment(lib, "glew32.lib")      // harus di-link agar fungsi GLEW (seperti glewInit()) bisa dipanggil
#endif


// Transformasi posisi vertex dari koordinat lokal ke koordinat layar (clip space) 
// dan Meneruskan warna vertex ke fragment shader untuk interpolasi.
const char* vertexShaderSource = R"(
    #version 330 core                       //Versi GLSL 3.30 dengan core profile (modern OpenGL)                     
    layout (location = 0) in vec3 aPos;     // Input posisi vertex (x,y,z)
    layout (location = 1) in vec3 aColor;   // Input warna vertex (r,g,b) untuk"
    out vec3 fragColor;                     // output fragment shader"
    uniform mat4 model;                     // Matriks transformasi (rotate/translate/scale) objek (local -> world space)
    uniform mat4 view;                      // Matriks kamera (world -> view space)
    uniform mat4 projection;                // Matriks proyeksi (view -> clip space)

    // Fungsi main mentransformasi geometri posisi vertex 3D -> 2D dan warna fragment
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        fragColor = aColor;
    }
)";

/*
  Shader ini bertanggung jawab untuk menentukan warna akhir setiap pixel(fragment)
  yang dihasilkan dari rasterisasi geometri.
*/ 
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;                     // Output warna akhir pixel (RGBA dengan alpha=1.0)
    in vec3 fragColor;                      // Input warna dari vertex shader
    void main() {                           // Menggabungkan warna rgb (r,g,b) dengan aplha channel 1.0 menjadi (r,g,b,a)
        FragColor = vec4(fragColor, 1.0);
    }
)";


/*
    Fungsi ini digunakan untuk menginisialisasi GLEW (OpenGL Extension Wrangler Library), 
    yang diperlukan untuk mengakses fungsi-fungsi modern OpenGL.
*/
void initializeGLEW() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }
}


/*
    Fungsi ini adalah callback function yang dipanggil secara otomatis 
    oleh GLFW ketika ukuran framebuffer (area drawable) window berubah
*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


//  Fungsi ini bertugas untuk memproses input keyboard (ESC) dalam aplikasi OpenGL/GLFW
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}


/*
  Fungsi yang bertanggung jawab Compile shaders and link program
    Alur Proses Shader
    - Pembuatan Shader Object
    - Kompilasi Source Code
    - Error Handling
    - Linking Program
    - Cleanup Resources
*/ 
GLuint compileShaders() {
    GLuint vertexShader = 
        glCreateShader(GL_VERTEX_SHADER); // Membuat container shader dengan tipe tertentu
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr); // Memasukkan source code GLSL
        glCompileShader(vertexShader); // Mengompilasi ke bahasa mesin GPU
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Membuat objek shader
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr); // Menggunakan sumber fragmentShaderSource ke dalam shader
    glCompileShader(fragmentShader); // Kompilasi menjadi binary untuk dijalankan GPU


    // Memastikan kompilasi shader berhasil
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Shader Program
    GLuint shaderProgram = glCreateProgram(); // Membuat program shader kosong
    glAttachShader(shaderProgram, vertexShader); // Hubungkan hasil kompilasi vertexShader ke program shader
    glAttachShader(shaderProgram, fragmentShader); // Hubungkan hasil kompilasi fragmentShader ke program shader
    glLinkProgram(shaderProgram); // Menghubungkan semua shader yang sudah dilampirkan

    /*
        Memverifikasi apakah proses linking program shader berhasil.
        Jika gagal, mengambil pesan error detail dari sistem dan
        menampilkan error ke output console untuk debugging
    */
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Hapus vertexShader dan fragmentShader untuk mencegah memory leak
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Mas Erik


int main() {
    // Initialize GLFW library
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }

    // Create GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL 3D Cube", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    initializeGLEW();


    // Set OpenGL settings
    /*
        Aktifkan depth testing untuk evelauasi kedalaman (z-value) yaitu
        jarak dengan kamera, untuk memastikan apakah objek perlu dirender.
    */
    glEnable(GL_DEPTH_TEST);


    // 8 titik pemebentuk sebuah kubus
    GLfloat vertices[] = {
        // Positions (x, y, z)      // Colors (r, g, b)
        -0.5f, -0.5f, -0.5f,        1.0f, 0.0f, 0.0f, // Front-left bottom - Merah
         0.5f, -0.5f, -0.5f,        0.0f, 1.0f, 0.0f, // Front-right bottom - Hijau
         0.5f,  0.5f, -0.5f,        0.0f, 0.0f, 1.0f, // Front-right top - Biru
        -0.5f,  0.5f, -0.5f,        1.0f, 1.0f, 0.0f, // Front-left top - Kuning
        -0.5f, -0.5f,  0.5f,        1.0f, 0.0f, 1.0f, // Back-left bottom - Magenta
         0.5f, -0.5f,  0.5f,        0.0f, 1.0f, 1.0f, // Back-right bottom - Cyan
         0.5f,  0.5f,  0.5f,        1.0f, 1.0f, 1.0f, // Back-right top - Putih
        -0.5f,  0.5f,  0.5f,        0.5f, 0.5f, 0.5f  // Back-left top - Abu-abu
    };


    /*
        Triangulation
        Index untuk menghubungkan vertex menjadi segitiga. Setiap wajah kubus
        bisa dibentuk dari dua segitiga, sehingga total ada 12 segitiga untuk kubus.
        (6 sisi x 2 segitiga per sisi = 12 segitiga).
    */
    GLuint indices[] = {
        0, 1, 2,  0, 2, 3,   // Front
        4, 5, 6,  4, 6, 7,   // Back
        0, 1, 5,  0, 5, 4,   // Bottom
        2, 3, 7,  2, 7, 6,   // Top
        0, 3, 7,  0, 7, 4,   // Left
        1, 2, 6,  1, 6, 5    // Right
    };


    GLuint VAO, VBO, EBO;       // Vertex Array Object, Vertex Buffer Object, Element Buffer Object
    glGenVertexArrays(1, &VAO); // Buat vertex array id -> VAO
    glGenBuffers(1, &VBO);      // Buat buffer vertex id -> VBO (posisi dan warna)
    glGenBuffers(1, &EBO);      // Buat buffer element id -> EBO (indices triangulation)

    glBindVertexArray(VAO);

    // Copy vertex data to buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // mengalokasikan sekaligus mengisi memori GPU dengan data

    // Copy index data to buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // mengalokasikan ruang GPU untuk indices dan mengisi-nya sekaligus

    // Set up vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0); // Menjelaskan kepada shader (melalui VAO yang sedang aktif) bagaimana cara membaca posisi dari buffer VBO.
    glEnableVertexAttribArray(0); // Menunjukkan akses location 0 -> vertex position

    // Set up vertex color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // Menunjukkan akses location 1 -> vertex color

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile shaders and link program
    GLuint shaderProgram = compileShaders();
    glUseProgram(shaderProgram);

    // Get uniform locations
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model"); // dapatkan lokasi modelLoc di shader "model"
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view"); // dapatkan lokasi viewLoc di shader "view"
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection"); // dapatkan lokasi projectionLoc di shader "projection"

    // Main loop
    while (!glfwWindowShouldClose(window)) {            // Pastikan window belum menerima instruksi mengakhiri program dengan (ESC)
        processInput(window);                           // Proses input yang diterima window

        // Rendering
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);               // Buat warna latar abu-abu gelap pekat (hampir hitam)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // reset nilai z-buffer ke 1.0 (jauh), agar depth-test mulai dari awal.

        // Buat matriks 4x4 transformasi setiap frame untuk model, view, dan projection
        glm::mat4 model = glm::mat4(1.0f); // model awal tanpa transformasi (identity matrix)

        glm::mat4 view =
            glm::lookAt(
                glm::vec3(2.0f, 2.0f, 2.0f),    // posisi kamera
                glm::vec3(0.0f, 0.0f, 0.0f),    // titik fokus kamera (pusat kubus)
                glm::vec3(0.0f, 1.0f, 0.0f)     // arah atas kamera (y-axis)
            );

        glm::mat4 projection =                          // Matriks proyeksi perspektif
            glm::perspective(
                glm::radians(45.0f),       // 
                800.0f / 600.0f,
                0.1f,
                100.0f
            );         // Rasio aspek 800:600, near plane 0.1, far plane 100.0

        // Rotate the cube
        model =
            glm::rotate(
                model,
                (float)glfwGetTime(),           // Waktu sejak aplikasi dimulai (dalam detik)
                glm::vec3(0.5f, 1.0f, 0.0f)     // Sumbu rotasi (x,y,z) untuk rotasi kubus
            );
        /*
            Y ↑
              │   /
            1 │  /
              │ / ← garis rotasi(0.5, 1, 0)
              │/_.___→ X
                0.5
        */


        // Pass transformation matrices to shaders
        // Kirim matriks transformasi model ke shader (untuk rotasi/transformasi objek)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Kirim matriks view (kamera) ke shader
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Kirim matriks proyeksi ke shader (untuk perspektif 3D)
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw the cube
        // Aktifkan VAO (Vertex Array Object) yang berisi konfigurasi vertex dan buffer
        glBindVertexArray(VAO);

        /*
            Gambar kubus menggunakan elemen(indeks) yang telah didefinisikan,
            sebanyak 36 indeks(12 segitiga)
        */
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Nonaktifkan VAO setelah menggambar untuk mencegah perubahan tidak sengaja
        glBindVertexArray(0);

        // Swap buffers and poll events
        // Tukar buffer depan dan belakang untuk menampilkan gambar yang telah dirender
        glfwSwapBuffers(window);
        // Ambil dan proses event (keyboard, mouse, dll.)
        glfwPollEvents();
    }

    // Jika window menerima input (ESC), bersihkan resource OpenGL yang telah dialokasikan sebelum keluar
    glDeleteVertexArrays(1, &VAO); // Hapus Vertex Array Object
    glDeleteBuffers(1, &VBO);      // Hapus Vertex Buffer Object
    glDeleteBuffers(1, &EBO);      // Hapus Element Buffer Object

    glfwTerminate(); // Akhiri dan bersihkan library GLFW
    return 0;        // Kembalikan nilai 0 sebagai tanda program selesai dengan sukses
}
