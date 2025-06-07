#pragma once

#include "core/resources.h"
#include "image/image.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"

#include "../src-ui/gl.h"
#include <GL/gl.h>

#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

static GLfloat g_vertex_buffer_data[] = {
    1.0f,  0.0f,  0.0f, //
    -1.0f, 0.0f,  0.0f, //
    0.0f,  1.0f,  0.0f, //

    -1.0f, 0.0f,  0.0f, //
    1.0f,  0.0f,  0.0f, //
    0.0f,  -1.0f, 0.0f, //
};

// Two UV coordinatesfor each vertex. They were created with Blender.
static GLfloat g_uv_buffer_data[] = {
    1,  1, //
    0,  0, //
    0,  1, //

    -1, -1, //
    0,  0,  //
    0,  -1, //
};

class OpenGLScene
{
private:
    GLuint program;

    GLuint VertexArrayID;
    GLuint vertexbuffer;

    GLuint uvbuffer;

    // STUFF
    GLuint FramebufferName = 0;
    GLuint renderedTexture;
    GLuint depthrenderbuffer;

    GLuint MatrixID;

    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 MVP;

    GLuint TextureID;
    GLuint Texture;

    std::vector<float> vertices_buf;
    std::vector<float> textpos_buf;

    const int render_width = 1024 * 4;
    const int render_height = 768 * 4;

    void addPoint(float x, float y, float z, float tex_x, float tex_y)
    {
        vertices_buf.push_back(x);
        vertices_buf.push_back(y);
        vertices_buf.push_back(z);

        textpos_buf.push_back(tex_x);
        textpos_buf.push_back(tex_y);
    }

