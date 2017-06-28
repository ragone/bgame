#include "model_loader.hpp"
#include <glm/glm.hpp>
#include <boost/container/flat_map.hpp>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#include <GL/glu.h>
#endif
#include <iostream>

namespace gl {

    boost::container::flat_map<int, std::unique_ptr<model_t>> models;

    // Shamelessly based on: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
    std::unique_ptr<model_t> load_model(const std::string &filename) {
        std::unique_ptr<model_t> result;

        std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
        std::vector<glm::vec3> temp_vertices;
        std::vector<glm::vec2> temp_uvs;
        std::vector<glm::vec3> temp_normals;

        FILE *file = fopen(filename.c_str(), "r");
        if (file == NULL) {
            printf("Impossible to open the file !\n");
            return result;
        }

        while (1) {
            char lineHeader[128];
            // read the first word of the line
            int res = fscanf(file, "%s", lineHeader);
            if (res == EOF)
                break; // EOF = End Of File. Quit the loop.

            // else : parse lineHeader
            if (strcmp(lineHeader, "v") == 0) {
                glm::vec3 vertex;
                fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
                temp_vertices.push_back(vertex);


            } else if (strcmp(lineHeader, "vt") == 0) {
                glm::vec2 uv;
                fscanf(file, "%f %f\n", &uv.x, &uv.y);
                temp_uvs.push_back(uv);
            } else if (strcmp(lineHeader, "vn") == 0) {
                glm::vec3 normal;
                fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
                temp_normals.push_back(normal);


            } else if (strcmp(lineHeader, "f") == 0) {
                std::string vertex1, vertex2, vertex3;
                unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
                int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0],
                                     &normalIndex[0],
                                     &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2],
                                     &normalIndex[2]);
                if (matches != 9) {
                    printf("File can't be read by our simple parser : ( Try exporting with other options\n");
                    return result;
                }
                vertexIndices.push_back(vertexIndex[0]);
                vertexIndices.push_back(vertexIndex[1]);
                vertexIndices.push_back(vertexIndex[2]);
                uvIndices.push_back(uvIndex[0]);
                uvIndices.push_back(uvIndex[1]);
                uvIndices.push_back(uvIndex[2]);
                normalIndices.push_back(normalIndex[0]);
                normalIndices.push_back(normalIndex[1]);
                normalIndices.push_back(normalIndex[2]);

            }
        }

        result = std::make_unique<model_t>();

        // For each vertex of each triangle
        for (unsigned int i = 0; i < vertexIndices.size(); i++) {
            unsigned int vertexIndex = vertexIndices[i];
            glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
            glm::vec3 normal = glm::normalize(temp_normals[ vertexIndex-1 ]);
            glm::vec2 uv = temp_uvs[ vertexIndex-1 ];

            result->items.emplace_back(vertex.x);
            result->items.emplace_back(vertex.y);
            result->items.emplace_back(vertex.z);
            result->items.emplace_back(normal.x);
            result->items.emplace_back(normal.y);
            result->items.emplace_back(normal.z);
            result->items.emplace_back(uv.x);
            result->items.emplace_back(uv.y);
        }

        glGenBuffers(1, &result->vbo_id); // Generate the VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result->vbo_id);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLfloat) * result->items.size(),
                     &result->items[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        std::cout << "VBO " << result->vbo_id << " bound to model " << filename << "\n";

        fclose(file);
        return result;
    }

    void setup_model(const int &idx, const std::string &filename) {
        models[idx] = std::move(load_model(filename));
    }

    model_t * get_model(const int &idx) {
        return models[idx].get();
    }
}