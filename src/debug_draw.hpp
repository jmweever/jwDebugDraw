#pragma once

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>

using namespace godot;

namespace JW
{
	class JWDebugDraw : public Object
	{
		GDCLASS( JWDebugDraw, Object ) // NOLINT

		public:
			enum DrawFlags : std::uint32_t
			{
				DRAW_FLAGS_NONE            = 0U,
				DRAW_FLAGS_FILLED          = 1U << 0, // 1
				DRAW_FLAGS_BACKGROUND      = 1U << 1, // 2
				DRAW_FLAGS_SILHOUETTE_ONLY = 1U << 2, // 4
				DRAW_FLAGS_SCREEN_SPACE    = 1U << 3  // 8
			};

			enum TextFlags : std::uint32_t
			{
				TEXT_FLAGS_NONE       = 0U,
				TEXT_FLAGS_FIXED_SIZE = 1U << 0, // 1
				TEXT_FLAGS_RELATIVE   = 1U << 1, // 2
				TEXT_FLAGS_APPEND     = 1U << 2  // 4
			};

			static inline float THICKNESS_DEFAULT          = 0.5f;
			static inline float BACKFACE_ALPHA_DEFAULT     = 0.2f;
			static inline bool  CONSTANT_THICKNESS_DEFAULT = true;
			static inline bool  ANTIALIASING_DEFAULT       = true;
			static inline int   FONT_SIZE                  = 14;

			static void initialize();
			static void shutdown();
			static void on_frame();

			/**
			 * @brief Draws the given string of text at the given @p position in 3D space.
			 * @param text             The string of text to draw.
			 * @param position         The @c Vector3 position to draw the text at.
			 * @param text_color       The @c Color the text will be drawn in.
			 * @param outline_color    The @c Color the outline of the text will be drawn in.
			 * @param background_color The @c Color the background of the text will be drawn in.
			 * @param flags            Optional extra config flags for the text.
			 */
			static void draw_text( const String& text, const Vector3& position, const Color& text_color = COLOR_WHITE, const Color& outline_color = COLOR_CLEAR, const Color& background_color = COLOR_CLEAR, BitField<TextFlags> flags = TEXT_FLAGS_NONE );

			/**
			 * @brief Draws the given string of text at the given @p transform in 3D space.
			 * @param text             The string of text to draw.
			 * @param transform        The @c Transform3D containing the position/rotation/scale for the text.
			 * @param text_color       The @c Color the text will be drawn in.
			 * @param outline_color    The @c Color the outline of the text will be drawn in.
			 * @param background_color The @c Color the background of the text will be drawn in.
			 * @param flags            Optional extra config flags for the text.
			 */
			static void draw_text_xform( const String& text, const Transform3D& transform, const Color& text_color = COLOR_WHITE, const Color& outline_color = COLOR_CLEAR, const Color& background_color = COLOR_CLEAR, BitField<TextFlags> flags = TEXT_FLAGS_NONE );

			/**
			 * @brief Draws the given string of text at the given <paramref name="position"/> on the screen.
			 * @param text             The string of text to draw.
			 * @param position         The @c Vector2 position to draw the text at.
			 * @param text_color       The @c Color the text will be drawn in.
			 * @param outline_color    The @c Color the outline of the text will be drawn in.
			 * @param background_color The @c Color the background of the text will be drawn in.
			 * @param flags            Optional extra config flags for the text.
			 */
			static void draw_text_2d( const String& text, const Vector2& position, const Color& text_color = COLOR_WHITE, const Color& outline_color = COLOR_CLEAR, const Color& background_color = COLOR_CLEAR, BitField<TextFlags> flags = TEXT_FLAGS_NONE );

