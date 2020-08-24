/**
 * HoloPlayContext.hpp
 * Contributors:
 *      * Arthur Sonzogni (author), Looking Glass Factory Inc.
 * Licence:
 *      * MIT
 */

#ifndef OPENGL_CMAKE_SKELETON_APPLICATION_HPP
#define OPENGL_CMAKE_SKELETON_APPLICATION_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <string>
#include "HoloPlayCore.h"
#include "Shader.hpp"

struct GLFWwindow;
struct GLFWmonitor;
struct hpc_Uniforms_t;

class HoloPlayContext
{
public:
    HoloPlayContext();

    static HoloPlayContext &getInstance();

    // get the window id
    GLFWwindow *getWindow() const;

    // exit function
    void exit();

    // delta time between frame and time from beginning
    float getFrameDeltaTime() const;
    float getTime() const;

    // application run
    void run();

    // Window functions
    int getWidth();
    int getHeight();
    float getWindowRatio();
    bool windowDimensionChanged();

    // functions that should be overrieded in the child class
    virtual void onExit();
    virtual void update();      // update function that will run every frame
    virtual void renderScene(); // render scene here
    virtual glm::mat4
    getViewMatrixOfCurrentFrame();                 // define how view matrix gets updated each
                                                   // frame here. projection matrix is not
                                                   // changeable because the field of view is
                                                   // static according to hardware measurement
    virtual bool processInput(GLFWwindow *window); // all key inputs
    virtual void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    virtual void scroll_callback(GLFWwindow *window,
                                 double xoffset,
                                 double yoffset);

private:
    enum State
    {
        stateReady,
        stateRun,
        stateExit
    };

    State state;

    HoloPlayContext &operator=(const HoloPlayContext &) { return *this; }

    GLFWwindow *window;

    // Window dimensions:
    int win_w;
    int win_h;
    int win_x;
    int win_y;

    bool windowChanged;
    void detectWindowChange();

    // storing matrix of each view
    glm::mat4 projectionMatrix = glm::mat4(1.0);
    glm::mat4 viewMatrix = glm::mat4(1.0);

protected:
    HoloPlayContext(const HoloPlayContext &){};

    std::string title;
    int opengl_version_major;
    int opengl_version_minor;
    std::string opengl_version_header;

    // Time:
    float time;
    float deltaTime;

    // lkg related:
    const int DEV_INDEX = 0; // the index of device we are rendering on,
                             // default is 0, the first Looking Glass detected
    float cameraSize = 5;    // size of the holoplay camera,
                             // changeable without limitation
    float viewCone = 40.0;   // view cone of hardware, always around 40

    // quilt settings
    // more info at
    // https://docs.lookingglassfactory.com/HoloPlayCAPI/guides/quilt/
    int qs_width;      // Total width of the quilt texture
    int qs_height;     // Total height of the quilt texture
    int qs_rows;       // Number of columns in the quilt
    int qs_columns;    // Number of rows in the quilt
    int qs_totalViews; // The total number of views in the quilt.
                       // Note that this number might be lower than rows *
                       // columns
    // qs_viewWidth & qs_viewHeight could be calculated by given numbers

    // shaders:
    ShaderProgram *lightFieldShader =
        NULL; // The shader program for drawing light field images to the Looking
              // Glass
    ShaderProgram *blitShader =
        NULL; // The shader program for copying views to the quilt

    // render var
    unsigned int
        quiltTexture; // The texture object used internally to draw quilt,
                      // It is bound and drawn by drawLightfield()
    unsigned int VAO; // The vertex array object used internally to blit to the
                      // quilt and screen
    unsigned int VBO; // The vertex buffer object used internally to blit to the
                      // quilt and screen
    unsigned int FBO; // The frame buffer object used internally to blit views to
                      // the quilt

    // example implementation for rendering 45 views
    // ====================================================================================
    // set up functions
    void initialize(); // calls all the functions necessary to set up the
                       // HoloPlay Context
    void setupQuilt(); // create the quiltTexture, VBO, VAO, and FBO
    void setupQuiltSettings(
        int preset);                  // Set up the quilt settings according to the preset passed
                                      // 0: 32 views
                                      // 1: 45 views, normally used one
                                      // 2: 45 views for 8k display
                                      // Feel free to customize if you want
    void passQuiltSettingsToShader(); // assign quilt settings to light-field
                                      // shader uniforms
    void loadCalibrationIntoShader(); // assign calibration to light-field shader
                                      // uniforms
    void loadLightFieldShaders();     // create and compile light-field shader
    void loadBlitShaders();           // create and comiple blit shader

    // create view texture, framebuffer and texture color buffer, render buffer
    // object and will output the bound handlers
    void setupViewTextureAndFrameBuffer(unsigned int &viewTexture,
                                        unsigned int &framebuffer,
                                        unsigned int &textureColorbuffer,
                                        unsigned int &rbo);

    // release function
    void release(); // Destroys / releases all buffers and objects creating
                    // during initialize()

    // render functions
    void setupVirtualCameraForView( // Changes the view matrix and projection
        int currentViewIndex,       // accoriding to the view index and the
                                    // currentViewMatrix
        glm::mat4 currentViewMatrix);

    void drawLightField();          // Uses the lightfieldShader program,
                                    // binds the quiltTexture, and draws a fullscreen
                                    // quad. Call this after all the views have been
                                    // copied into quiltTexture using copyViewIntoQuilt().
    void copyViewToQuilt(int view); // Copies the currently bound glTexture to
                                    // the specified view on the quilt

    // holoplay core related helper functions
    bool
    GetLookingGlassInfo(); // get all the information of all connected looking
                           // glass retuyrn false if no looking glass detected
    GLFWwindow *
    openWindowOnLKG(); // open a full-szie window on the looking glass

    // some get functions
    unsigned int getQuiltTexture() { return quiltTexture; }
    unsigned int getLightfieldShader() { return lightFieldShader->getHandle(); }
    glm::mat4 GetProjectionMatrixOfCurrentView() { return projectionMatrix; }
    glm::mat4 GetViewMatrixOfCurrentView() { return viewMatrix; }
};

#endif /* end of include guard: OPENGL_CMAKE_SKELETON_APPLICATION_HPP */
