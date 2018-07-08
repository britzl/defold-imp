# Imp
Defold native extension to perform image processing on an image [by doing a convolution between a kernel and the image](https://www.wikiwand.com/en/Kernel_%28image_processing%29).

## Installation
You can use Imp in your own project by adding this project as a [Defold library dependency](https://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

https://github.com/britzl/defold-imp/archive/master.zip

Or point to the ZIP file of a [specific release](https://github.com/britzl/defold-imp/releases).

## Usage

#### imp.convolution(source, destination, width, height, kernel)
Perform a convolution between the kernel and the pixels of the source buffer and write the resulting values to the destination buffer. Edge handling is done by extending border pixels.

**PARAMETERS**
* ```source``` (buffer) - Source buffer to read pixels from (must contain a stream named "rgb" of type uint8)
* ```destination``` (buffer) - Destination buffer to write pixels to (must contain a stream named "rgb" of type uint8)
* ```width``` (number) - Width of source and destination buffers
* ```height``` (number) - Height of source and destination buffers
* ```kernel``` (table) - Table with 9 numbers describing the kernel. First value is top left, last value is bottom right.

## Example
There's an example in the examples folder. The example will use the camera if running on iOS or OSX and on other systems it will grab an image from Imgur.

[Try the HTML5 version (with Imgur image)](https://britzl.github.io/Imp/).
