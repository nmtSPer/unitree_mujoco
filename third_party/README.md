# Third-Party Dependencies

This directory contains vendored dependencies that are required to build the
repository without relying on system-wide installs.

## unitree_sdk2

`third_party/unitree_sdk2` is a clean vendored copy of Unitree SDK2. It excludes
the source clone's `.git`, `build`, `.github`, `.devcontainer`, and cache
directories so it can be committed by the outer repository without becoming an
embedded Git repository.

Both `simulate` and `agent/cpp` prefer this vendored SDK when it exists. If it
is removed, their CMake files fall back to `find_package(unitree_sdk2)` through
`/opt/unitree_robotics/lib/cmake`.
