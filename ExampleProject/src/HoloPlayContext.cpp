/**
 * HoloPlayContext.cpp
 * Contributors:
 *      * Arthur Sonzogni (author), Looking Glass Factory Inc.
 * Licence:
 *      * MIT
 */

#include "HoloPlayContext.hpp"
#include "HoloPlayShaders.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <iostream>
#include <stdexcept>

#include "Shader.hpp"
#include "glError.hpp"

using namespace std;

HoloPlayContext *currentApplication = NULL;

HoloPlayContext &HoloPlayContext::getInstance()
{
  if (currentApplication)
    return *currentApplication;
  else
    throw std::runtime_error("There is no current Application");
}

// Mouse and scroll function wrapper
// ========================================================
// callback functions must be static
HoloPlayContext &getInstance()
{
  if (currentApplication)
    return *currentApplication;
  else
    throw std::runtime_error("There is no current Application");
}

// wrapper for getting mouse movement callback
static void external_mouse_callback(GLFWwindow *window,
                                    double xpos,
                                    double ypos)
{
  // here we access the instance via the singleton pattern and forward the
  // callback to the instance method
  getInstance().mouse_callback(window, xpos, ypos);
}

// wrapper for getting mouse scroll callback
static void external_scroll_callback(GLFWwindow *window,
                                     double xpos,
                                     double ypos)
{
  // here we access the instance via the singleton pattern and forward the
  // callback to the instance method
  getInstance().scroll_callback(window, xpos, ypos);
}

HoloPlayContext::HoloPlayContext()
    : state(stateReady),
      title("Application"),
      opengl_version_major(3),
      opengl_version_minor(3)
{
  currentApplication = this;

  // get device info via holoplay core
  if (!GetLookingGlassInfo())
  {
    cout << "[Info] HoloplayCore Message Pipe tear down" << endl;
    state = stateExit;
    // must tear down the message pipe before shut down the app
    hpc_TeardownMessagePipe();
    throw std::runtime_error("Couldn't find looking glass");
  }
  // get the viewcone here, which is used as a const
  viewCone = hpc_GetDevicePropertyFloat(DEV_INDEX, "/calibration/viewCone/value");

  cout << "[Info] GLFW initialisation" << endl;

  opengl_version_header = "#version ";
  opengl_version_header += to_string(opengl_version_major);
  opengl_version_header += to_string(opengl_version_minor);
  opengl_version_header += "0 core\n";

  // initialize the GLFW library
  if (!glfwInit())
  {
    throw std::runtime_error("Couldn't init GLFW");
  }
  
  // set opengl version (will also be used for lightfield and blit shaders)

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_version_major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_version_minor);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // create window on the first looking glass device
  window = openWindowOnLKG();
  if (!window)
  {
    glfwTerminate();
    throw std::runtime_error(
        "Couldn't create a window on looking glass device");
  }
  cout << "[Info] Window opened on lkg" << endl;
  glfwMakeContextCurrent(window);
  glCheckError(__FILE__, __LINE__);

  // set up the cursor callback
  glfwSetCursorPosCallback(window, external_mouse_callback);
  glfwSetScrollCallback(window, external_scroll_callback);
  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glCheckError(__FILE__, __LINE__);

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  glCheckError(__FILE__, __LINE__);

  if (err != GLEW_OK)
  {
    cout << "terminiated" << endl;
    glfwTerminate();
    throw std::runtime_error(string("Could initialize GLEW, error = ") +
                             (const char *)glewGetErrorString(err));
  }

  // get OpenGL version info
  const GLubyte *renderer = glGetString(GL_RENDERER);
  const GLubyte *version = glGetString(GL_VERSION);
  cout << "Renderer: " << renderer << endl;
  cout << "[Info] OpenGL version supported " << version << endl;

  // opengl configuration
  glEnable(GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc(GL_LESS);    // depth-testing interprets a smaller value as "closer"

  // initialize the holoplay context
  initialize();
}

