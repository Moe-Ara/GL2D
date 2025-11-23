//
// DebugOverlay.hpp
//

#ifndef GL2D_DEBUGOVERLAY_HPP
#define GL2D_DEBUGOVERLAY_HPP

class DebugOverlay {
public:
    static void toggle() { s_enabled = !s_enabled; }
    static bool enabled() { return s_enabled; }
    static void render() {
        // TODO: draw bounds/triggers/camera info
    }
private:
    static bool s_enabled;
};

#endif //GL2D_DEBUGOVERLAY_HPP
