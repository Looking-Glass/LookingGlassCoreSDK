/**
 * SampleScene.hpp
 * Contributors:
 *      * Arthur Sonzogni (author), Looking Glass Factory Inc.
 * Licence:
 *      * MIT
 */
#ifndef OPENGL_CMAKE_SKELETON_MYAPPLICATION
#define OPENGL_CMAKE_SKELETON_MYAPPLICATION

#include "HoloPlayContext.hpp"

class SampleScene : public HoloPlayContext
{
public:
  SampleScene();
  // control
  virtual void mouse_callback(GLFWwindow *window, double xpos, double ypos);
  virtual void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

protected:
  virtual void update();
  virtual void onExit();
  virtual void renderScene();
  virtual glm::mat4 getViewMatrixOfCurrentFrame();
  virtual bool processInput(GLFWwindow *window);

  ShaderProgram *shaderProgram;

private:
  const unsigned int size = 100;

  // VBO/VAO/ibo
  GLuint vao, vbo, ibo;

  // camera
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  bool firstMouse = true;
  float yaw = -90.0f; // yaw is initialized to -90.0 degrees since a yaw of 0.0
                      // results in a direction vector pointing to the right so
                      // we initially rotate a bit to the left.
  float pitch = 0.0f;
  float lastX = 800.0f / 2.0;
  float lastY = 600.0 / 2.0;
  int debug = 0;
};

#endif // OPENGL_CMAKE_SKELETON_MYAPPLICATION
