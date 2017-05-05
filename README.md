# vulkan-visbuff 
Developer: Alberto Taiuti

Supervisor: Dr Paul Robertson

Industry mentor: Dr Matthäus G. Chajdas

Implementation of a Visibility Buffer renderer using the AMD GCN extensions.
Developed as part of my honours project.

## License
MIT.  
See [LICENSE](https://github.com/snowzurfer/vulkan-visbuff/blob/master/LICENSE).

## Prerequisites
* Git LFS
* Vulkan SDK and Vulkan drivers
* CMake
* Models: https://goo.gl/ajpMH0

## Basic structure
Two binaries are produced:
* vksagres-deferred
* vksagres-visbuffer

One library is produced which is linked against the executables to provide
common functionalities.

## Cloning and building
#### Debug
* Clone the repository
* Download the models and unpack them in the folder `assets\models\` so that
the path to the model files looks like this:
`assets\models\model_a\modelfile.dae`
* Make sure you meet the prerequisites as above
* Create a separate `build` folder
* Inside the build folder create a folder called `debug`
* Use CMake to configure and then generate the project in the `debug` folder
  * If using CMake GUI:
     * Change the entry `CMAKE_BUILD_TYPE` to **Debug**
     * Set the build folder to the `debug` folder aforementioned 
  * If using CMake CLI
     * Pass `-DCMAKE_BUILD_TYPE=Debug` when launching CMake
     * Launch Cmake from the `debug` build folder
* Compile
#### Release
* Repeat steps for Debug but change the build type to **Release** and create
a separate folder called `release` into `build`

## Drivers
Vulkan drivers and other related resources can be found at https://www.khronos.org/vulkan/

## Usage
### Activation and usage of performance counters
The performance counters are activated by uncommenting the lines `1201` and
`1080` in `visbuff/renderer.cpp` and lines `1057` and `1103` in
`deferred/deferred_renderer.cpp`. Then recompile.

Capture of a sample can be done in two ways:
1. Move the camera to the desired location, then press `C`
2. Press `N` anywhere and the application will automatically take 10 samples
at predefined locations.

The captured data is located in your build folder in `\perf_data`.
