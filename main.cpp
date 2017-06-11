#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#ifdef _WIN32
#define GLEW_STATIC
#include "glew.c"
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct GLvec3
{
    GLfloat x, y, z;
};

void generate_vertices(std::vector<GLvec3> &vertices, std::ifstream &file)
{
    std::string t;
    while(std::getline(file, t))
        if(t.substr(0,2)=="v ")
        {
            GLvec3 vertex;
            std::istringstream in(t.substr(t.find(" ")+1));
            in >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        }
}

void generate_faces(std::vector< std::vector<GLuint> > &faces, std::ifstream &file)
{
    std::string t;
    while(std::getline(file, t))
    {
        if(t.substr(0,2)=="f ")
        {
            std::vector<GLuint> face;
            std::istringstream in(t.substr(t.find(" ")+1));
            if(t.find("/") == std::string::npos)
            {
               GLuint val;
               while(in)
               {
                   in >> val;
                   face.push_back(val-1);
                   std::getline(in, t, ' ');
               }
            }
            else while(std::getline(in, t, '/'))
            {
                face.push_back(stoi(t)-1);
                std::getline(in, t, ' ');
            }
            faces.push_back(face);
        }
    }
}

void draw_face(std::vector<GLuint> &face)
{
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*face.size(), face.data(), GL_STATIC_DRAW);
    glDrawElements(GL_LINE_STRIP, face.size(), GL_UNSIGNED_INT, 0);
}

GLvec3 operator/ (GLvec3 v, float d)
{
    GLvec3 t;
    t.x = v.x/d;
    t.y = v.y/d;
    t.z = v.z/d;
    return t;
}

bool operator< (GLvec3 u, GLvec3 v)
{
    return (std::max( std::max(abs(u.x),abs(u.y)) , abs(u.z) ) -
            std::max( std::max(abs(v.x),abs(v.y)) , abs(v.z) ) < 0);
}

std::ostream& operator<<(std::ostream &out, GLvec3 v)
{
    out << v.x << ' ' << v.y << ' ' << v.z << std::endl;
    return out;
}

void normalize(std::vector<GLvec3> &v)
{
    auto t = std::max_element(v.begin(), v.end());
    GLvec3 u = v[std::distance(v.begin(), t)];
    GLfloat max_val = std::max( std::max(abs(u.x),abs(u.y)) , abs(u.z) ) > 1.0f ?
                std::max( std::max(abs(u.x),abs(u.y)) , abs(u.z) ) : 1.0f;
    for(auto &&i : v)
        i = i/max_val;
}

GLuint shaders(const char *v, const char *f)
{
    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(v_shader, 1, &v, NULL);
    glShaderSource(f_shader, 1, &f, NULL);

    glCompileShader(v_shader);
    glCompileShader(f_shader);

    GLuint shader_program = glCreateProgram();

    glAttachShader(shader_program, v_shader);
    glAttachShader(shader_program, f_shader);
    glLinkProgram(shader_program);

    glDetachShader(shader_program, v_shader);
    glDetachShader(shader_program, f_shader);

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return shader_program;
}

const char *v = "#version 330\n"
                "layout(location = 0) in vec3 position;\n"
                "out vec3 pos;\n"
                "uniform mat4 rotMatrix;\n"
                "uniform mat4 transMatrix;\n"
                "uniform mat4 scaleMatrix;\n"
                "void main() {\n"
                "gl_Position = scaleMatrix * transMatrix * rotMatrix * vec4(position, 1);\n"
                "pos = gl_Position.xyz;\n"
                "}\n";

const char *f = "#version 330\n"
                "in vec3 pos;\n"
                "out vec3 color;\n"
                "uniform float time;\n"
                "uniform float fog_r, fog_g, fog_b;\n"
                "void main() {\n"
                "vec3 fogCol = vec3(fog_r, fog_g, fog_b);\n"
                "float fog = clamp(distance(clamp(-pos.z, 0, 1), 0), 0, 1);\n"
                "color = vec3(.7,1,0);\n"
                "color = mix(fogCol, color, fog);}\n";

int main(int argc, char **argv)
{
    if(argc!=2)
    {
        std::cout << "Usage: GLtest file.obj\n";
        return 1;
    }

    srand(time(NULL));
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window;
    window = glfwCreateWindow(640, 480, "GL_test", 0, 0);
    glfwMakeContextCurrent(window);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_CLAMP);
    glDepthRange(0.001f, 2000.0f);

    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK ) return -1;
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));
    glm::mat4 transMatrix = glm::translate(glm::vec3(-.05f, .0f, -.55f));

    float rotVal = 0.0f, rotX = .75f, rotY = .4f, rotZ = .6f;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::ifstream file(argv[1]);

    std::vector<GLvec3> vertices;
    generate_vertices(vertices, file);
    normalize(vertices);

    file.close();
    file.clear();

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLvec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

    GLuint shader = shaders(v, f);

    glUseProgram(shader);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    file.open(argv[1]);

    std::vector< std::vector<GLuint> > faces;
    generate_faces(faces, file);

    file.close();
    file.clear();

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    GLfloat time = 0.0f;

    GLfloat r=0.0f,g=0.0f,b=0.0f;
    glClearColor(r, g, b, 1);
    glClearDepth(1);

    int w, h;

    do
    {
        time +=0.01f;

        glfwGetWindowSize(window, &w, &h);

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rotVal+=.005f;
        rotationMatrix = glm::rotate(glm::mat4(1.0f), rotVal, glm::vec3(rotX, rotY, rotZ));

        glUniformMatrix4fv(glGetUniformLocation(shader, "rotMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader, "transMatrix"), 1, GL_FALSE, glm::value_ptr(transMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader, "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
        glUniform1f(glGetUniformLocation(shader, "time"), time);

        glUniform1f(glGetUniformLocation(shader, "fog_r"), r);
        glUniform1f(glGetUniformLocation(shader, "fog_g"), g);
        glUniform1f(glGetUniformLocation(shader, "fog_b"), b);

        for(auto &&i : faces)
            draw_face(i);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
    glfwWindowShouldClose(window) == 0 );

    return 0;
}
