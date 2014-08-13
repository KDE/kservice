# KService

Plugin framework for desktop services

## Introduction

KService provides a plugin framework for handling desktop services. Services can
be applications or libraries. They can be bound to MIME types or handled by
application specific code.

## Usage

If you are using CMake, you need to have

    find_package(KF5Service)

(or similar) in your CMakeLists.txt file, and you need to link to KF5::Service.

