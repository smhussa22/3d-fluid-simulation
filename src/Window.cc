#include "Window.hh"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>

namespace
{

    // GLFW calls this on every scroll; it finds our Window through the user pointer and adds up the movement
    void _on_scroll(::GLFWwindow* glfw_window, double, double y_offset)
    {

        auto* window { static_cast<Window*>(::glfwGetWindowUserPointer(glfw_window)) };
        window->addScroll(y_offset);

    }

}

Window::Window(int width, int height, const char* title) : m_width { width }, m_height { height }
{

    if (!::glfwInit())
    {

        throw std::runtime_error { "glfwInit failed" };

    }

    // no OpenGL context (Direct3D draws instead) and a fixed size
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window.reset(::glfwCreateWindow(width, height, title, nullptr, nullptr));

    if (!m_window)
    {

        ::glfwTerminate();
        throw std::runtime_error { "glfwCreateWindow failed" };

    }

    // let the scroll callback find this Window (it can't move, so the pointer stays valid)
    ::glfwSetWindowUserPointer(m_window.get(), this);
    ::glfwSetScrollCallback(m_window.get(), _on_scroll);

}

Window::~Window()
{

    m_window.reset();
    ::glfwTerminate();

}

bool Window::shouldClose() const
{

    return ::glfwWindowShouldClose(m_window.get());

}

void Window::close()
{

    ::glfwSetWindowShouldClose(m_window.get(), true);

}

void Window::pollEvents()
{

    ::glfwPollEvents();

}

bool Window::key(int k) const
{

    return ::glfwGetKey(m_window.get(), k) == GLFW_PRESS;

}

bool Window::mouseButton(int button) const
{

    return ::glfwGetMouseButton(m_window.get(), button) == GLFW_PRESS;

}

std::pair<double, double> Window::cursor() const
{

    double x { 0.0 };
    double y { 0.0 };
    ::glfwGetCursorPos(m_window.get(), &x, &y);
    return { x, y };

}

double Window::takeScroll()
{

    const double scroll { m_scroll };
    m_scroll = 0.0;
    return scroll;

}

void Window::addScroll(double amount)
{

    m_scroll += amount;

}

::HWND Window::hwnd() const
{

    return ::glfwGetWin32Window(m_window.get());

}

int Window::width() const
{

    return m_width;

}

int Window::height() const
{

    return m_height;

}

::GLFWwindow* Window::handle() const
{

    return m_window.get();

}
