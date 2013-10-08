
#ifndef _COWLIBRARY_H_
#define _COWLIBRARY_H_

#include <stdexcept>
#include <string>
#include <dlfcn.h>

namespace cow {

/** Class for dynamic class-loading. */
template <class T>
class Library
{
public:
    /** Constructor. Does nothing. */
    Library(): m_lib(0), m_class(0) { }

    /** Load the class from a file
     * @param filename The name of the library.
     * @param path Path of the file. Default is "./"
     * @throw std::runtime_error if library cannot be loaded
     * @throw std::logic_error if library is already loaded (and is not unloaded) */
    void load(const std::string &filename, const std::string &path = "./")
        throw(std::runtime_error, std::logic_error)
    {
        if(m_lib != 0)
            throw std::logic_error("Library already loaded");

        m_name = filename;
        m_path = path;

        std::string file = m_path + m_name;
        m_lib = dlopen(file.c_str(), RTLD_NOW);
        if(m_lib == 0)
            throw std::runtime_error(dlerror());

        typedef T* (*Func) ();
        Func class_create = (Func)dlsym(m_lib, "class_create");

        if(class_create == 0) {
            std::string error = dlerror();
            dlclose(m_lib);
            m_lib = 0;
            throw std::runtime_error(error);
        }

        m_class = class_create();
        if(m_class == 0) {
            throw std::runtime_error("Error in creating the class");
        }
    }

    void load()
        throw(std::runtime_error, std::logic_error)
    {
        load(m_name, m_path);
    }

    /**  Unloads the library and frees allocated memory. */
    void unload() throw() {
        delete m_class;
        m_class = 0;

        if(m_lib != 0) {
            dlclose(m_lib);
            m_lib = 0;
        }
    }

    /** Get the name of the library. */
    const std::string &get_name() const { return m_name; }

    /** Get the path of the library. */
    const std::string &get_path() const { return m_path; }

    /** Smart pointer dereferencing. */
    T* operator ->() const { return m_class; }

    /** Smart pointer dereferencing. */
    T& operator *() const { return *m_class; }

    /** Get the loaded class. */
    T* get() const { return m_class; }

    /** Destructor. Calls unload. */
    ~Library() throw() { unload(); }
private:
    void *m_lib;
    T *m_class;
    std::string m_name;
    std::string m_path;
};

} // namespace cow

#endif // _COWLIBRARY_H_