void HoloPlayContext::OnExit()
{
  cout << "[INFO] : on exit" << endl;
}

void HoloPlayContext::exit()
{
  state = stateExit;
  cout << "[Info] Informing Holoplay Core to close app" << endl;
  hpc_CloseApp();
  // release all the objects created for setting up the HoloPlay Context
  release();
}

// main loop
void HoloPlayContext::run()
{
  state = stateRun;

  // Make the window's context current
  glfwMakeContextCurrent(window);

  time = glfwGetTime();

  // create a viewTex for rendering
  unsigned int viewTex;
  // create framebuffer and texture color buffer
  unsigned int framebuffer, textureColorbuffer;
  // create a renderbuffer object for depth and stencil attachment
  unsigned int rbo;
  // set up all related objects for saving a single view here
  setupViewTextureAndFrameBuffer(viewTex, framebuffer, textureColorbuffer, rbo);

  while (state == stateRun)
  {
    // compute new time and delta time
    float t = glfwGetTime();
    deltaTime = t - time;
    time = t;

    // detech window related changes
    detectWindowChange();
    glCheckError(__FILE__, __LINE__);

    // press esc to quit
    if (!processInput(window))
    {
      exit();
      OnExit();
      continue;
    }

    // decide how camera updates here, override in SampleScene.cpp
    glm::mat4 currentViewMatrix = GetViewMatrixOfCurrentFrame();
    glCheckError(__FILE__, __LINE__);

    // do the update
    update();

    // render views and copy each view to the quilt
    for (int viewIndex = 0; viewIndex < qs_totalViews; viewIndex++)
    {
      // 1. bind to framebuffer and draw scene as we normally would to color texture
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
      glCheckError(__FILE__, __LINE__);
      glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering texture)
      glCheckError(__FILE__, __LINE__);

      // 2. accept fragment if it closer to the camera than the former one
      glDepthFunc(GL_LESS);
      glCheckError(__FILE__, __LINE__);

      // 3. make sure we clear the framebuffer's content
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glCheckError(__FILE__, __LINE__);

      // 4. set up the camera rotation and position for current view
      setupVirtualCameraForView(viewIndex, currentViewMatrix);
      glCheckError(__FILE__, __LINE__);

      // 5. render the scene according to the view
      renderScene();

      // 6. now bind back to default framebuffer
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // 7. disbale the depth and clear color before rendering view texture
      // -----------------------------------------------------------------
      glDisable(GL_DEPTH_TEST); // disable depth test so texture isn't
                                // discarded due to depth test.
      // set clear color to white (not really necessary actually,
      // since we won't be able to see behind the texture anyways)
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      // clear all relevant buffers
      glClear(GL_COLOR_BUFFER_BIT);

      // 8. now render on view texture
      glBindVertexArray(viewTex);
      glBindTexture(GL_TEXTURE_2D,
                    textureColorbuffer); // use the color attachment texture as
                                         // the texture
      glCheckError(__FILE__, __LINE__);

      // 9. copy current view texture to the responding view grid in quilt
      copyViewToQuilt(viewIndex);
      glCheckError(__FILE__, __LINE__);
    }

    // draw the light field image
    drawLightField();

    // Swap Front and Back buffers (double buffering)
    glfwSwapBuffers(window);
    // Poll and process events
    glfwPollEvents();
  }

  // clean up objects created for saving and transferring single view
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteTextures(1, &textureColorbuffer);
  glDeleteRenderbuffers(1, &rbo);
  glDeleteTextures(1, &viewTex);

  glfwTerminate();
}

// window coordinates may be changed when the main display is scaled and the
// looking glass display is not, so we make this function here to detect the
// window change and force our window to be full-screen again
void HoloPlayContext::detectWindowChange()
{
  int w, h;
  int x, y;
  glfwGetWindowSize(getWindow(), &w, &h);
  glfwGetWindowPos(getWindow(), &x, &y);

  windowChanged = (w != win_w) || (h != win_h) || (x != win_x) || (y != win_y);
  if (windowChanged)
  {
    cout << "[Info] Dimension changed: (" << w << "," << h << ")" << endl;
    glfwSetWindowPos(getWindow(), win_x, win_y);
    glfwSetWindowSize(getWindow(), win_w, win_h);
    cout << "[Info] force window to be full-screen again" << endl;
  }
}

