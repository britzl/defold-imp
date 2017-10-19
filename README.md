# Imp
Defold native extension to perform image processing on an image [by doing a convolution between a kernel and the image](https%3A%2F%2Fwww.wikiwand.com%2Fen%2FKernel_%28image_processing%29).

## Installation
You can use Imp in your own project by adding this project as a [Defold library dependency](https://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

https://github.com/britzl/defold-imp/archive/master.zip

Or point to the ZIP file of a [specific release](https://github.com/britzl/defold-imp/releases).

## Usage

#### imp.apply(source, destination, width, height, kernel)

Apply a kernel to the pixels of the source buffer and write the resulting values to the destination buffer.

**PARAMETERS**
* ```source``` (buffer) - Source buffer to read pixels from
* ```destination``` (buffer) - Destination buffer to write pixels to
* ```width``` (number) - Width of source and destination buffers
* ```height``` (number) - Height of source and destination buffers
* ```kernel``` (table) - Table with 9 numbers describing the kernel. First value is top left, last value is bottom right.
