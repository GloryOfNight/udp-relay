{
    "version": 8,
    "configurePresets": [
    {
      "name": "msvc2026",
      "displayName": "Visual Studio 18 2026",
      "generator": "Visual Studio 18 2026",
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
      "name": "msvc2026",
      "displayName": "Debug",
      "configurePreset": "msvc2026",
      "configuration": "Debug"
    },
    {
      "name": "msvc2026-release",
      "displayName": "Release",
      "configurePreset": "msvc2026",
      "configuration": "Release"
    },
    {
      "name": "msvc2026-releasewithdgbinfo",
      "displayName": "Release with debug info",
      "configurePreset": "msvc2026",
      "configuration": "RelWithDebInfo"
    },
    {
      "name": "msvc2026-minsizerel",
      "displayName": "Release minimum size",
      "configurePreset": "msvc2026",
      "configuration": "MinSizeRel"
    }
  ]
}