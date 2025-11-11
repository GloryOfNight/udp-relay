{
    "version": 8,
    "configurePresets": [
    {
      "name": "msvc2022",
      "displayName": "Visual Studio 17 2022",
      "generator": "Visual Studio 17 2022",
      "installDir": "${sourceDir}/install",
      "binaryDir": "${sourceDir}/build",
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "msvc2022",
      "displayName": "Debug",
      "configurePreset": "msvc2022",
      "configuration": "Debug"
    },
    {
      "name": "msvc2022-release",
      "displayName": "Release",
      "configurePreset": "msvc2022",
      "configuration": "Release"
    },
    {
      "name": "msvc2022-releasewithdgbinfo",
      "displayName": "Release with debug info",
      "configurePreset": "msvc2022",
      "configuration": "RelWithDebInfo"
    },
    {
      "name": "msvc2022-minsizerel",
      "displayName": "Release minimum size",
      "configurePreset": "msvc2022",
      "configuration": "MinSizeRel"
    }
  ]
}