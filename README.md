# Portable Puzzle Collection for Windows Store

[![Build status](https://ci.appveyor.com/api/projects/status/1025rpc6remat8xu/branch/master?svg=true)](https://ci.appveyor.com/project/x-sheep/puzzles/branch/master)

Port of [Simon Tatham's Portable Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/) to C++ Windows Store and Windows Phone apps. The interface is built in XAML, and the graphics are drawn with Direct2D.

[![Download From Windows Store](docs/download.png)](https://apps.microsoft.com/windows/app/43460909-920b-465a-afb3-9b92c33c7356)

## Building requirements

* Visual Studio 2013 or higher
* Windows 8.1 SDK for the desktop version
* Windows Phone 8.1 SDK for the phone version

Note that this fork has several modifications which cause the original makefiles (for other platforms) to no longer work.

## Contents

* `/` - The root folder contains the original sources written in C.
* `/html, /unfinished, /icons` - Only used by the upstream code.
* `/Help` - The HTML manual.
* `/PuzzleCommon` - Shared C++ sources. All graphics handling and interfacing with the original source code is located here.
* `/PuzzleModern` - Solution with Windows 8.1 project.
* `/PuzzleModernPhone` - Solution with Windows Phone 8.1 project.

## Copyright

(C) 2017 Lennard Sprong

Contains icons from ModernUIIcons.com

Based on Simon Tatham's Portable Puzzle Collection. [MIT License](LICENCE)