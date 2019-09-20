#include <cstdint>
#include <vector>
#include <thread>
#include <glad/glad.h>

class Framebuffer {

private:
    uint32_t _width{0};
    uint32_t _height{0};
    uint32_t _fbo{0};
    uint32_t _tex{0};
    mutable std::vector<float> _image_buffer;
    mutable std::vector<float> _red_buffer;
    mutable std::vector<float> _blue_buffer;
    mutable std::vector<float> _green_buffer;
    mutable std::thread _thread;

public:
    Framebuffer(uint32_t width, uint32_t height)
        : _width{width}, _height{height} {
        
        auto fbo = 0u;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        
        auto tex = 0u;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _width, _height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "G-Buffer Framebuffer not complete!" << std::endl;
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        _image_buffer.resize(_width * _height * 3);
        _red_buffer.resize(_width * _height);
        _green_buffer.resize(_width * _height);
        _blue_buffer.resize(_width * _height);
        
        _fbo = fbo;
        _tex = tex;
    }
    
    ~Framebuffer() noexcept {
        if (_thread.joinable()) {
            _thread.join();
        }
    }
    
    template<typename F>
    void with(F &&render) {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _width, _height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(static_cast<uint32_t>(GL_COLOR_BUFFER_BIT) | static_cast<uint32_t>(GL_DEPTH_BUFFER_BIT));
        render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    uint32_t texture() const noexcept { return _tex; }
    
    void save(const std::string &path) {
        
        if (_thread.joinable()) {
            _thread.join();
        }
        
        glBindTexture(GL_TEXTURE_2D, _tex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, _image_buffer.data());
        
        _thread = std::thread{[this, path] {
            
            for (auto y = 0; y < _height; y++) {
                for (auto x = 0; x < _width; x++) {
                    _red_buffer[(_height - 1 - y) * _width + x] = _image_buffer[(y * _width + x) * 3 + 0];
                    _green_buffer[(_height - 1 - y) * _width + x] = _image_buffer[(y * _width + x) * 3 + 1];
                    _blue_buffer[(_height - 1 - y) * _width + x] = _image_buffer[(y * _width + x) * 3 + 2];
                }
            }
            
            EXRHeader header;
            InitEXRHeader(&header);
            
            EXRImage image;
            InitEXRImage(&image);
            
            float *image_ptrs[]{_blue_buffer.data(), _green_buffer.data(), _red_buffer.data()};
            image.images = reinterpret_cast<uint8_t **>(image_ptrs);
            image.width = _width;
            image.height = _height;
            
            EXRChannelInfo channel_infos[3];
            header.num_channels = 3;
            header.channels = channel_infos;
            header.channels[0].name[0] = 'B';
            header.channels[0].name[1] = '\0';
            header.channels[1].name[0] = 'G';
            header.channels[1].name[1] = '\0';
            header.channels[2].name[0] = 'R';
            header.channels[2].name[1] = '\0';
            
            int32_t pixel_types[3]{TINYEXR_PIXELTYPE_FLOAT, TINYEXR_PIXELTYPE_FLOAT, TINYEXR_PIXELTYPE_FLOAT};
            header.pixel_types = pixel_types;
            header.requested_pixel_types = pixel_types;
            
            const char *err = nullptr;
            if (SaveEXRImageToFile(&image, &header, path.c_str(), &err) != TINYEXR_SUCCESS) {
                std::cerr << "Failed to save EXR: " << err << std::endl;
                FreeEXRErrorMessage(err);
            }
        }};
    }
    
};
