#include <stdint.h>
#include <memory>
#include <limits>

#ifndef buffer_t_defined
#define buffer_t_defined
#include <stdint.h>
typedef struct buffer_t {
  uint8_t* host;
  uint64_t dev;
  bool host_dirty;
  bool dev_dirty;
  size_t dims[4];
  size_t elem_size;
} buffer_t;
#endif

extern "C" void __copy_to_host(buffer_t* buf);

template<typename T>
class Image {
    struct Contents {
        buffer_t buf;
        uint8_t *alloc;
        ~Contents() {
            delete[] alloc;
        }        
    };

    std::shared_ptr<Contents> contents;

    void initialize(int w, int h, int c) {
        buffer_t buf;
        buf.dims[0] = w;
        buf.dims[1] = h;
        buf.dims[2] = c;
        buf.dims[3] = 1;
        buf.elem_size = sizeof(T);

        uint8_t *ptr = new uint8_t[sizeof(T)*w*h*c+16];
        buf.host = ptr;
        buf.host_dirty = false;
        buf.dev_dirty = false;
        buf.dev = 0;
        while ((size_t)buf.host & 0xf) buf.host++; 
        contents.reset(new Contents {buf, ptr});
    }

public:
    Image() {
    }
    
    Image(int w, int h = 1, int c = 1) {
        initialize(w, h, c);
    }

    T *data() {return (T*)contents->buf.host;}

    Image(std::initializer_list<T> l) {
        initialize(l.size(), 1, 1);
        int x = 0;
        for (auto &iter: l) {
            (*this)(x++, 0, 0) = iter;
        }
    }

    Image(std::initializer_list<std::initializer_list<T> > l) {
        initialize(l.begin()->size(), l.size(), 1);
        int y = 0;
        for (auto &row: l) {
            int x = 0;
            for (auto &elem: row) {
                (*this)(x++, y, 0) = elem;
            }
            y++;
        }
    }


    T &operator()(int x, int y = 0, int c = 0) {
        // copy back if needed
        if (contents->buf.dev_dirty)
            __copy_to_host(&contents->buf);
        
        // mark host dirty
        contents->buf.host_dirty = true;

        T *ptr = (T *)contents->buf.host;
        int w = contents->buf.dims[0];
        int h = contents->buf.dims[1];
        return ptr[(c*h + y)*w + x];
    }

    const T &operator()(int x, int y = 0, int c = 0) const {
        if (contents->buf.dev_dirty)
            __copy_to_host(&contents->buf);

        const T *ptr = (const T *)contents->buf.host;
        int w = contents->buf.dims[0];
        int h = contents->buf.dims[1];
        return ptr[(c*h + y)*w + x];
    }

    operator buffer_t *() {
        return &(contents->buf);
    }

    int width() {
        return contents->buf.dims[0];
    }

    int height() {
        return contents->buf.dims[1];
    }

    int channels() {
        return contents->buf.dims[2];
    }

};