// virtual functions
// =========================================================
void HoloPlayContext::update()
{
  cout << "[INFO] : update" << endl;
  // implement update function in the child class
}

void HoloPlayContext::renderScene()
{
  cout << "[INFO] : render scene" << endl;
}

glm::mat4 HoloPlayContext::GetViewMatrixOfCurrentFrame()
{
  cout << "[INFO] : update camera" << endl;
  return glm::mat4(1.0);
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly. return false to close application
// ---------------------------------------------------------------------------------------------------------
bool HoloPlayContext::processInput(GLFWwindow *window)
{
  cout << "[INFO] : process input" << endl;
  return true;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void HoloPlayContext::mouse_callback(GLFWwindow *window,
                                     double xpos,
                                     double ypos) {}

void HoloPlayContext::scroll_callback(GLFWwindow *window,
                                      double xoffset,
                                      double yoffset) {}

// get functions
// =========================================================
int HoloPlayContext::getWidth()
{
  return win_w;
}

int HoloPlayContext::getHeight()
{
  return win_h;
}

float HoloPlayContext::getWindowRatio()
{
  return float(win_w) / float(win_h);
}

GLFWwindow *HoloPlayContext::getWindow() const
{
  return window;
}

bool HoloPlayContext::windowDimensionChanged()
{
  return windowChanged;
}

float HoloPlayContext::getFrameDeltaTime() const
{
  return deltaTime;
}

float HoloPlayContext::getTime() const
{
  return time;
}

// Sample code for creating HoloPlay context

// Holoplay Core related functions
// =========================================================
// Register and initialize the app through HoloPlay Core
// And print information about connected Looking Glass devices
bool HoloPlayContext::GetLookingGlassInfo()
{
  hpc_client_error errco =
      hpc_InitializeApp("Holoplay Core Example App", hpc_LICENSE_NONCOMMERCIAL);
  if (errco)
  {
    string errstr;
    switch (errco)
    {
    case hpc_CLIERR_NOSERVICE:
      errstr = "HoloPlay Service not running";
      break;
    case hpc_CLIERR_SERIALIZEERR:
      errstr = "Client message could not be serialized";
      break;
    case hpc_CLIERR_VERSIONERR:
      errstr = "Incompatible version of HoloPlay Service";
      break;
    case hpc_CLIERR_PIPEERROR:
      errstr = "Interprocess pipe broken";
      break;
    case hpc_CLIERR_SENDTIMEOUT:
      errstr = "Interprocess pipe send timeout";
      break;
    case hpc_CLIERR_RECVTIMEOUT:
      errstr = "Interprocess pipe receive timeout";
      break;
    default:
      errstr = "Unknown error";
      break;
    }
    cout << "HoloPlay Service access error (code " << errco << "): " << errstr
         << "!" << endl;
    return false;
  }
  char buf[1000];
  hpc_GetHoloPlayCoreVersion(buf, 1000);
  cout << "HoloPlay Core version " << buf << "." << endl;
  hpc_GetHoloPlayServiceVersion(buf, 1000);
  cout << "HoloPlay Service version " << buf << "." << endl;
  int num_displays = hpc_GetNumDevices();
  cout << num_displays << " devices connected." << endl;
  if (num_displays < 1)
  {
    return false;
  }
  for (int i = 0; i < num_displays; ++i)
  {
    cout << "Device information for display " << i << ":" << endl;
    hpc_GetDeviceHDMIName(i, buf, 1000);
    cout << "\tDevice name: " << buf << endl;
    hpc_GetDeviceType(i, buf, 1000);
    cout << "\tDevice type: " << buf << endl;
    hpc_GetDeviceType(i, buf, 1000);
    cout << "\nWindow parameters for display " << i << ":" << endl;
    cout << "\tPosition: (" << hpc_GetDevicePropertyWinX(i) << ", "
         << hpc_GetDevicePropertyWinY(i) << ")" << endl;
    cout << "\tSize: (" << hpc_GetDevicePropertyScreenW(i) << ", "
         << hpc_GetDevicePropertyScreenH(i) << ")" << endl;
    cout << "\tAspect ratio: " << hpc_GetDevicePropertyDisplayAspect(i) << endl;
    cout << "\nShader uniforms for display " << i << ":" << endl;
    cout << "\tPitch: " << hpc_GetDevicePropertyPitch(i) << endl;
    cout << "\tTilt: " << hpc_GetDevicePropertyTilt(i) << endl;
    cout << "\tCenter: " << hpc_GetDevicePropertyCenter(i) << endl;
    cout << "\tSubpixel width: " << hpc_GetDevicePropertySubp(i) << endl;
    cout << "\tView cone: "
         << hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value")
         << endl;
    cout << "\tFringe: " << hpc_GetDevicePropertyFringe(i) << endl;
    cout << "\tRI: " << hpc_GetDevicePropertyRi(i)
         << "\n\tBI: " << hpc_GetDevicePropertyBi(i)
         << "\n\tinvView: " << hpc_GetDevicePropertyInvView(i) << endl;
  }

  return true;
}

// setup fuctions
// =========================================================
void HoloPlayContext::initialize()
{
  cout << "[Info] initializing" << endl;
  glfwMakeContextCurrent(window);

  loadBlitShaders();
  glCheckError(__FILE__, __LINE__);

  loadLightFieldShaders();
  glCheckError(__FILE__, __LINE__);

  loadCalibrationIntoShader();
  glCheckError(__FILE__, __LINE__);

  setupQuiltSettings(1);
  passQuiltSettingsToShader();
  glCheckError(__FILE__, __LINE__);

  setupQuilt();
  glCheckError(__FILE__, __LINE__);
}

// set up the quilt settings
void HoloPlayContext::setupQuiltSettings(int preset)
{
  // there are 3 presets:
  switch (preset)
  {
  case 0: // standard
    qs_width = 2048;
    qs_height = 2048;
    qs_columns = 4;
    qs_rows = 8;
    qs_totalViews = 32;
    break;
  default:
  case 1: // hires
    qs_width = 4096;
    qs_height = 4096;
    qs_columns = 5;
    qs_rows = 9;
    qs_totalViews = 45;
    break;
  case 2: // 8k
    qs_width = 4096 * 2;
    qs_height = 4096 * 2;
    qs_columns = 5;
    qs_rows = 9;
    qs_totalViews = 45;
    break;
  }
}
// pass quilt values to shader
void HoloPlayContext::passQuiltSettingsToShader()
{
  lightFieldShader->use();
  lightFieldShader->setUniform("overscan", 0);
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("tile",
                               glm::vec3(qs_columns, qs_rows, qs_totalViews));
  glCheckError(__FILE__, __LINE__);

  int qs_viewWidth = qs_width / qs_columns;
  int qs_viewHeight = qs_height / qs_rows;

  lightFieldShader->setUniform(
      "viewPortion", glm::vec2(qs_viewWidth * qs_columns / (float)qs_width,
                               qs_viewHeight * qs_rows / (float)qs_height));
  glCheckError(__FILE__, __LINE__);
  lightFieldShader->unuse();
}

void HoloPlayContext::setupQuilt()
{
  cout << "setting up quilt texture and framebuffer" << endl;
  glGenTextures(1, &quiltTexture);
  glBindTexture(GL_TEXTURE_2D, quiltTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, qs_width, qs_height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  // framebuffer
  glGenFramebuffers(1, &FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  // bind the quilt texture as the color attachment of the framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         quiltTexture, 0);

  // vbo and vao
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  // set up the vertex array object
  glBindVertexArray(VAO);

  // fullscreen quad vertices
  const float fsquadVerts[] = {
      -1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
  };

  // create vbo
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(fsquadVerts), fsquadVerts,
               GL_STATIC_DRAW);

  // setup the attribute pointers
  // note: using only 2 floats per vert, not 3
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // unbind stuff
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void HoloPlayContext::loadBlitShaders()
{
  cout << "loading blit shaders" << endl;
  string blitFragmentShaderSource = R"--(
  in vec2 texCoords;
  out vec4 fragColor;
  uniform sampler2D blitTex;
  void main()
  {
      fragColor = texture(blitTex, texCoords.xy);
  }
  )--";
  string blitVertexShaderSource = R"--(
  layout (location = 0)
  in vec2 vertPos_data;
  out vec2 texCoords;
  void main()
  {
      gl_Position = vec4(vertPos_data.xy, 0.0, 1.0);
      texCoords = (vertPos_data.xy + 1.0) * 0.5;
  }
  )--";
  Shader blitVertexShader(
      GL_VERTEX_SHADER,
      (opengl_version_header + blitVertexShaderSource).c_str());
  Shader blitFragmentShader(
      GL_FRAGMENT_SHADER,
      (opengl_version_header + blitFragmentShaderSource).c_str());

  blitShader = new ShaderProgram({blitVertexShader, blitFragmentShader});
}

void HoloPlayContext::loadLightFieldShaders()
{
  cout << "loading quilt shader" << endl;
  Shader lightFieldVertexShader(
      GL_VERTEX_SHADER,
      (opengl_version_header + hpc_LightfieldVertShaderGLSL).c_str());
  Shader lightFieldFragmentShader(
      GL_FRAGMENT_SHADER,
      (opengl_version_header + hpc_LightfieldFragShaderGLSL).c_str());
  lightFieldShader =
      new ShaderProgram({lightFieldVertexShader, lightFieldFragmentShader});
}

void HoloPlayContext::loadCalibrationIntoShader()
{
  cout << "begin assigning calibration uniforms" << endl;
  lightFieldShader->use();
  lightFieldShader->setUniform("pitch", hpc_GetDevicePropertyPitch(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("tilt", hpc_GetDevicePropertyTilt(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("center",
                               hpc_GetDevicePropertyCenter(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("invView",
                               hpc_GetDevicePropertyInvView(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("quiltInvert", 0);
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("subp", hpc_GetDevicePropertySubp(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("ri", hpc_GetDevicePropertyRi(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("bi", hpc_GetDevicePropertyBi(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);

  lightFieldShader->setUniform("displayAspect",
                               hpc_GetDevicePropertyDisplayAspect(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);
  lightFieldShader->setUniform("quiltAspect",
                               hpc_GetDevicePropertyDisplayAspect(DEV_INDEX));
  glCheckError(__FILE__, __LINE__);
  lightFieldShader->unuse();
  glCheckError(__FILE__, __LINE__);
}

void HoloPlayContext::setupViewTextureAndFrameBuffer(
    unsigned int &viewTexture,
    unsigned int &framebuffer,
    unsigned int &textureColorbuffer,
    unsigned int &rbo)
{
  glGenTextures(1, &viewTexture);
  glBindTexture(GL_TEXTURE_2D, viewTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_w, win_h, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, win_w, win_h, 0,
               GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

  // generate framebuffer
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  // create a color attachment texture
  glGenTextures(1, &textureColorbuffer);
  glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_w, win_h, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  // attach it to currently bound framebuffer object
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         textureColorbuffer, 0);

  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, win_w,
                        win_h); // use a single renderbuffer object for both a
                                // depth AND stencil buffer.
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  // attach the renderbuffer object to the depth and stencil attachment of the
  // framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo); // now actually attach it

  // now that we actually created the framebuffer and added all attachments we
  // want to check if it is actually complete 
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "[ERROR] Framebuffer is not complete!" << std::endl;
  }
  else
  {
    // attach texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D, viewTexture, 0);
    glCheckError(__FILE__, __LINE__);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// release function
// =========================================================
void HoloPlayContext::release()
{
  cout << "[Info] HoloPlay Context releasing" << endl;
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteFramebuffers(1, &FBO);
  glDeleteTextures(1, &quiltTexture);
  delete lightFieldShader;
  delete blitShader;
}

// render functions
// =========================================================
// set up the camera for each view and the shader of the rendering object
void HoloPlayContext::setupVirtualCameraForView(int currentViewIndex,
                                                glm::mat4 currentViewMatrix)
{
  // The standard model Looking Glass screen is roughly 4.75" vertically. If we
  // assume the average viewing distance for a user sitting at their desk is
  // about 36", our field of view should be about 14°. There is no correct
  // answer, as it all depends on your expected user's distance from the Looking
  // Glass, but we've found the most success using this figure.
  const float fov = glm::radians(14.0f);
  float cameraDistance = -cameraSize / tan(fov / 2.0f);

  float offsetAngle =
      (currentViewIndex / (qs_totalViews - 1.0f) - 0.5f) *
      glm::radians(
          viewCone); // start at -viewCone * 0.5 and go up to viewCone * 0.5

  float offset =
      cameraDistance *
      tan(offsetAngle); // calculate the offset that the camera should move

  // modify the view matrix (position)
  viewMatrix = glm::translate(currentViewMatrix,
                              glm::vec3(offset, 0.0f, cameraDistance));

  float aspectRatio = getWindowRatio();

  projectionMatrix = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);
  // modify the projection matrix, relative to the camera size and aspect ratio
  projectionMatrix[2][0] += offset / (cameraSize * aspectRatio);
}

// copy view to quilt
void HoloPlayContext::copyViewToQuilt(int view)
{
  // framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  // save the viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  int qs_viewWidth = qs_width / qs_columns;
  int qs_viewHeight = qs_height / qs_rows;
  int x = (view % qs_columns) * qs_viewWidth;
  int y = (view / qs_columns) * qs_viewHeight;

  glViewport(x, y, qs_viewWidth, qs_viewHeight);
  glScissor(x, y, qs_viewWidth, qs_viewHeight);

  // vao
  glBindVertexArray(VAO);

  // use the shader and draw
  blitShader->use();
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // reset framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // reset viewport
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
  glBindVertexArray(0);
  blitShader->unuse();
}

void HoloPlayContext::drawLightField()
{
  // bind quilt texture
  glBindTexture(GL_TEXTURE_2D, quiltTexture);

  // bind vao
  glBindVertexArray(VAO);

  // use the shader and draw
  lightFieldShader->use();
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // clean up
  glBindVertexArray(0);
  lightFieldShader->unuse();
}

// Other helper functions
// =======================================================================
// open window at looking glass monitor
GLFWwindow *HoloPlayContext::openWindowOnLKG()
{
  // Load GLFW and Create a Window
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  // open the window
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CENTER_CURSOR, false);
  glfwWindowHint(GLFW_DECORATED, false);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

  // get the window size / coordinates
  win_w = hpc_GetDevicePropertyScreenW(DEV_INDEX);
  win_h = hpc_GetDevicePropertyScreenH(DEV_INDEX);
  win_x = hpc_GetDevicePropertyWinX(DEV_INDEX);
  win_y = hpc_GetDevicePropertyWinY(DEV_INDEX);
  cout << "[Info] window opened at (" << win_x << ", " << win_y << "), size: ("
       << win_w << ", " << win_h << ")" << endl;
  // open the window
  auto mWindow =
      glfwCreateWindow(win_w, win_h, "Looking Glass Output", NULL, NULL);

  glfwSetWindowPos(mWindow, win_x, win_y);

  return mWindow;
}
