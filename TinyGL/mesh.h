#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "bufferobject.h"

class Mesh
{
public:
  glm::mat4 m_modelMatrix;
  glm::mat4 m_normalMatrix;

  Mesh();
  ~Mesh();

  void attachBuffer(BufferObject* buff);
  void draw();

  inline void setDrawCb(void(*drawCb)())
  {
    m_drawCb = drawCb;
  }

  inline void bind()
  {
    glBindVertexArray(m_vao);
  }
  
protected:
  std::vector<BufferObject*> m_buffers;
  void(*m_drawCb)();

  GLuint m_vao;
};

Mesh* createGridMesh(int nx, int ny);
Mesh* createSphereMesh(int nx, int ny);

#endif // MESH_H