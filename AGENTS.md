# Vulkan C++ Tutorial - Project Documentation

## Project Overview

This repository contains the code and lessons for a comprehensive Vulkan C++ tutorial series. The tutorial aims to teach the fundamentals of graphics programming using the official C++ wrapper for the cross-platform Vulkan Graphics SDK.

### Learning Goals

The tutorial covers:
- Compute and rendering pipelines
- Texture mapping
- Anti-aliasing
- Lighting techniques
- Modern graphics programming practices

### Scope

This tutorial is designed as a practical introduction to Vulkan, not an exhaustive reference. It focuses on building a solid foundation for graphics programming while maintaining accessibility for learners. Advanced topics such as performance optimization and raytracing are outside the scope of this tutorial.

### Repository Structure

Each lesson is maintained in a dedicated branch to help students follow along at their own pace. This allows learners to check out specific lesson states and compare their implementation with reference code.

## Coding Standards

### Language and Compatibility

- **C++ Standard**: C++20
- **Target Platforms**: Windows, macOS, and Linux
- **Build System**: CMake
- **Package Manager**: vcpkg

### Code Style

- Use clear, descriptive variable and function names
- Follow modern C++ best practices (RAII, smart pointers, etc.)
- Prefer the Vulkan C++ wrapper (vulkan.hpp) over the C API
- Keep code readable and well-commented where complexity is unavoidable
- Maintain consistency with existing code in the repository
- use trailing return type function syntax
- use camelCase for variable names, function names, and parameter names
- use PascalCase for classes, structs


### Project Organization

- Keep platform-specific code isolated and clearly marked
- Separate concerns: rendering logic, resource management, and application flow
- Use CMake for cross-platform build configuration
- Declare dependencies in vcpkg.json for reproducible builds

### Graphics Programming Practices

- Properly manage Vulkan object lifetimes
- Follow Vulkan synchronization best practices
- Clean up resources explicitly
- Validate Vulkan usage with validation layers during development
- Handle errors gracefully and provide meaningful error messages

### Documentation

- Document non-obvious design decisions
- Explain Vulkan-specific concepts as they are introduced
- Reference official Vulkan documentation where appropriate
- Keep README files updated with lesson-specific requirements
- use proper markdown footnotes in .md files
- make sure all code-blocks in .md files are annotated with the correct language

## Package Management Migration

This tutorial has been migrated from Conan to vcpkg as the dependency management solution. vcpkg provides better cross-platform support, native CMake integration, and a more streamlined setup process. This change affects how dependencies are declared and managed throughout the tutorial lessons.

The migration ensures that users have a consistent experience across all supported platforms without requiring Python or additional tooling beyond CMake and vcpkg.

---

**Last Updated**: February 22, 2026