			/**
			 * @brief Draws a point at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the point at.
			 * @param size     The visual size of the point <em>(default is 0.01)</em>.
			 * @param color    The @c Color the point will be drawn in.
			 * @param flags    Optional extra config flags for the point.
			 */
			static void draw_point( const Vector3& position, float size = 0.01f, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a line between the given @p start and @p end positions in 3D space.
			 * @param start The @c Vector3 position to start drawing the line at.
			 * @param end   The @c Vector3 position to end drawing the line at.
			 * @param color The @c Color the line will be drawn in.
			 * @param flags Optional extra config flags for the line.
			 */
			static void draw_line( const Vector3& start, const Vector3& end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a ray starting at the given @p origin and point in the given @p direction in 3D space.
			 * @param origin    The @c Vector3 position to start drawing the ray at.
			 * @param direction The @c Vector3 direction the ray will draw toward.
			 * @param color     The @c Color the ray will be drawn in.
			 * @param flags     Optional extra config flags for the ray.
			 */
			static void draw_ray( const Vector3& origin, const Vector3& direction, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a triangle along the 3 given vertex positions in 3D space.
			 * @param vertex_a The @c Vector3 position of the first vertex of the triangle.
			 * @param vertex_b The @c Vector3 position of the second vertex of the triangle.
			 * @param vertex_c The @c Vector3 position of the third vertex of the triangle.
			 * @param color    The @c Color the triangle will be drawn in.
			 * @param flags    Optional extra config flags for the triangle.
			 */
			static void draw_triangle( const Vector3& vertex_a, const Vector3& vertex_b, const Vector3& vertex_c, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws an arc shape at the given @p position in 3D space.
			 * @param position    The @c Vector3 position to draw the arc at.
			 * @param axis        The @c Vector3 axis to align the arc to.
			 * @param radius      The @c float radius of the arc.
			 * @param angle_start The @c float starting angle to draw the arc at.
			 * @param angle_end   The @c float ending angle to draw the arc to.
			 * @param color       The @c Color the arc will be drawn in.
			 * @param flags       Optional extra config flags for the arc.
			 */
			static void draw_arc( const Vector3& position, const Vector3& axis, float radius, float angle_start, float angle_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a curve along a given set of @p points in 3D space.
			 * @param points   The set of points to draw the curve through.
			 * @param tension  The tension of the curve.
			 * @param segments The amount of segments that will make up the curve.
			 * @param color    The @c Color the curve will be drawn in.
			 * @param flags    Optional extra config flags for the curve.
			 */
			static void draw_curve( const TypedArray<Vector3>& points, float tension = 0.5f, int segments = 10, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a circle at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the circle at.
			 * @param axis     The @c Vector3 axis to align the circle to.
			 * @param radius   The @c float radius of the circle.
			 * @param color    The @c Color the circle will be drawn in.
			 * @param flags    Optional extra config flags for the circle.
			 */
			static void draw_circle( const Vector3& position, const Vector3& axis, float radius, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a circle with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the circle with.
			 * @param color     The @c Color the circle will be drawn in.
			 * @param flags     Optional extra config flags for the circle.
			 */
			static void draw_circle_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a box at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the box at.
			 * @param size     The @c Vector3 size of the box.
			 * @param color    The @c Color the box will be drawn in.
			 * @param flags    Optional extra config flags for the box.
			 */
			static void draw_box( const Vector3& position, const Vector3& size, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a box with the given @p transform in 3D space.
			 * @param transform The @c Transform3D where to draw the box.
			 * @param color     The @c Color the box will be drawn in.
			 * @param flags     Optional extra config flags for the box.
			 */
			static void draw_box_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a frustum shape (tapered box) at the given @p position in 3D space.
			 * @param position   The @c Vector3 position to draw the frustum at.
			 * @param size_start The @c Vector2 size of the starting end of the frustum.
			 * @param size_end   The @c Vector2 size of the ending end of the frustum.
			 * @param length     The @c float length of the frustum.
			 * @param color      The @c Color the frustum will be drawn in.
			 * @param flags      Optional extra config flags for the frustum.
			 */
			static void draw_frustum( const Vector3& position, const Vector2& size_start, const Vector2& size_end, float length, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a frustum shape (tapered box) with the given @p transform in 3D space.
			 * @param transform  The @c Transform3D to draw the frustum with.
			 * @param size_start The @c Vector2 size of the starting end of the frustum.
			 * @param size_end   The @c Vector2 size of the ending end of the frustum.
			 * @param color      The @c Color the frustum will be drawn in.
			 * @param flags      Optional extra config flags for the frustum.
			 */
			static void draw_frustum_xform( const Transform3D& transform, const Vector2& size_start, const Vector2& size_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a pyramid shape at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the pyramid at.
			 * @param size     The @c Vector2 size of the base of the pyramid.
			 * @param height   The @c float height of the pyramid.
			 * @param color    The @c Color the pyramid will be drawn in.
			 * @param flags    Optional extra config flags for the pyramid.
			 */
			static void draw_pyramid( const Vector3& position, const Vector2& size, float height, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a pyramid with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the pyramid with.
			 * @param color     The @c Color the pyramid will be drawn in.
			 * @param flags     Optional extra config flags for the pyramid.
			 */
			static void draw_pyramid_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a sphere arc shape at the given @p position in 3D space.
			 * @param position    The @c Vector3 position to draw the sphere arc at.
			 * @param radius      The @c float radius of the sphere arc.
			 * @param angle_start The @c float starting angle of the sphere arc.
			 * @param angle_end   The @c float ending angle of the sphere arc.
			 * @param color       The @c Color the sphere arc will be drawn in.
			 * @param flags       Optional extra config flags for the sphere arc.
			 */
			static void draw_sphere_arc( const Vector3& position, float radius, float angle_start, float angle_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a sphere arc shape with the given @p transform in 3D space.
			 * @param transform   The @c Transform3D to draw the sphere arc with.
			 * @param angle_start The @c float starting angle of the sphere arc.
			 * @param angle_end   The @c float ending angle of the sphere arc.
			 * @param color       The @c Color the sphere arc will be drawn in.
			 * @param flags       Optional extra config flags for the sphere arc.
			 */
			static void draw_sphere_arc_xform( const Transform3D& transform, float angle_start, float angle_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a hemisphere shape at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the hemisphere at.
			 * @param radius   The @c float radius of the hemisphere.
			 * @param up       The @c Vector3 upward direction of the hemisphere.
			 * @param color    The @c Color the hemisphere will be drawn in.
			 * @param flags    Optional extra config flags for the hemisphere.
			 */
			static void draw_hemisphere( const Vector3& position, float radius, const Vector3& up = VEC3_UP, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a sphere shape at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the sphere at.
			 * @param radius   The @c float radius of the sphere.
			 * @param color    The @c Color the sphere will be drawn in.
			 * @param flags    Optional extra config flags for the sphere.
			 */
			static void draw_sphere( const Vector3& position, float radius, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a sphere shape with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the sphere with.
			 * @param color     The @c Color the sphere will be drawn in.
			 * @param flags     Optional extra config flags for the sphere.
			 */
			static void draw_sphere_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a capsule shape at the given @p position in 3D space.
			 * @param start  The @c Vector3 position to start drawing the capsule at.
			 * @param end    The @c Vector3 position to end drawing the capsule at.
			 * @param radius The @c float radius of the capsule.
			 * @param color  The @c Color the capsule will be drawn in.
			 * @param flags  Optional extra config flags for the capsule.
			 */
			static void draw_capsule( const Vector3& start, const Vector3& end, float radius, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a capsule shape with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the capsule with.
			 * @param color     The @c Color the capsule will be drawn in.
			 * @param flags     Optional extra config flags for the capsule.
			 */
			static void draw_capsule_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a tapered capsule shape at the given @p position in 3D space.
			 * @param start        The @c Vector3 position to start drawing the tapered capsule at.
			 * @param end          The @c Vector3 position to end drawing the tapered capsule at.
			 * @param radius_start The @c float starting radius of the tapered capsule.
			 * @param radius_end   The @c float ending radius of the tapered capsule.
			 * @param color        The @c Color the tapered capsule will be drawn in.
			 * @param flags        Optional extra config flags for the tapered capsule.
			 */
			static void draw_tapered_capsule( const Vector3& start, const Vector3& end, float radius_start, float radius_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a tapered capsule shape with the given @p transform in 3D space.
			 * @param transform    The @c Transform3D to draw the tapered capsule with.
			 * @param radius_start The @c float starting radius of the tapered capsule.
			 * @param radius_end   The @c float ending radius of the tapered capsule.
			 * @param color        The @c Color the tapered capsule will be drawn in.
			 * @param flags        Optional extra config flags for the tapered capsule.
			 */
			static void draw_tapered_capsule_xform( const Transform3D& transform, float radius_start, float radius_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cylinder shape at the given @p position in 3D space.
			 * @param start  The @c Vector3 position to start drawing the cylinder at.
			 * @param end    The @c Vector3 position to end drawing the cylinder at.
			 * @param radius The @c float radius of the cylinder.
			 * @param color  The @c Color the cylinder will be drawn in.
			 * @param flags  Optional extra config flags for the cylinder.
			 */
			static void draw_cylinder( const Vector3& start, const Vector3& end, float radius, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cylinder shape with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the cylinder with.
			 * @param color     The @c Color the cylinder will be drawn in.
			 * @param flags     Optional extra config flags for the cylinder.
			 */
			static void draw_cylinder_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a tapered cylinder shape at the given @p position in 3D space.
			 * @param start        The @c Vector3 position to start drawing the tapered cylinder at.
			 * @param end          The @c Vector3 position to end drawing the tapered cylinder at.
			 * @param radius_start The @c float starting radius of the tapered cylinder.
			 * @param radius_end   The @c float ending radius of the tapered cylinder.
			 * @param color        The @c Color the tapered cylinder will be drawn in.
			 * @param flags        Optional extra config flags for the tapered cylinder.
			 */
			static void draw_tapered_cylinder( const Vector3& start, const Vector3& end, float radius_start, float radius_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a tapered cylinder shape with the given @p transform in 3D space.
			 * @param transform    The @c Transform3D to draw the tapered cylinder with.
			 * @param radius_start The @c float starting radius of the tapered cylinder.
			 * @param radius_end   The @c float ending radius of the tapered cylinder.
			 * @param color        The @c Color the tapered cylinder will be drawn in.
			 * @param flags        Optional extra config flags for the tapered cylinder.
			 */
			static void draw_tapered_cylinder_xform( const Transform3D& transform, float radius_start, float radius_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cylinder arc shape at the given @p position in 3D space.
			 * @param start       The @c Vector3 position to start drawing the cylinder arc at.
			 * @param end         The @c Vector3 position to end drawing the cylinder arc at.
			 * @param radius      The @c float radius of the cylinder arc.
			 * @param angle_start The @c float starting angle of the cylinder arc.
			 * @param angle_end   The @c float ending angle of the cylinder arc.
			 * @param color       The @c Color the cylinder arc will be drawn in.
			 * @param flags       Optional extra config flags for the cylinder arc.
			 */
			static void draw_cylinder_arc( const Vector3& start, const Vector3& end, float radius, float angle_start, float angle_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cylinder arc shape with the given @p transform in 3D space.
			 * @param transform   The @c Transform3D to draw the cylinder arc with.
			 * @param angle_start The @c float starting angle of the cylinder arc.
			 * @param angle_end   The @c float ending angle of the cylinder arc.
			 * @param color       The @c Color the cylinder arc will be drawn in.
			 * @param flags       Optional extra config flags for the cylinder arc.
			 */
			static void draw_cylinder_arc_xform( const Transform3D& transform, float angle_start, float angle_end, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cone shape at the given @p position in 3D space.
			 * @param position The @c Vector3 position to draw the cone at.
			 * @param radius   The @c float radius of the base of the cone.
			 * @param height   The @c float height of the cone.
			 * @param color    The @c Color the cone will be drawn in.
			 * @param flags    Optional extra config flags for the cone.
			 */
			static void draw_cone( const Vector3& position, float radius, float height, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws a cone shape with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the cone with.
			 * @param color     The @c Color the cone will be drawn in.
			 * @param flags     Optional extra config flags for the cone.
			 */
			static void draw_cone_xform( const Transform3D& transform, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws an arrow shape at the given @p position in 3D space.
			 * @param start       The @c Vector3 position to start drawing the arrow at.
			 * @param end         The @c Vector3 position to end drawing the arrow at.
			 * @param head_radius The @c float radius of the arrow head.
			 * @param head_length The @c float length of the arrow head.
			 * @param color       The @c Color the arrow will be drawn in.
			 * @param flags       Optional extra config flags for the arrow.
			 */
			static void draw_arrow( const Vector3& start, const Vector3& end, float head_radius = 0.15f, float head_length = -1.0f, const Color& color = COLOR_DEFAULT, BitField<DrawFlags> flags = DRAW_FLAGS_NONE );

			/**
			 * @brief Draws an axis with the given @p transform in 3D space.
			 * @param transform The @c Transform3D to draw the axis with.
			 * @param size      The @c float size to draw the axis at.
			 */
			static void draw_axis( const Transform3D& transform, float size = 1.0f );

		protected:
			static void _bind_methods();

		private:
			struct Shape
			{
				Vector3    param_a;
				float      _pad_a = 0;
				Vector3    param_b;
				float      _pad_b = 0;
				Color      color;
				Quaternion rotation;
				Vector4    params; // x: type, y: flags, z: param, w: param

				Shape() = default;

				Shape( const Vector3& param_a, const Vector3& param_b, const Color& color, const Quaternion& rotation, const Vector4& params )
					: param_a( param_a ), param_b( param_b ), color( color ), rotation( rotation ), params( params )
				{}

				[[nodiscard]] int get_type() const { return static_cast<int>( params.x ); }
				void set_type( const int type ) { params.x = static_cast<real_t>( type ); }
			};

			struct Text
			{
				bool        is_2d          = false;
				Vector2     pos_2d;
				bool        fixed_size     = false;
				String      content;
				Color       color;
				Color       outline;
				Color       background;
				bool        has_outline    = false;
				bool        has_background = false;
				bool        is_oriented    = false;
				Transform3D orientation;
			};

			static constexpr float EPS                = 0.05f;
			static constexpr float PIXEL_SCALE        = 0.01f;
			static constexpr int   INITIAL_ALLOCATION = 4096;
			static constexpr int   TEX_WIDTH          = 1024;
			static constexpr int   PIXELS_PER_SHAPE   = 5;

			static inline const auto VEC2_ZERO  = Vector2( 0, 0 );
			static inline const auto VEC2_ONE   = Vector2( 1, 1 );
			static inline const auto VEC3_ZERO  = Vector3( 0, 0, 0 );
			static inline const auto VEC3_ONE   = Vector3( 1, 1, 1 );
			static inline const auto VEC3_UP    = Vector3( 0, 1, 0 );
			static inline const auto VEC3_DOWN  = Vector3( 0,-1, 0 );
			static inline const auto VEC3_LEFT  = Vector3(-1, 0, 0 );
			static inline const auto VEC3_RIGHT = Vector3( 1, 0, 0 );

			static inline const auto COLOR_WHITE   = Color( 1, 1, 1, 1 );
			static inline const auto COLOR_BLACK   = Color( 0, 0, 0, 1 );
			static inline const auto COLOR_RED     = Color( 1, 0, 0, 1 );
			static inline const auto COLOR_GREEN   = Color( 0, 1, 0, 1 );
			static inline const auto COLOR_BLUE    = Color( 0, 0, 1, 1 );
			static inline const auto COLOR_CLEAR   = Color( 0, 0, 0, 0 );
			static inline const auto COLOR_DEFAULT = Color( 0.5961, 0.9843, 0.5961, 1 );

			static bool                _initialized;
			static SceneTree          *_scene_tree;
			static Viewport           *_viewport;
			static Camera3D           *_camera;
			static Vector3             _camera_pos;
			static float               _pixel_scale;
			static float               _line_thickness;
			static uint64_t            _last_frame;
			static Vector3             _last_cam_pos;
			static float               _last_pixel_scale;
			static float               _last_thickness;
			static bool                _shapes_dirty;
			static Vector<Shape>      *_shapes_frame;
			static Vector<Shape>      *_shapes_phys;
			static int                 _shape_num;
			static int                 _last_shape_num;
			static int                 _shape_num_frame;
			static int                 _shape_num_phys;
			static int                 _visible_instances;
			static Vector<Text>       *_texts;
			static Vector<Text>       *_texts_frame;
			static Vector<Text>       *_texts_phys;
			static int                 _text_num;
			static int                 _text_num_frame;
			static int                 _text_num_phys;
			static Vector<float>      *_sort_keys;
			static Vector<int>        *_indices;
			static uint64_t            _phys_shape_frame;
			static uint64_t            _phys_text_frame;
			static RID                *_mesh_rid;
			static RID                *_multimesh_rid;
			static RID                *_instance_rid;
			static RID                *_shader_rid;
			static RID                *_material_rid;
			static RID                *_text_canvas_rid;
			static Ref<Font>          *_debug_font;
			static Ref<Image>         *_data_image;
			static Ref<ImageTexture>  *_data_texture;
			static PackedByteArray    *_texture_bytes;
			static PackedFloat32Array *_transform_buffer;
			static int                 _allocated_count;
			static bool                _test_enabled;

			static float _calculate_pixel_scale();
			static Quaternion _rotation_from_normal( Vector3 normal );
			static Quaternion _extract_rotation( const Basis& b );
			static Vector3 _catmull_rom( Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t, float tension );
			static bool _unproject( Vector3 position, const Transform3D &view, const Projection &proj, Vector2 vp_size, Vector2 &screen_pos );
			static RID _create_unit_cube_mesh( RenderingServer *rs );
			static const Shape& _get_shape( int i );
			static void _maybe_advance_frame( const Engine *engine );
			static void _update_texture();
			static void _update_transforms();
			static void _process_transform( int buffer_index, int shape_index, const Vector3& camera_position, float safe_px_scale, float *buf );
			static void _add_shape( int type, const Vector3& param_a, const Vector3& param_b, const Color& color, const Quaternion& rotation, const Vector4& params );
			static void _add_shape( int type, const Transform3D &transform, const Color& color, const Vector4& params );
			static void _add_text( const Text &t );
			static void _update_text();
#ifdef DEBUG_ENABLED
			static void _test();
#endif
	};
}

VARIANT_BITFIELD_CAST( JW::JWDebugDraw::DrawFlags );
VARIANT_BITFIELD_CAST( JW::JWDebugDraw::TextFlags );