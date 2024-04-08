## :page_facing_up: HTML Generation for Embedded Systems

### :file_folder: Table of Contents
- [Introduction](#introduction)
- [Requirements](#requirements)
- [Code Overview](#code-overview)
  - [File Structure](#file-structure)
  - [Template Files](#template-files)
  - [HTML Compilation](#html-compilation)
- [Usage Example](#usage-example)
- [Tips and Tricks](#tips-and-tricks)

### :bulb: Introduction
This codebase provides a robust and efficient way to generate HTML content for embedded systems. By leveraging templates and a dynamic compilation process, it enables developers to easily create device-specific web pages without the need for runtime HTML rendering.

### :heavy_check_mark: Requirements
- Arduino IDE
- C++ programming language knowledge

### 🗺️ Code Overview
#### 🌐 File Structure
- `src/html.h`: Header file containing generated HTML content.
- `<html_files/*.html>`: Individual HTML files containing web page content.
- `<tpl_files/*.tpl>`: Template files providing reusable HTML components.

#### 🧩 Template Files
Template files allow for the inclusion of common HTML elements in multiple HTML files. For example, a `header.tpl` file could contain:

```html
<header>
  <h1>My Embedded Device</h1>
</header>
```

#### 🛠️ HTML Compilation
The `html.h` header file is generated dynamically by the `html-generator.py` script, which:
- Reads all HTML and template files.
- Replaces template placeholders with actual content.
- Escapes special characters for use in C++ code.
- Generates a `const` character array for each HTML file.

### 💻 Usage Example
```c++
#include "../src/html.h"

void setup() {
  // Send HTML content to a web server using `WiFiClient` or other communication library.
  client.write((const uint8_t *)PAGE_HOME, sizeof(PAGE_HOME));
}
```

### 💡 Tips and Tricks
- Use unique names for template placeholders to avoid conflicts.
- Minimize whitespace and line breaks in HTML files to reduce code size.
- Consider using minification tools to further reduce code footprint.
- For complex HTML pages, create multiple template files for organization and maintainability.