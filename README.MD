[![C CI](https://github.com/fergul/chibiWebC/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/fergul/chibiWebC/actions/workflows/c-cpp.yml)

ChibiWebC
=========

ChibiWebC is an adventure in writing a small web-server in C.

Current target features are:
 - [x] Multi-threading
 - [x] GET request parameter support
 - [ ] POST request support
 - [x] Path recognition support with callback handlers
 - [x] File serving (basic)
 - [ ] HTML Templating

Design decisions
----------------
As the project develops many decisions must be made on the use of data structures or algorithms. My current approach is to keep it simple, clean and iterate as the requirements change e.g., need more speeeeeed.

Warning!
--------
This code is a work-in-progress and not designed for external facing web-servers.
