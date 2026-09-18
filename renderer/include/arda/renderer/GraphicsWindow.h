#pragma once

#include <functional>
#include <utility>

namespace arda::renderer {

    /**
     * Represent's a window to draw into, and owns the context used to draw into it.
     */
    class GraphicsWindow {
        public:
            using Handler = std::function<void()>;

            virtual ~GraphicsWindow() = default;
            GraphicsWindow(const GraphicsWindow&) = delete;
            GraphicsWindow& operator=(const GraphicsWindow&) = delete;

        protected:
            GraphicsWindow() = default;

            // Backends call this when the framebuffer size has changed.
            void OnResize();
        private:
            Handler m_onResize;
            Handler m_onUpdateFrame;
            Handler m_onRenderFrame;

    }
}
