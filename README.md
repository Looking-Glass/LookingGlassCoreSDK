**Note that this SDK is now out of date.**

This SDK does not have support for the lastest Looking Glass devices (Looking Glass Go, Looking Glass 16" Spatial Displays, and Looking Glass 32" Spatial Displays). **[Please use our new Bridge SDK instead](https://github.com/Looking-Glass/bridge-sdk-samples).** You can see documentation on our Bridge SDK [here](https://docs.lookingglassfactory.com/core/looking-glass-bridge-sdk) and a migration guide from Looking Glass Core [here](https://docs.lookingglassfactory.com/core/looking-glass-core-migration-guide).

# Looking Glass Core

The Looking Glass Core SDK (formerly HoloPlay Core) unlocks the ability to integrate your existing 3D software with Looking Glass displays, making the Looking Glass a holographic second monitor.

The SDK comes in two different flavors:

- Native dynamic libraries for Windows, Mac, and Linux (contained in this repository)
- JavaScript for web or Node development, which you can [access here](https://github.com/Looking-Glass/holoplaycore.js)

## Prerequisites 

Developers can use any rendering backend (DirectX, OpenGL, Metal, Vulkan) and any windowing library (e.g. GLFW, Qt, SDL). Beyond that, in order to render to the Looking Glass display, your application must be able to:

- Generate a borderless fullscreen window at a specific position
- Distort the camera's projection matrix
- Render multiple views to a texture
- Apply a shader to the texture

Also, your end user must have [Looking Glass Bridge](https://lookingglassfactory.com/software/looking-glass-bridge) installed on their machine.

## How It Works 

The Looking Glass Core SDK provides your application all the information it needs to draw to the Looking Glass, including window information and calibration.

#### **Why Window Information is Important**

Operating systems see connected Looking Glass devices as HDMI monitors. Your application must know how the OS sees the monitor so that it knows where to draw the windows.

#### **Why Calibration Data is Important**

Each Looking Glass has its own device-specific calibration, read over USB. Your application must know this calibration data in order to render holograms properly on the Looking Glass.

#### **All Together**

Looking Glass Core (the graphics library integrated into your application) gets this information by providing an API endpoint to request information from Looking Glass Bridge (a persistent driver-like service that is installed on the user’s machine). Looking Glass Core uses this information to draw your 3D scene to the Looking Glass. 

## Guides

These reference documents cover some of the underlying logic inside the SDK:

- [Camera](https://docs.lookingglassfactory.com/keyconcepts/camera)
- [Quilt](https://docs.lookingglassfactory.com/keyconcepts/quilts)

An example OpenGL project is distributed as part of the Looking Glass Core SDK to demonstrate how to move the camera and render to a texture in the appropriate format.

To learn more about the Looking Glass and how it works, click [here](https://docs.lookingglassfactory.com/keyconcepts/how-it-works). 

## Questions

Email us at [support@lookingglassfactory.com](mailto:support@lookingglassfactory.com) if you have any further questions about how you can integrate Looking Glass Core into your software.

