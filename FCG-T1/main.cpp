#include "config.h"
#include "tinygl.h"
#include "logger.h"
#include "shader.h"
#include "mesh.h"
#include "grid.h"
#include "sphere.h"
#include "axis.h"
#include "ciemesh.h"
#include "ciepointcloud.h"
#include "colorspace.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

extern "C" {
#include "color.h"
#include "mtwist.h"
}

using namespace std;

void init();
void initGLUT(int argc, char** argv);
void initGLEW();
void destroy();
void update();
void draw();
void reshape(int w, int h);
void keyPress(unsigned char c, int x, int y);
void specialKeyPress(int c, int x, int y);
void exit_cb();

typedef enum { XYZ, RGB, n_colorspaces } colorspaces;

int g_window = -1;

glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::vec3 g_eye;
glm::vec3 g_center;

bool initCalled = false;
bool initGLEWCalled = false;

bool g_wireRender = false;
bool g_meshRender = false;
int g_cloudRender = XYZ;

void drawPointsArrays(size_t num_points)
{
  glDrawArrays(GL_POINTS, 0, num_points);
}

void drawLinesIdx(size_t num_points)
{
  glDrawElements(GL_TRIANGLES, num_points, GL_UNSIGNED_SHORT, NULL);
}

void drawAxis(size_t num_points)
{
  glDrawArrays(GL_LINES, 0, num_points);
}

int main(int argc, char** argv)
{
  Logger::getInstance()->setLogStream(&cout);
  Logger::getInstance()->log(TINYGL_LIBNAME + string(" v") + to_string(TINYGL_MAJOR_VERSION) + "." + to_string(TINYGL_MINOR_VERSION));

  initGLUT(argc, argv);
  initGLEW();
  init();

  glutMainLoop();
  return 0;
}

void initGLUT(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowSize(800, 600);

  g_window = glutCreateWindow("FCG-T1");
  glutReshapeFunc(reshape);
  glutDisplayFunc(update);
  glutKeyboardFunc(keyPress);
  glutSpecialFunc(specialKeyPress);

  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutCloseFunc(exit_cb);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
}

void initGLEW()
{
  GLenum err = glewInit();

  if (err != GLEW_OK) {
    Logger::getInstance()->error("Failed to initialize GLEW");
    std::cerr << glewGetErrorString(err) << std::endl;
    exit(1);
  }

  glClearColor(0.8f, 0.8f, 0.8f, 1.f);
  glEnable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPointSize(2);

  initGLEWCalled = true;
}