    void addRect(float c1_x, float c1_y, float c1_z, //
                 float c2_x, float c2_y, float c2_z, //
                 float c3_x, float c3_y, float c3_z, //
                 float c4_x, float c4_y, float c4_z, //
                 float t1_x, float t1_y,             //
                 float t2_x, float t2_y,             //
                 float t3_x, float t3_y,             //
                 float t4_x, float t4_y)
    {
        addPoint(c1_x, c1_y, c1_z, t1_x, t1_y);
        addPoint(c2_x, c2_y, c2_z, t2_x, t2_y);
        addPoint(c4_x, c4_y, c4_z, t4_x, t4_y);

        addPoint(c2_x, c2_y, c2_z, t2_x, t2_y);
        addPoint(c3_x, c3_y, c3_z, t3_x, t3_y);
        addPoint(c4_x, c4_y, c4_z, t4_x, t4_y);
    }

public:
    OpenGLScene()
    {
        // Original
        {
            float latc = 500;
            float lonc = latc * 2; // 100;
            float radius = 5;

            float lat_delta_angle = (M_PI) / latc;
            float lon_delta_angle = (M_PI * 2) / lonc;

            float lat_text_delta = 1.0f / latc;
            float lon_text_delta = 1.0f / lonc;

            for (float latn = 0; latn < latc; latn++) // The poles are made from triangles... Skip them
            {
                for (float lonn = 0; lonn < lonc; lonn++)
                {
                    float lat = (latn - (latc / 2)) * lat_delta_angle;
                    float lon = lonn * lon_delta_angle;

                    float x1 = radius * cos(lat) * cos(lon);
                    float y1 = radius * cos(lat) * sin(lon);
                    float z1 = radius * sin(lat);

                    lat += lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x2 = radius * cos(lat) * cos(lon);
                    float y2 = radius * cos(lat) * sin(lon);
                    float z2 = radius * sin(lat);

                    // lat += lat_delta_angle;
                    lon += lon_delta_angle;

                    float x3 = radius * cos(lat) * cos(lon);
                    float y3 = radius * cos(lat) * sin(lon);
                    float z3 = radius * sin(lat);

                    lat -= lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x4 = radius * cos(lat) * cos(lon);
                    float y4 = radius * cos(lat) * sin(lon);
                    float z4 = radius * sin(lat);

                    float tex_x = lonn / lonc;
                    float tex_y = latn / latc;

                    // std::cout << x1 << " " << y1 << " " << z1 << " " << x2 << " " << y2 << " " << z2 << std::endl;

                    addRect(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4,

                            tex_x, tex_y, tex_x, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y);
                }
            }

            /*addRect(-1, -1, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, -1, 0,
                    1, 1,
                    1, 0,
                    0, 0,
                    0, 1);*/

            // float r = 0.5;
            // for (float val = 0; val = M_PI * 2; val++)
            //{
            // }

            /*
            float x = -1.2;

            float chunks = 50;
            float delta_angle = (M_PI * 2) / chunks;

            float delta_text = 1.0 / chunks;

            for (float i = 0; i < M_PI * 2; i += delta_angle)
            {
                float tx = 1.0f * (i / (M_PI * 2));

                float angle1 = i;
                float x1 = 0.5 * sin(angle1);
                float z1 = 0.5 * cos(angle1);

                float angle2 = i + delta_angle;
                float x2 = 0.5 * sin(angle2);
                float z2 = 0.5 * cos(angle2);

                addRect(x1, -1, z1,
                        x1, 0, z1,
                        x2, 0, z2,
                        x2, -1, z2,

                        tx, 0,
                        tx, 1,
                        tx + delta_text, 1,
                        tx + delta_text, 0);

                //x += 0.6;
            }*/

            /*
            addRect(-1, -1, 0,
                    -1, 0, 0,
                    1, 0, 0,
                    1, -1, 0,

                    0, 0,
                    0, 0.5,
                    1, 0.5,
                    1, 0);

            addRect(-1, 0, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, 0, 0,

                    0, 0.5,
                    0, 1,
                    1, 1,
                    1, 0.5);
            */
        }

        // Test 2
        {
            float latc = 500;
            float lonc = latc * 2; // 100;
            float radius = 0.5;

            float lat_delta_angle = (M_PI) / latc;
            float lon_delta_angle = (M_PI * 2) / lonc;

            float lat_text_delta = 1.0f / latc;
            float lon_text_delta = 1.0f / lonc;

            double pos_offset_x = -10;
            double pos_offset_y = 0;
            double pos_offset_z = 0;

            for (float latn = 0; latn < latc; latn++) // The poles are made from triangles... Skip them
            {
                for (float lonn = 0; lonn < lonc; lonn++)
                {
                    float lat = (latn - (latc / 2)) * lat_delta_angle;
                    float lon = lonn * lon_delta_angle;

                    float x1 = radius * cos(lat) * cos(lon);
                    float y1 = radius * cos(lat) * sin(lon);
                    float z1 = radius * sin(lat);

                    lat += lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x2 = radius * cos(lat) * cos(lon);
                    float y2 = radius * cos(lat) * sin(lon);
                    float z2 = radius * sin(lat);

                    // lat += lat_delta_angle;
                    lon += lon_delta_angle;

                    float x3 = radius * cos(lat) * cos(lon);
                    float y3 = radius * cos(lat) * sin(lon);
                    float z3 = radius * sin(lat);

                    lat -= lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x4 = radius * cos(lat) * cos(lon);
                    float y4 = radius * cos(lat) * sin(lon);
                    float z4 = radius * sin(lat);

                    float tex_x = lonn / lonc;
                    float tex_y = latn / latc;

                    // std::cout << x1 << " " << y1 << " " << z1 << " " << x2 << " " << y2 << " " << z2 << std::endl;

                    addRect(x1 + pos_offset_x, y1 + pos_offset_y, z1 + pos_offset_z, x2 + pos_offset_x, y2 + pos_offset_y, z2 + pos_offset_z, x3 + pos_offset_x, y3 + pos_offset_y, z3 + pos_offset_z,
                            x4 + pos_offset_x, y4 + pos_offset_y, z4 + pos_offset_z,

                            tex_x, tex_y, tex_x, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y);
                }
            }

            /*addRect(-1, -1, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, -1, 0,
                    1, 1,
                    1, 0,
                    0, 0,
                    0, 1);*/

            // float r = 0.5;
            // for (float val = 0; val = M_PI * 2; val++)
            //{
            // }

            /*
            float x = -1.2;

            float chunks = 50;
            float delta_angle = (M_PI * 2) / chunks;

            float delta_text = 1.0 / chunks;

            for (float i = 0; i < M_PI * 2; i += delta_angle)
            {
                float tx = 1.0f * (i / (M_PI * 2));

                float angle1 = i;
                float x1 = 0.5 * sin(angle1);
                float z1 = 0.5 * cos(angle1);

                float angle2 = i + delta_angle;
                float x2 = 0.5 * sin(angle2);
                float z2 = 0.5 * cos(angle2);

                addRect(x1, -1, z1,
                        x1, 0, z1,
                        x2, 0, z2,
                        x2, -1, z2,

                        tx, 0,
                        tx, 1,
                        tx + delta_text, 1,
                        tx + delta_text, 0);

                //x += 0.6;
            }*/

            /*
            addRect(-1, -1, 0,
                    -1, 0, 0,
                    1, 0, 0,
                    1, -1, 0,

                    0, 0,
                    0, 0.5,
                    1, 0.5,
                    1, 0);

            addRect(-1, 0, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, 0, 0,

                    0, 0.5,
                    0, 1,
                    1, 1,
                    1, 0.5);
            */
        }

        // Test 3
        {
            float latc = 500;
            float lonc = latc * 2; // 100;
            float radius = 0.5;

            float lat_delta_angle = (M_PI) / latc;
            float lon_delta_angle = (M_PI * 2) / lonc;

            float lat_text_delta = 1.0f / latc;
            float lon_text_delta = 1.0f / lonc;

            double pos_offset_x = 0;
            double pos_offset_y = 10;
            double pos_offset_z = 0;

            for (float latn = 0; latn < latc; latn++) // The poles are made from triangles... Skip them
            {
                for (float lonn = 0; lonn < lonc; lonn++)
                {
                    float lat = (latn - (latc / 2)) * lat_delta_angle;
                    float lon = lonn * lon_delta_angle;

                    float x1 = radius * cos(lat) * cos(lon);
                    float y1 = radius * cos(lat) * sin(lon);
                    float z1 = radius * sin(lat);

                    lat += lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x2 = radius * cos(lat) * cos(lon);
                    float y2 = radius * cos(lat) * sin(lon);
                    float z2 = radius * sin(lat);

                    // lat += lat_delta_angle;
                    lon += lon_delta_angle;

                    float x3 = radius * cos(lat) * cos(lon);
                    float y3 = radius * cos(lat) * sin(lon);
                    float z3 = radius * sin(lat);

                    lat -= lat_delta_angle;
                    // lon += lon_delta_angle;

                    float x4 = radius * cos(lat) * cos(lon);
                    float y4 = radius * cos(lat) * sin(lon);
                    float z4 = radius * sin(lat);

                    float tex_x = lonn / lonc;
                    float tex_y = latn / latc;

                    // std::cout << x1 << " " << y1 << " " << z1 << " " << x2 << " " << y2 << " " << z2 << std::endl;

                    addRect(x1 + pos_offset_x, y1 + pos_offset_y, z1 + pos_offset_z, x2 + pos_offset_x, y2 + pos_offset_y, z2 + pos_offset_z, x3 + pos_offset_x, y3 + pos_offset_y, z3 + pos_offset_z,
                            x4 + pos_offset_x, y4 + pos_offset_y, z4 + pos_offset_z,

                            tex_x, tex_y, tex_x, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y + lat_text_delta, tex_x + lon_text_delta, tex_y);
                }
            }

            /*addRect(-1, -1, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, -1, 0,
                    1, 1,
                    1, 0,
                    0, 0,
                    0, 1);*/

            // float r = 0.5;
            // for (float val = 0; val = M_PI * 2; val++)
            //{
            // }

            /*
            float x = -1.2;

            float chunks = 50;
            float delta_angle = (M_PI * 2) / chunks;

            float delta_text = 1.0 / chunks;

            for (float i = 0; i < M_PI * 2; i += delta_angle)
            {
                float tx = 1.0f * (i / (M_PI * 2));

                float angle1 = i;
                float x1 = 0.5 * sin(angle1);
                float z1 = 0.5 * cos(angle1);

                float angle2 = i + delta_angle;
                float x2 = 0.5 * sin(angle2);
                float z2 = 0.5 * cos(angle2);

                addRect(x1, -1, z1,
                        x1, 0, z1,
                        x2, 0, z2,
                        x2, -1, z2,

                        tx, 0,
                        tx, 1,
                        tx + delta_text, 1,
                        tx + delta_text, 0);

                //x += 0.6;
            }*/

            /*
            addRect(-1, -1, 0,
                    -1, 0, 0,
                    1, 0, 0,
                    1, -1, 0,

                    0, 0,
                    0, 0.5,
                    1, 0.5,
                    1, 0);

            addRect(-1, 0, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, 0, 0,

                    0, 0.5,
                    0, 1,
                    1, 1,
                    1, 0.5);
            */
        }

        image::Image map_image;
        // map_image.load_jpeg(resources::getResourcePath("maps/nasa.jpg"));
        // map_image.load_jpeg("/home/alan/Downloads/land_ocean_ice_cloud_2048.jpg");
        image::load_img(map_image, "resources/maps/nasa_hd.jpg"); // "/home/alan/Downloads/world.200408.3x21600x10800.png");
        // map_image.load_png("/home/alan/Downloads/projection_ews_test.png");
        // map_image.load_jpeg("/home/alan/Downloads/dnb_land_ocean_ice.2012.54000x27000_geo.jpg");
        // map_image.load_png("/home/alan/Downloads/projection_metop.png");
        Texture = makeImageTexture();
        uint32_t *map_texture_buff = new uint32_t[map_image.width() * map_image.height()];
        image::image_to_rgba(map_image, map_texture_buff);
        // uchar_to_rgba(map_image.raw_data(), map_texture_buff, map_image.width() * map_image.height(), 3);
        updateImageTexture(Texture, map_texture_buff, map_image.width(), map_image.height());
        delete[] map_texture_buff;

        // Use our shader
        // glUseProgram(program);

        glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices_buf.size(), vertices_buf.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * textpos_buf.size(), textpos_buf.data(), GL_STATIC_DRAW);

        // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
        glGenFramebuffers(1, &FramebufferName);
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

        // The texture we're going to render to
        glGenTextures(1, &renderedTexture);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, renderedTexture);

        // Give an empty image to OpenGL ( the last "0" )
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, render_width, render_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        // Poor filtering. Needed !
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // The depth buffer
        glGenRenderbuffers(1, &depthrenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, render_width, render_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

        // Set "renderedTexture" as our colour attachement #0
        // glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

        // Always check that our framebuffer is ok
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            logger->error("ERROR");

        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        std::string vertex = "#version 330 core\n"
                             "layout(location = 0) in vec3 vertexPosition_modelspace;"
                             "layout(location = 1) in vec2 vertexUV;"
                             "out vec2 UV;"
                             "uniform mat4 MVP;"
                             "void main(){"
                             "gl_Position =  MVP * vec4(vertexPosition_modelspace,1);"
                             //"gl_Position.xyz.y = 1.0 - gl_Position.xyz.y;"
                             "UV = vertexUV;"
                             "}";

        std::string fragment = "#version 330 core\n"
                               "in vec2 UV;"
                               "out vec3 color;"
                               "uniform sampler2D myTextureSampler;"
                               "void main(){"
                               "color = texture2D( myTextureSampler, UV ).rgb;"
                               "}";

        program = compileShaders(vertex, fragment);

        MatrixID = glGetUniformLocation(program, "MVP");
        TextureID = glGetUniformLocation(program, "myTextureSampler");

        Projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        // Camera matrix
        View = glm::lookAt(glm::vec3(0, 0, 2), // Camera is at (4,3,3), in World Space
                           glm::vec3(0, 0, 0), // and looks at the origin
                           glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );
        // Model matrix : an identity matrix (model will be at the origin)
        Model = glm::mat4(1.0f);
        // glm::vec3 vv(1, 0, 0);
        Model = glm::rotate<float>(Model, (M_PI / 2) * 3, glm::vec3(1, 0, 0)); // where x, y, z is axis of rotation (e.g. 0 1 0)
        Model = glm::rotate<float>(Model, (M_PI / 2) * 1, glm::vec3(0, 0, 1));

        // Enable depth test
        // glEnable(GL_DEPTH_TEST);
        // Accept fragment if it closer to the camera than the former one
        // glDepthFunc(GL_LESS);
    }
    // int t = 0;
    // float view_x = 4;
    // float view_y = 3;
    // float view_z = 3;

    // Initial position : on +Z
    glm::vec3 position = glm::vec3(0, 0, 5);
    // Initial horizontal angle : toward -Z
    float horizontalAngle = 3.14f;
    // Initial vertical angle : none
    float verticalAngle = 0.0f;
    // Initial Field of View
    float initialFoV = 45.0f;

    float speed = 3.f; // 3 units / second
    float mouseSpeed = 0.001f;

    GLuint draw(GLFWwindow *window, int render_widthf, int render_heightf)
    {
        Projection = glm::perspective(glm::radians(45.0f), float(render_widthf) / float(render_heightf), 0.1f, 100.0f); // Update aspect ratio
        // if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        //     view_z += 0.1;
        // if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        //     view_z -= 0.1;

        // if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        //     view_x += 0.1;
        // if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        //     view_x -= 0.1;

        // if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        //     view_y += 0.1;
        // if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        //     view_y -= 0.1;

        {
            static double lastTime = glfwGetTime();

            // Compute time difference between current and last frame
            double currentTime = glfwGetTime();
            float deltaTime = float(currentTime - lastTime);

            // // Get mouse position
            // double xpos, ypos;
            // glfwGetCursorPos(window, &xpos, &ypos);

            // Reset mouse position for next frame
            //   glfwSetCursorPos(window, 1024 / 2, 768 / 2);

            // // Compute new orientation
            // horizontalAngle += mouseSpeed * float(1024 / 2 - xpos);
            // verticalAngle += mouseSpeed * float(768 / 2 - ypos);

            // auto mousedrag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

            // Compute new orientation
            // horizontalAngle += mouseSpeed * mousedrag.x;
            // verticalAngle += mouseSpeed * mousedrag.y;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                verticalAngle += mouseSpeed * -10;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                verticalAngle += mouseSpeed * 10;

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                horizontalAngle += mouseSpeed * -10;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                horizontalAngle += mouseSpeed * 10;

            if (verticalAngle > M_PI / 2)
                verticalAngle = M_PI / 2;
            else if (verticalAngle < -M_PI / 2)
                verticalAngle = -M_PI / 2;

#if 1
            // Direction : Spherical coordinates to Cartesian coordinates conversion
            glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));

            auto dir2 = direction;
            dir2.y = 0;

            // Right vector
            glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));

            // Up vector
            glm::vec3 up = glm::cross(right, direction);

            // Move forward
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                position.y -= deltaTime * speed;
            }
            // Move backward
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            {
                position.y += deltaTime * speed;
            }
            // Move up
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            {
                position += dir2 * deltaTime * speed;
            }
            // Move down
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            {
                position -= dir2 * deltaTime * speed;
            }
            // Strafe right
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            {
                position += right * deltaTime * speed;
            }
            // Strafe left
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            {
                position -= right * deltaTime * speed;
            }

            // Projection = glm::perspective(glm::radians(45), 4.0f / 3.0f, 0.1f, 100.0f);

            View = glm::lookAt(position,             // Camera is here
                               position + direction, // and looks here : at the same position, plus "direction"
                               up                    // Head is up (set to 0,-1,0 to look upside-down)
            );
