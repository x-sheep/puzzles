# Portable Puzzle Collection for Windows Store

[![Build status](https://ci.appveyor.com/api/projects/status/0bq5fuxibnqx3gbt?svg=true)](https://ci.appveyor.com/project/x-sheep/puzzles)

Port of [Simon Tatham's Portable Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/) to C++ Windows Store and Windows Phone apps. The interface is built in XAML, and the graphics are drawn with Direct2D.

<a href="https://get.microsoft.com/installer/download/9nblggh16n44?referrer=appbadge" target="_self" >
	<img src="docs/get.svg" alt="Download from the Microsoft Store" title="Download from the Microsoft Store" 
	width="284" />
</a>

## Building requirements

Note that this fork has several modifications which cause the original makefiles (for other platforms) to no longer work.

### UWP Version
* Visual Studio 2019 or higher
* Windows 10 SDK (10.0.19041.0) or higher

### Windows 8.1 version
* Visual Studio 2013 or 2015
* Windows 8.1 SDK

### Windows Phone version
* Visual Studio 2013 or 2015
* Windows Phone 8.1 SDK

## Contents

* `/` - The root folder contains the original sources written in C.
* `/html, /unfinished, /icons, /osx` - Only used by the upstream code.
* `/Help` - The HTML manual.
* `/PuzzleCommon` - Shared C++ sources. All graphics handling and interfacing with the original source code is located here.
* `/PuzzleModern` - Windows 8.1 project.
* `/PuzzleModern.Phone` - Windows Phone 8.1 project.
* `/PuzzleModern.UWP` - Windows 10 project.
* `/PuzzleTests` - Unit test project for PuzzleCommon.

## Copyright

(C) 2021 Lennard Sprong

Contains icons from ModernUIIcons.com

Based on Simon Tatham's Portable Puzzle Collection. [MIT License](LICENCE)