void init()
{
  const int STEP = 1;
  g_eye = glm::vec3(2.5, 2.5, 2.5);
  g_center = glm::vec3(0, 0, 0);
  viewMatrix = glm::lookAt(g_eye, g_center, glm::vec3(0, 1, 0));
  projMatrix = glm::perspective(static_cast<float>(M_PI / 4.f), 1.f, 0.1f, 1000.f);

  std::vector<glm::vec3> xyz;
  std::vector<glm::vec3> xyz_mesh;
  std::vector<glm::vec3> rgb;
  std::vector<glm::vec3> rgb_mesh;

  std::vector<CIEPointCloud*> cieclouds(n_colorspaces);
  std::vector<CIEMesh*> ciemesh(n_colorspaces);
  Axis* axis;

  float* beta = new float[15000 * 400];
  float* illum = new float[400];
  std::vector<glm::vec3> xyzbar;

  for (int i = 0; i < 400; i++)  {
    float x, y, z;
    illum[i] = corGetD65(380.f + i);
    corGetCIExyz(380.f + i, &x, &y, &z);
    xyzbar.push_back(glm::vec3(x, y, z));
  }

  glm::mat3 m = glm::inverse(glm::mat3({ 0.490, 0.310, 0.200, 0.177, 0.813, 0.011, 0.000, 0.010, 0.990 }));

  FILE* fp = fopen("beta_reflectance_15000_1.dat", "rb");
  fread(beta, sizeof(float), 15000 * 400, fp);

  for (size_t i = 0; i < 15000; i++) {
    glm::vec3 tmp = createCIEXYZ(beta + i * 400, illum, xyzbar, STEP);
    xyz.push_back(tmp);
    rgb.push_back(m * tmp);
  }

  delete[] beta;

  //variar o Y de 0 at� 1
  //calcular a curva beta  de forma a montar a superf�cie do plano
  //feito isso, interpolar os pontos gerados e formar uma malha de triangulos.
  const float Y_STEP = 0.1f;   

  /*float sum = tmp.x + tmp.y + tmp.z;
  tmp = tmp * (1 / sum) * 0.3f;*/
  for (float Y = Y_STEP; Y < 1.f; Y += Y_STEP) {
    float sum = 0.f;
    glm::vec3 ciexyz = createCIEXYZPureSource(illum, xyzbar, STEP);
    sum = ciexyz.x + ciexyz.y + ciexyz.z;

    ciexyz.x /= sum;
    ciexyz.y /= sum;
    ciexyz.z /= sum;

    xyz_mesh.push_back(ciexyz);
    rgb_mesh.push_back(m * ciexyz);
  }

  //xyz_mesh.push_back(glm::vec3(lw[0], lw[1], lw[2]));
  //rgb_mesh.push_back(m * glm::vec3(lw[0], lw[1], lw[2]));

  delete[] illum;  
  
  cieclouds[XYZ] = new CIEPointCloud(xyz);
  cieclouds[XYZ]->setDrawCb(drawPointsArrays);
  cieclouds[XYZ]->setMaterialColor(glm::vec4(0));
  TinyGL::getInstance()->addMesh("CIExyzCloud", cieclouds[XYZ]);
  cieclouds[XYZ]->m_modelMatrix = glm::mat4(1.f);
  cieclouds[XYZ]->m_normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * cieclouds[XYZ]->m_modelMatrix));

  cieclouds[RGB] = new CIEPointCloud(rgb);
  cieclouds[RGB]->setDrawCb(drawPointsArrays);
  cieclouds[RGB]->setMaterialColor(glm::vec4(0));
  TinyGL::getInstance()->addMesh("CIErgbCloud", cieclouds[RGB]);
  cieclouds[RGB]->m_modelMatrix = glm::mat4(glm::inverse(m));
  cieclouds[RGB]->m_normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * cieclouds[RGB]->m_modelMatrix));

  ciemesh[XYZ] = new CIEMesh(xyz_mesh);
  ciemesh[XYZ]->setDrawCb(drawPointsArrays);
  ciemesh[XYZ]->setMaterialColor(glm::vec4(0));
  TinyGL::getInstance()->addMesh("CIExyzMesh", ciemesh[XYZ]);
  ciemesh[XYZ]->m_modelMatrix = glm::mat4(1.f);
  ciemesh[XYZ]->m_normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * ciemesh[XYZ]->m_modelMatrix));

  ciemesh[RGB] = new CIEMesh(rgb_mesh);
  ciemesh[RGB]->setDrawCb(drawLinesIdx);
  ciemesh[RGB]->setMaterialColor(glm::vec4(0));
  TinyGL::getInstance()->addMesh("CIErgbMesh", ciemesh[RGB]);
  ciemesh[RGB]->m_modelMatrix = glm::mat4(glm::inverse(m));
  ciemesh[RGB]->m_normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * ciemesh[RGB]->m_modelMatrix));

  axis = new Axis(glm::vec2(-1, 1), glm::vec2(-1, 1), glm::vec2(-1, 1));
  axis->setDrawCb(drawAxis);
  axis->setMaterialColor(glm::vec4(0.f));
  TinyGL::getInstance()->addMesh("axis", axis);
  axis->m_modelMatrix = glm::mat4(1.f);
  axis->m_normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * axis->m_modelMatrix));

  Shader* g_shader = new Shader("../Resources/fcgt1.vs", "../Resources/fcgt1.fs");
  g_shader->bind();
  g_shader->bindFragDataLoc("out_vColor", 0);
  g_shader->setUniformMatrix("viewMatrix", viewMatrix);
  g_shader->setUniformMatrix("projMatrix", projMatrix);

  TinyGL::getInstance()->addShader("fcgt1", g_shader);

  initCalled = true;
}

void destroy()
{
  TinyGL::getInstance()->freeResources();
}

void update()
{
  if (!initCalled || !initGLEWCalled)
    return;

  draw();
}

