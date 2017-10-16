package com.continuum.nova;

import org.lwjgl.opengl.DisplayMode;
import org.lwjgl.opengl.PixelFormat;

import java.nio.ByteBuffer;
import java.util.List;

/**
 * A replacement for LWJGL's Display class
 *
 * @author ddubois
 * @since 16-Oct-17
 */
public class NovaDisplay {
    public static int getWidth() {
        NovaNative.window_size windowSize = NovaNative.INSTANCE.get_window_size();
        return windowSize.width;
    }

    public static int getHeight() {
        NovaNative.window_size windowSize = NovaNative.INSTANCE.get_window_size();
        return windowSize.height;
    }

    public static boolean isActive() {
        return NovaNative.INSTANCE.display_is_active();
    }

    public static void setResizable(boolean resizable) {
        NovaNative.INSTANCE.set_resizable(resizable ? 1 : 0);
    }

    public static void setTitle(String title) {
        NovaNative.INSTANCE.set_window_title(title);
    }

    public static void create(PixelFormat pixelFormat) {
        // Nova creates its own window with its own pixel format, which may or may not be what MC wants (wanna support
        // HRD monitors at some point) so I have this method so this class has the same interface but it won't do
        // anything
    }

    public static void create() {
        // Nova creates its own window so I have this method so this class has the same interface but it won't do
        // anything
    }

    public static void setFullscreen(boolean fullscreen) {
        NovaNative.INSTANCE.set_fullscreen(fullscreen ? 1 : 0);
    }

    public DisplayMode getDisplayMode() {}

    public void setDisplayMode(DisplayMode displayMode) {}

    public boolean isCreated() {}

    public boolean isCloseRequested() {}

    public void sync(int framerateLimit) {}

    public void setIcon(ByteBuffer iconBytes) {}

    public void destroy() {}

    public void setVsyncEnabled(boolean vsyncEnabled) {}

    public List<DisplayMode> getAvailableDisplayModes() {}

    public DisplayMode getDesktopDisplayMode() {}
}
