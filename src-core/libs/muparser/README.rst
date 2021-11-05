.. image:: https://travis-ci.org/beltoforion/muparser.svg?branch=master
    :target: https://travis-ci.org/beltoforion/muparser

.. image:: https://ci.appveyor.com/api/projects/status/u4882uj8btuspj9x?svg=true
    :target: https://ci.appveyor.com/project/beltoforion/muparser

.. image:: https://img.shields.io/github/issues/beltoforion/muparser.svg?maxAge=360
    :target: https://github.com/beltoforion/muparser/issues
 
.. image:: https://img.shields.io/github/release/beltoforion/muparser.svg?maxAge=360
    :target: https://github.com/beltoforion/muparser/blob/master/CHANGELOG
 
.. image:: https://repology.org/badge/tiny-repos/muparser.svg
    :target: https://repology.org/project/muparser/versions


muparser - Fast Math Parser 2.3.3 
===========================
.. image:: http://beltoforion.de/en/muparser/images/title.jpg
 

To read the full documentation please go to: http://beltoforion.de/en/muparser.

See Install.txt for installation

Change Notes for Revision 2.3.3  (Prerelease)
===========================
Security Fixes:  
------------
The following new issues, discovered by oss-fuzz are fixed: 

* https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24167 (Abrt)
* https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24355 (Heap-buffer-overflow READ 8)
* https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25402 (Heap-buffer-overflow READ 8)

Bugfixes:
-----------
* Fixed a couple of issues for building the C-Interface (muParserDLL.cpp/.h) with wide character support.
* fix for #93 (https://github.com/beltoforion/muparser/issues/93)
* fix for #94 (https://github.com/beltoforion/muparser/issues/94)

Fixed Compiler Warnings:
-----------
* Visual Studio: Disabled compiler warning 26812 (Prefer 'enum class' over 'enum') Use of plain old enums has not been deprecated and only MSVC is complaining. 
* Visual Studio: Disabled compiler warning 4251 (... needs to have dll-interface to be used by clients of class ...)  For technical reason the DLL contains the class API and the DLL API. Just do not use the class API if you intent to share the dll accross windows versions. (The same is true for Linux but distributions do compile each application against their own library version anyway)

Changes:
------------
* Adding manual definitions to avoid potential issues with MSVC
* Adding missing overrides
* Added a new option "-DENABLE_WIDE_CHAR" to CMake for building muparser with wide character support
* export muparser targets, such that client projects can import it using find_package() (https://github.com/beltoforion/muparser/pull/81#event-3528671228)