void draw()
{
  if (!initCalled || !initGLEWCalled)
    return;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  TinyGL* glPtr = TinyGL::getInstance();
  Shader* s = glPtr->getShader("fcgt1");

  s->bind();
  
  switch (g_cloudRender) {
  case XYZ:
    s->setUniformMatrix("modelMatrix", glPtr->getMesh("CIExyzCloud")->m_modelMatrix);
    //glPtr->draw("CIExyzCloud");
    if (g_meshRender) {
      s->setUniformMatrix("modelMatrix", glPtr->getMesh("CIExyzMesh")->m_modelMatrix);
      glPtr->draw("CIExyzMesh");
    }
    break;
  case RGB:
    s->setUniformMatrix("modelMatrix", glPtr->getMesh("CIErgbCloud")->m_modelMatrix);
    //glPtr->draw("CIErgbCloud");
    if (g_meshRender) {
      s->setUniformMatrix("modelMatrix", glPtr->getMesh("CIErgbMesh")->m_modelMatrix); 
      glPtr->draw("CIErgbMesh");
    }
    break;
  }

  s->setUniformMatrix("modelMatrix", glPtr->getMesh("axis")->m_modelMatrix);
  glPtr->draw("axis");

  glBindVertexArray(0);
  Shader::unbind();

  glutSwapBuffers();
  glutPostRedisplay();
}

void reshape(int w, int h)
{
  if (!initCalled || !initGLEWCalled)
    return;

  glViewport(0, 0, w, h);
  projMatrix = glm::perspective(static_cast<float>(M_PI / 4.f), static_cast<float>(w) / static_cast<float>(h), 0.1f, 1000.f);

  Shader* s = TinyGL::getInstance()->getShader("fcgt1");
  s->bind();
  s->setUniformMatrix("projMatrix", projMatrix);

  Shader::unbind();
}

void keyPress(unsigned char c, int x, int y)
{
  bool cameraChanged = false;
//  printf("keyPress = %d\n", c);

  switch (c) {
  case 'w':
    g_wireRender = !g_wireRender;
    break;
  case 'm':
    g_meshRender = !g_meshRender;
    break;
  }

  if(g_wireRender)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if(cameraChanged) {
    viewMatrix = glm::lookAt(g_eye, g_center, glm::vec3(0, 1, 0));

    Shader* s = TinyGL::getInstance()->getShader("fcgt1");
    s->bind();
    s->setUniformMatrix("viewMatrix", viewMatrix);
  }
}

void specialKeyPress(int c, int x, int y)
{
  bool cameraChanged = false;
//  printf("specialKeyPress = %d\n", c);

  glm::vec3 back = g_eye - g_center;

  switch (c) {
  case GLUT_KEY_LEFT:
    back = glm::mat3(glm::rotate(-(float)M_PI / 100.f, glm::vec3(0, 1, 0))) * back;
    g_eye = back + g_center;
    cameraChanged = true;
    break;
  case GLUT_KEY_RIGHT:
    back = glm::mat3(glm::rotate((float)M_PI / 100.f, glm::vec3(0, 1, 0))) * back;
    g_eye = back + g_center;
    cameraChanged = true;
    break;
  case GLUT_KEY_UP:
    back = glm::mat3(glm::rotate(-(float)M_PI / 100.f, glm::vec3(1, 0, 0))) * back;
    g_eye = back + g_center;
    cameraChanged = true;
    break;
  case GLUT_KEY_DOWN:
    back = glm::mat3(glm::rotate((float)M_PI / 100.f, glm::vec3(1, 0, 0))) * back;
    g_eye = back + g_center;
    cameraChanged = true;
    break;
  case GLUT_KEY_F1:
    g_cloudRender = XYZ;
    break;
  case GLUT_KEY_F2:
    g_cloudRender = RGB;
    break;
  }

  if(cameraChanged) {
    viewMatrix = glm::lookAt(g_eye, g_center, glm::vec3(0, 1, 0));

    Shader* s = TinyGL::getInstance()->getShader("fcgt1");
    s->bind();
    s->setUniformMatrix("viewMatrix", viewMatrix);
  }
}

void exit_cb()
{
  glutDestroyWindow(g_window);
  destroy();
  exit(EXIT_SUCCESS);
}