#else
            // Direction : Spherical coordinates to Cartesian coordinates conversion
            glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));

            // Right vector
            glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));

            // Up vector
            glm::vec3 up = glm::cross(right, direction);

            // Move forward
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                position += direction * deltaTime * speed;
            }
            // Move backward
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            {
                position -= direction * deltaTime * speed;
            }
            // Move up
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            {
                position += up * deltaTime * speed;
            }
            // Move down
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            {
                position -= up * deltaTime * speed;
            }
            // Strafe right
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            {
                position += right * deltaTime * speed;
            }
            // Strafe left
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            {
                position -= right * deltaTime * speed;
            }

            // Projection = glm::perspective(glm::radians(45), 4.0f / 3.0f, 0.1f, 100.0f);

            View = glm::lookAt(position,             // Camera is here
                               position + direction, // and looks here : at the same position, plus "direction"
                               up                    // Head is up (set to 0,-1,0 to look upside-down)
            );
#endif

            // For the next frame, the "last time" will be "now"
            lastTime = currentTime;
        }

        // View = glm::lookAt(
        //     glm::vec3(view_x, view_y, view_z), // Camera is at (4,3,3), in World Space
        //     glm::vec3(0, 0, 0),                // and looks at the origin
        //     glm::vec3(0, 1, 0)                 // Head is up (set to 0,-1,0 to look upside-down)
        //);

        // Render to our framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
        glViewport(0, 0, render_width, render_height); // Render on the whole framebuffer, complete from the lower left corner to the upper right

        MVP = Projection * View * Model;

        // Dark blue background
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        // Accept fragment if it closer to the camera than the former one
        glDepthFunc(GL_LESS);

        // Use our shader
        glUseProgram(program);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
                              3,        // size
                              GL_FLOAT, // type
                              GL_FALSE, // normalized?
                              0,        // stride
                              (void *)0 // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1,        // attribute. No particular reason for 1, but must match the layout in the shader.
                              2,        // size : U+V => 2
                              GL_FLOAT, // type
                              GL_FALSE, // normalized?
                              0,        // stride
                              (void *)0 // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, vertices_buf.size()); // 3 indices starting at 0 -> 1 triangle

        glDisableVertexAttribArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return renderedTexture;
    }

    GLuint compileShaders(std::string vertex_src, std::string fragment_src)
    {
        GLint Result;
        GLint InfoLogLength;

        // Create the shaders
        GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

        // Compile Vertex Shader
        const char *vsrc = vertex_src.c_str();
        glShaderSource(VertexShaderID, 1, &vsrc, NULL);
        glCompileShader(VertexShaderID);

        // Check Vertex Shader
        glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0)
        {
            std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
            printf("%s\n", &VertexShaderErrorMessage[0]);
        }

        // Compile Fragment Shader
        const char *fsrc = fragment_src.c_str();
        glShaderSource(FragmentShaderID, 1, &fsrc, NULL);
        glCompileShader(FragmentShaderID);

        // Check Fragment Shader
        glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0)
        {
            std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
            printf("%s\n", &FragmentShaderErrorMessage[0]);
        }

        // Link the program
        printf("Linking program\n");
        GLuint ProgramID = glCreateProgram();
        glAttachShader(ProgramID, VertexShaderID);
        glAttachShader(ProgramID, FragmentShaderID);
        glLinkProgram(ProgramID);

        // Check the program
        glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
        glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0)
        {
            std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
            glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
            printf("%s\n", &ProgramErrorMessage[0]);
        }

        glDetachShader(ProgramID, VertexShaderID);
        glDetachShader(ProgramID, FragmentShaderID);

        glDeleteShader(VertexShaderID);
        glDeleteShader(FragmentShaderID);

        printf("Done\n");

        return ProgramID;
    }
};