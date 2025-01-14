# Android Vulkan Cube

Hello VK is an Android C++ sample that draws the simple rotating cube over a 3D plane, with comments
over each function. Based on the Android code lab
https://developer.android.com/codelabs/beginning-vulkan-on-android?hl=en#0.

---

Aside from the base functionality the source code also covers convenient things such as:

- Vulkan validation layers. See section below for information on how to enable these
- Vulkan transformations and rotations on objects
- Look at matrix with perspective
- Several comments explaining functions
- Added light to the scene

## Pre-requisites

- Android Studio 4.2+ with [NDK](https://developer.android.com/ndk/) bundle.

## Getting Started

1. [Download Android Studio](http://developer.android.com/sdk/index.html)
1. Launch Android Studio.
1. Open the sample directory.
1. Open *File/Project Structure...*

- Click *Download* or *Select NDK location*.

1. Click *File/Sync Project with Gradle Files*.
1. Click *Run/Run 'app'*.

## Validation layers

As the validation layer is a sizeable download, we chose to not ship them within the apk. As such in
order to enable validation layer, please follow the simple steps below:

1. Download the latest android binaries
   from: https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases
1. Place them in their respective ABI folders located in: app/src/main/jniLibs
1. Go to hellovk.h, search for 'bool enableValidationLayers = false' and toggle
   that to true.