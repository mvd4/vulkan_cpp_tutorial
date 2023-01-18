# Getting Started

## Prerequisites
### Compiler toolchain
The code for this tutorial should work with any compiler that supports the C++17 standard, so if you already have a C++ toolchain working, you'll likely be able to use that. If not, here are a few suggestions:
* if you're on Windows, the [Visual Studion Community Edition](https://visualstudio.microsoft.com/vs/community/) is probably the most convenient C++ environment to install
* on Mac, [Xcode]([https://apps.apple.com/de/app/xcode/id497799835?mt=12) is the default. I'm not really a fan of that IDE, but it does the job. You can easily install it from the AppStore. 
* if you're on Linux or don't want to use a full-fledged IDE, you might want to look at [VS-Code](https://code.visualstudio.com/). It's a pretty awesome editor that runs on multiple platforms. And with a few extensions it can be converted into a veritable C++ development environment.

### CMake
Because we want to develop a cross-platform graphics application, we should make sure that our project can easily be built on multiple platforms. The most widespread way to do that for C++ is to use [CMake](https://cmake.org/). Please download and install the latest version for your platform.

### Conan
[Conan](https://conan.io/) is a package manager, similar to e.g. Pypi for Phython. Using Conan makes dependency management a lot easier in many cases. Please refer to their setup guide for information on how to install it on your platform.1

### Vulkan SDK
We want to develop a Vulkan application, so it might actually be a good idea to install the official Vulkan SDK. Please go to the [LunarG website](https://www.lunarg.com/vulkan-sdk/) and download and execute the installer for your platform. The location where you put the SDK is not relevant.

Alright, your environment should be set up now, so let's get going with our project.

## Project setup
### Source code checkout
Go to the bitbucket / github repository (see links in the sidebar on the right) and clone it to your computer. Then checkout the branch for lesson_1:
```
> git checkout lesson_1
```

### Creating the project files
I prefer to have my project files in a separate folder rather than mixed with my source files etc. Therefore I usually create a build folder. Navigate to your project folder and run the following commands on your console:
```
> mkdir build
> cd build
```
Next you'll need to run conan to install the dependencies. In your python environment (if that's where you installed conan) run:
```
> conan install ..
```
This shouldn't take more than a few seconds, because at this point we don't have any dependencies (this will change over the course of this tutorial).

Finally we need to run cmake
```
> cmake .. -G <your desired project file type>
```
That's it, you now should have a project file for your environment in the build folder.2 Open it in your IDE (if that's what you use) and try to compile and run the project to make sure everything works.

We're now set up to get started for real. In the next lesson we'll have a look at some of the basic concepts in the C++ wrapper before we finally get our hands dirty and start programming Vulkan.



---

1. if you're on mac you might be tempted to install Conan via homebrew. My experiences here haven't been so good, since brew only offerered an outdated version of Conan which then didn't work due to a certificate error. Better install as a python package in a virtual environment.
2. CMake should be able to locate the Vulkan SDK automatically. If it doesn't, please set the CMake variable `VULKAN_SDK_ROOT` to the path to the Vulkan SDK folder on your machine and try again.
