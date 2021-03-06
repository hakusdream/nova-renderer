package com.continuum.nova;

import com.sun.jna.*;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.Arrays;
import java.util.List;
import java.nio.IntBuffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public interface NovaNative extends Library {
    NovaNative INSTANCE = (NovaNative) Native.loadLibrary("nova-renderer", NovaNative.class);

    Logger LOG = LogManager.getLogger(NovaNative.class);

    enum NovaVertexFormat {
        POS,
        POS_UV,
        POS_UV_LIGHTMAPUV_NORMAL_TANGENT,
        POS_UV_COLOR
    }

    class mc_atlas_texture extends Structure {
        public int width;
        public int height;
        public int num_components;
        public Pointer texture_data;
        public String name;

        public mc_atlas_texture(int width, int height, int num_components, byte[] texture_data) {
            this.width = width;
            this.height = height;
            this.num_components = num_components;

            this.texture_data = new Memory(width * height * num_components * Native.getNativeSize(Byte.TYPE));
            for(int i = 0; i < width * height * num_components; i++) {
                this.texture_data.setByte(i, texture_data[i]);
            }
        }

        public void setName(String name) {
            this.name = name;
        }

        @Override
        public List<String> getFieldOrder() {
            return Arrays.asList("width", "height", "num_components", "texture_data");
        }

        @Override
        public String toString() {
            return "mc_atlas_texture{" +
                    "width=" + width +
                    ", height=" + height +
                    ", num_components=" + num_components +
                    ", name='" + name + '\'' +
                    '}';
        }
    }

    class mc_texture_atlas_location extends Structure {
        public String name;
        public float min_u;
        public float max_u;
        public float min_v;
        public float max_v;

        public mc_texture_atlas_location(String name, float min_u, float min_v, float max_u, float max_v) {
            this.name = name;
            this.min_u = min_u;
            this.max_u = max_u;
            this.min_v = min_v;
            this.max_v = max_v;
        }

        @Override
        public List<String> getFieldOrder() {
            return Arrays.asList("name", "min_u", "max_u", "min_v", "max_v");
        }

        @Override
        public String toString() {
            return "mc_texture_atlas_location{" +
                    "name='" + name + '\'' +
                    ", min_u=" + min_u +
                    ", max_u=" + max_u +
                    ", min_v=" + min_v +
                    ", max_v=" + max_v +
                    '}';
        }
    }

    class mc_chunk_render_object extends Structure {
        public int format;
        public float x;
        public float y;
        public float z;
        public int id;
        public Pointer vertex_data; // int[]
        public Pointer indices;     // int[]
        public int vertex_buffer_size;
        public int index_buffer_size;

        public void setVertex_data(IntBuffer vertexData) {
            int s = vertexData.limit();
            Memory vertex_datam = new Memory(s * Native.getNativeSize(Integer.class));
            ByteBuffer bbuf = vertex_datam.getByteBuffer(0, s * Native.getNativeSize(Integer.class));
            ByteBuffer bbuf2 = ByteBuffer.allocate(s * Native.getNativeSize(Integer.class));
            bbuf.position(0);
            vertexData.position(0);
            bbuf.asIntBuffer().put(vertexData);
            bbuf.position(0);
            vertex_data = vertex_datam;

            vertex_buffer_size = s;
        }

        public void setIndices(List<Integer> indices) {
            this.indices = new Memory(indices.size() * Native.getNativeSize(Integer.class));
            for(int i = 0; i < indices.size(); i++) {
                Integer data = indices.get(i);
                this.indices.setInt(i * Native.getNativeSize(Integer.TYPE), data);
            }

            index_buffer_size = indices.size();
        }

        @Override
        public List<String> getFieldOrder() {
            return Arrays.asList("format", "x", "y", "z", "id", "vertex_data", "indices", "vertex_buffer_size", "index_buffer_size");
        }
    }

    class mc_settings extends Structure {
        public boolean render_menu;

        public boolean anaglyph;

        public double fog_color_red;
        public double fog_color_green;
        public double fog_color_blue;

        public int display_width;
        public int display_height;

        public boolean view_bobbing;
        public int should_render_clouds;

        public int render_distance;

        public boolean has_blindness;

        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList(
                    "render_menu", "anaglyph", "fog_color_red", "fog_color_green", "fog_color_blue", "display_width",
                    "display_height", "view_bobbing", "should_render_clouds", "render_distance",
                    "has_blindness"
            );
        }
    }

    class mc_gui_buffer extends Structure {
        public String texture_name;
        public int index_buffer_size;
        public int vertex_buffer_size;
        public Pointer index_buffer; // int[]
        public Pointer vertex_buffer; // float[]
        public String atlas_name;

        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("particle_texture", "index_buffer_size", "vertex_buffer_size", "index_buffer", "vertex_buffer", "sprite_name");
        }
    }

    class mouse_button_event extends Structure implements Structure.ByValue {
        public int button;
        public int action;
        public int mods;
        public int filled;
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("button", "action", "mods","filled");
        }
    }

    class mouse_position_event extends Structure implements Structure.ByValue {
        public int xpos;
        public int ypos;
        public int filled;
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("xpos", "ypos","filled");
        }
    }

    class mouse_scroll_event extends Structure implements Structure.ByValue {
        public double xoffset;
        public double yoffset;
        public int filled;
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("xoffset", "yoffset","filled");
        }
    }

    class key_press_event extends Structure implements Structure.ByValue {
        public int key;
        public int scancode;
        public int action;
        public int mods;
        public int filled;
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("key", "scancode", "action","mods","filled");
        }
    }

    class key_char_event extends Structure implements Structure.ByValue {
        public long unicode_char;
        public int filled;
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("unicode_char","filled");
        }
    }

    class window_size extends Structure implements Structure.ByValue{
        public int height;
        public int width;

        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("height","width");
        }
    }

    enum GeometryType {
        BLOCK,
        ENTITY,
        FALLING_BLOCK,
        GUI,
        CLOUD,
        SKY_DECORATION,
        SELECTION_BOX,
        GLINT,
        WEATHER,
        HAND,
        FULLSCREEN_QUAD,
        PARTICLE,
        LIT_PARTICLE,
        EYES
    }

    enum NativeBoolean{
        FALSE,
        TRUE
    }

    void initialize();

    void execute_frame();

    void send_lightmap_texture(int[] data, int length, int width, int height);

    void add_texture(mc_atlas_texture texture);

    void add_texture_location(mc_texture_atlas_location location);

    int get_max_texture_size();

    void reset_texture_manager();

    void add_chunk_geometry_for_filter(String filter_name, mc_chunk_render_object render_object);

    void remove_chunk_geometry_for_filter(String filter_name, mc_chunk_render_object render_object);

    boolean should_close();

    void add_gui_geometry(mc_gui_buffer buffer);

    void clear_gui_buffers();

    void set_mouse_grabbed(boolean grabbed);

    mouse_button_event get_next_mouse_button_event();

    mouse_position_event get_next_mouse_position_event();

    mouse_scroll_event get_next_mouse_scroll_event();

    key_press_event get_next_key_press_event();

    key_char_event get_next_key_char_event();

    window_size get_window_size();

    void set_fullscreen(int fullscreen);

    boolean display_is_active();

    void set_string_setting(String setting, String value);

    void set_float_setting(String setting_name, float setting_value);

    void set_player_camera_transform(double x, double y, double z, float yaw, float pitch);

    String get_shaders_and_filters();
}
