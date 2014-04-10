#ifndef COLORSPACE_H
#define COLORSPACE_H

#include <vector>
#include <glm/glm.hpp>
#include "logger.h"

extern "C" {
#include "mtwist.h"
#include "color.h"
}

glm::vec3 createCIEXYZ(float* beta, float* illuminant, std::vector<glm::vec3> xyzbar, size_t step)
{
  if (!beta || !illuminant || xyzbar.empty() || !step) {
    Logger::getInstance()->warn("createCIEXYZ() -> One of the parameters is invalid, nothing will be done.");
    return glm::vec3(0, 0, 0);
  }

  glm::vec3 ciexyz(0, 0, 0);
  float n = 0.f;
  for (size_t i = 0; i < 400; i += step) {
    float p = beta[i] * illuminant[i];
    ciexyz.x += p * xyzbar[i].x;
    ciexyz.y += p * xyzbar[i].y;
    ciexyz.z += p * xyzbar[i].z;
    n += illuminant[i] * xyzbar[i].y;
  }

  ciexyz.x /= n;
  ciexyz.y /= n;
  ciexyz.z /= n;

  return ciexyz;
}

glm::vec3 createCIEXYZPureSource(float* illuminant, std::vector<glm::vec3> xyzbar, size_t step)
{
  if (!illuminant || xyzbar.empty() || step == 0) {
    Logger::getInstance()->warn("createCIEXYZPureSource() -> One of the parameters is invalid, nothing will be done.");
    return glm::vec3(0, 0, 0);
  }

  glm::vec3 ciexyz(0, 0, 0);
  float n = 0.f;
  for (size_t i = 0; i < 400; i += step) {
    ciexyz.x += illuminant[i] * xyzbar[i].x;
    ciexyz.y += illuminant[i] * xyzbar[i].y;
    ciexyz.z += illuminant[i] * xyzbar[i].z;
  }

  n = ciexyz.x + ciexyz.y + ciexyz.z;

  if (n != 0) {
    ciexyz.x /= n;
    ciexyz.y /= n;
    ciexyz.z /= n;
  }
  else
    ciexyz = glm::vec3(0);

  return ciexyz;
}

int createBetaCurve(float* beta, size_t n)
{
  if (beta == NULL || n == 0)
    return 0;

  for (size_t i = 0; i < n; i++) {
    mts_goodseed(&mt_default_state);
    beta[i] = (float)mt_drand();
  }
  return 1;
}

void createAndWriteBeta(size_t num_samples, size_t delta)
{
  std::string filename = "beta_reflectance_" + std::to_string(num_samples) + "_" + std::to_string(delta) + ".dat";
  FILE* fp = fopen(filename.c_str(), "wb+");

  float* beta = new float[400 / delta];

  for (size_t i = 0; i < num_samples; i++) {
    createBetaCurve(beta, 400 / delta);
    if (i % 100 == 0)
      Logger::getInstance()->log(std::to_string(i) + " beta spectrum");
    fwrite(beta, sizeof(float), 400 / delta, fp);
  }

  fclose(fp);
  delete[] beta;
}

#endif