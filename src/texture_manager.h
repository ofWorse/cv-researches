#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <memory>

struct Texture
{
    GLuint id;
    int width;
    int height;
    std::string path;

    Texture(void) : id(0), width(0), height(0) {}
    ~Texture(void) { if (id) glDeleteTextures(1, &id); }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept 
        : id(other.id)
        , width(other.width)
        , height(other.height)
        , path(std::move(other.path))
    {
        other.id = 0;
    }
};


class TextureManager
{
public:
    static TextureManager& getInstance(void) 
    {
        static TextureManager instance;
        return instance;
    }

    bool loadTexture(const std::string& path, const std::string& key = "");
    Texture* getTexture(const std::string& key) const;
   
    void unloadTexture(const std::string& key);
    void clear(void);

    bool hasTexture(const std::string& key) const;

private:
    TextureManager(void) = default;
    ~TextureManager(void) { clear(); }

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;

    bool loadTextureFromFile(const std::string& path, GLuint& outTexture, int& outWidth, int& outHeight);

    std::string generateKey(const std::string& path) const;
};



#endif // TEXTURE_MANAGER_H