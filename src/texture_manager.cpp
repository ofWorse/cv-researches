#include "texture_manager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <algorithm>

bool TextureManager::loadTextureFromFile(const std::string& path, GLuint& outTexture, int& outWidth, int& outHeight) 
{
    int image_width = 0, image_height = 0;

    unsigned char* imageData = stbi_load(
        path.c_str(), 
        &image_width, 
        &image_height, 
        nullptr, 
        4
    );

    if(!imageData) 
    {
        std::cerr << "Failed to load texture: " << path << stbi_failure_reason() << '\n';
        return false;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA, 
        image_width, 
        image_height, 
        0, 
        GL_RGBA, 
        GL_UNSIGNED_BYTE, 
        imageData
    );

    stbi_image_free(imageData);
    glBindTexture(GL_TEXTURE_2D, 0);

    outTexture = textureID;
    outWidth = image_width;
    outHeight = image_height;

    return true;
}

std::string TextureManager::generateKey(const std::string& path) const 
{
    size_t lastSlash = path.find_last_of("/\\");
    size_t lastDot = path.find_last_of('.');

    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    return (lastDot == std::string::npos) ? filename : filename.substr(0, lastDot);
}

bool TextureManager::loadTexture(const std::string& path, const std::string& key) 
{
    std::string finalKey = key.empty() ? generateKey(path) : key;

    if(hasTexture(finalKey)) 
    {
        std::cout << "Texture already loaded with key '" << finalKey << '\n';
        return true;
    }
    
    auto texture = std::make_unique<Texture>();
    texture->path = path;

    if(!loadTextureFromFile(path, texture->id, texture->width, texture->height)) 
    {
        return false;
    }

    std::cout << "Loaded texture: " << path <<
       " (Key: " << finalKey << ", ID: " <<
       texture->id << ", Size: " << texture->width <<
       "x" << texture->height << ")\n";

    textures[finalKey] = std::move(texture);
    
    return true;
}

Texture* TextureManager::getTexture(const std::string& key) const 
{
    auto it = textures.find(key);
    return (it != textures.end()) ? it->second.get() : nullptr;
}

void TextureManager::unloadTexture(const std::string& key) 
{
    textures.erase(key);
}

void TextureManager::clear(void) 
{
    textures.clear();
}

bool TextureManager::hasTexture(const std::string& key) const 
{
    return textures.find(key) != textures.end();
}