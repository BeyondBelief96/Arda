#pragma once

#include <memory>
#include <string>
#include <arda/renderer/GraphicsApi.h>

class GraphicsWindow;

namespace arda::renderer {

    struct DeviceLimits {
        int maximumNumberOfVertexAttributes = 0;
        int numberOfTextureUnits = 0;
        int maximumNumberOfColorAttachments = 0;
    };

    /**
     * Creates resources that can be used/shared by every graphics window.
     */
    class Device {
        public:
            Device(const Device&) = delete;
            Device& operator=(const Device&) = delete;
            virtual ~Device() = default;

            virtual GraphicsApi Api() const = 0;

            const DeviceLimits& Limits() const {
                return m_limits;
            }

            const std::unique_ptr<GraphicsWindow> CreateGraphicsWindow(
                int width, int height, const std::string& title, WindowType
                type = WindowType::Default);
        protected:
            Device() = default;

            void SetLimits(const DeviceLimits& limits) {
                m_limits = limits;
            }

            virtual std::unique_ptr<GraphicsWindow> DoCreateGraphicsWindow(
                int width, int height, const std::string& title, WindowType type) = 0;
        private:
            DeviceLimits m_limits;
    };

    bool IsGraphicsApiAvailable(GraphicsApi api);

    std::unique_ptr<Device> CreateDevice(GraphicsApi api);
}
