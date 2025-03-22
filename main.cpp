#include <filesystem>
#include <iostream>
#include <string>

#include <RmlUi/Core.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexCode, const char* fragmentCode) {
        // Vertex shader
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment shader
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

private:
    void checkCompileErrors(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "SHADER_COMPILATION_ERROR: " << type << "\n" << infoLog << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "PROGRAM_LINKING_ERROR: " << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

class GL3RenderInterface : public Rml::RenderInterface {
public:
    struct GeometryData {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ibo = 0;
        int num_indices = 0;
    };

    std::unordered_map<Rml::CompiledGeometryHandle, GeometryData> geometries;
    Rml::CompiledGeometryHandle next_geometry_handle = 1;
    std::unordered_map<Rml::TextureHandle, GLuint> textures;
    Rml::TextureHandle next_texture_handle = 1;
    Shader* shader = nullptr;

    void SetShader(Shader* shader) {
        this->shader = shader;
    }

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override {
        GeometryData data;

        glGenVertexArrays(1, &data.vao);
        glBindVertexArray(data.vao);

        glGenBuffers(1, &data.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Rml::Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, position));
        // Color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, colour));
        // Tex Coord
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, tex_coord));

        glGenBuffers(1, &data.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), indices.data(), GL_STATIC_DRAW);

        data.num_indices = (int)indices.size();

        const auto handle = next_geometry_handle++;
        geometries[handle] = data;
        return handle;
    }

    // Modified RenderGeometry function
    void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override {
        if (!geometries.count(handle)) return;
        if (!shader) return;

        GeometryData& data = geometries[handle];

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        shader->use();

        // Set model matrix with translation
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
        shader->setMat4("uModel", model);
        shader->setInt("uTexture", 0);

        if (texture && textures.count(texture)) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[texture]);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindVertexArray(data.vao);
        glDrawElements(GL_TRIANGLES, data.num_indices, GL_UNSIGNED_INT, 0);

        glDisable(GL_BLEND);
    }

    // ... (Keep other GL3RenderInterface methods the same as before)
    void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override
    {
        if (geometries.count(handle)) {
            GeometryData& data = geometries[handle];
            glDeleteVertexArrays(1, &data.vao);
            glDeleteBuffers(1, &data.vbo);
            glDeleteBuffers(1, &data.ibo);
            geometries.erase(handle);
        }
    }

    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override
    {
        std::cout << "Loading texture: " << source << std::endl;

        // Implement proper texture loading here (using stb_image or similar)
        // This is a simplified example
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // Dummy texture - white pixel
        const unsigned char pixels[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        const auto handle = next_texture_handle++;
        textures[handle] = texture_id;
        texture_dimensions = { 1, 1 };
        return handle;
    }

    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override
    {
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            source_dimensions.x, source_dimensions.y,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            source.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        const auto handle = next_texture_handle++;
        textures[handle] = texture_id;
        return handle;
    }

    void ReleaseTexture(Rml::TextureHandle texture) override
    {
        if (textures.count(texture)) {
            glDeleteTextures(1, &textures[texture]);
            textures.erase(texture);
        }
    }

    void EnableScissorRegion(bool enable) override
    {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    void SetScissorRegion(Rml::Rectanglei region) override
    {
        // OpenGL expects scissor region in window coordinates, which start at the bottom left
        int height = 0;
        GLFWwindow* window = glfwGetCurrentContext();
        glfwGetFramebufferSize(window, nullptr, &height);

        glScissor(region.Left(), height - region.Bottom(), region.Width(), region.Height());
    }

    // Optional: Implement these if needed for advanced features
    void EnableClipMask(bool /*enable*/) override {}
    void RenderToClipMask(Rml::ClipMaskOperation /*operation*/, Rml::CompiledGeometryHandle /*geometry*/, Rml::Vector2f /*translation*/) override {}
    void SetTransform(const Rml::Matrix4f* /*transform*/) override {}
};

class GLFWSystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override { return glfwGetTime(); }
};

class CustomFileInterface : public Rml::FileInterface {
public:
    CustomFileInterface(const Rml::String& root) : root(root) {}

    Rml::FileHandle Open(const Rml::String& path) override {
        Rml::String fullPath = root + path;
        FILE* fp = fopen(fullPath.c_str(), "rb");
        return (Rml::FileHandle)fp;
    }

    void Close(Rml::FileHandle file) override { fclose((FILE*)file); }
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override { return fread(buffer, 1, size, (FILE*)file); }
    bool Seek(Rml::FileHandle file, long offset, int origin) override { return fseek((FILE*)file, offset, origin) == 0; }
    size_t Tell(Rml::FileHandle file) override { return ftell((FILE*)file); }

private:
    Rml::String root;
};

// ... (Keep input callback functions the same)

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "RmlUi Sample", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    // Shaders
    // Modified vertex shader
    const char* vertexShader = R"(
        #version 330 core
        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec4 aColor;
        layout(location = 2) in vec2 aTexCoord;
    
        uniform mat4 uProjection;
        uniform mat4 uModel;
    
        out vec4 vColor;
        out vec2 vTexCoord;
    
        void main() {
            gl_Position = uProjection * uModel * vec4(aPosition, 0.0, 1.0);
            vColor = aColor;
            vTexCoord = aTexCoord;
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        in vec4 vColor;
        in vec2 vTexCoord;
        
        uniform sampler2D uTexture;
        
        out vec4 FragColor;
        
        void main() {
            FragColor = vColor * texture(uTexture, vTexCoord);
        }
    )";

    Shader rmlShader(vertexShader, fragmentShader);

    GL3RenderInterface render_interface;
    render_interface.SetShader(&rmlShader);
    GLFWSystemInterface system_interface;

    Rml::SetRenderInterface(&render_interface);
    Rml::SetSystemInterface(&system_interface);
    Rml::SetFileInterface(new CustomFileInterface("./"));

    if (!Rml::Initialise()) {
        glfwTerminate();
        return -1;
    }

    Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(1024, 768));
    glfwSetWindowUserPointer(window, context);

    // ... (Set up input callbacks same as before)
    // Add these input callback registrations right after creating the context in main()
    /*glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);*/
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) {
        Rml::Context* context = (Rml::Context*)glfwGetWindowUserPointer(window);
        if (context) context->ProcessTextInput((char)codepoint);
        });

    // Load fonts and document
    Rml::LoadFontFace("assets/LatoLatin-Regular.ttf");
    Rml::LoadFontFace("assets/LatoLatin-Bold.ttf");
    Rml::LoadFontFace("assets/LatoLatin-Italic.ttf");

    if (auto doc = context->LoadDocument("assets/demo.rml")) {
        doc->Show();
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Set up projection matrix
        glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
        rmlShader.use();
        rmlShader.setMat4("uProjection", projection);

        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        context->Update();
        context->Render();
        // After context->Render()
        glBindVertexArray(0);
        glUseProgram(0);
        glDrawArrays(GL_TRIANGLES, 0, 3); // Simple test triangle

        glfwSwapBuffers(window);
    }

    Rml::Shutdown();
    glfwTerminate();
    return 0;
}