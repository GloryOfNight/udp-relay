{
    "version": 8,
    "configurePresets": [
    {
      "name": "ninja-multi",
      "displayName": "Ninja Multi-Config",
      "description": "Default build using Ninja Multi-Config generator",
      "generator": "Ninja Multi-Config",
      "installDir": "${sourceDir}/install",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_CXX_COMPILER": "clang++"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "ninja",
      "displayName": "Debug",
      "configurePreset": "ninja-multi",
      "configuration": "Debug"
    },
    {
      "name": "ninja-release",
      "displayName": "Release",
      "configurePreset": "ninja-multi",
      "configuration": "Release"
    },
    {
      "name": "ninja-releasewithdgbinfo",
      "displayName": "Release with debug info",
      "configurePreset": "ninja-multi",
      "configuration": "RelWithDebInfo"
    },
    {
      "name": "ninja-minsizerel",
      "displayName": "Release minimum size",
      "configurePreset": "ninja-multi",
      "configuration": "MinSizeRel"
    }
  ]
}