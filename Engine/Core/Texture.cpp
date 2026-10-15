
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



#include "Texture.hpp"
#include "../Logger.hpp"

void ImageLoader::loadImage(const std::string &path) {

  int width, height, channels;
  auto img = stbi_load(path.c_str(), &width, &height, &channels, 0);
  if (img == nullptr) {
    logError("Failed to Open File", path);
  }

  std::cout << img;  

}
