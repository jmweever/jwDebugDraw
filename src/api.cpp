#include <godot_cpp/classes/viewport.hpp>
#include "utils.hpp" // IWYU pragma: keep
#include "debug_draw.hpp"

namespace JW
{
	void JWDebugDraw::draw_text( const String& text, const Vector3& position, const Color& text_color, const Color& outline_color, const Color& background_color, const BitField<TextFlags> flags )
	{
		ERR_FAIL_COND( !_initialized || !_viewport );

		_add_text( Text {
			false,                                              // is_2d
			Vector2(),                                          // pos_2d
			static_cast<bool>( flags & TEXT_FLAGS_FIXED_SIZE ), // fixed_size
			text,                                               // content
			text_color,                                         // color
			outline_color,                                      // outline
			background_color,                                   // background
			! outline_color.is_equal_approx( COLOR_CLEAR ),     // has_outline
			! background_color.is_equal_approx( COLOR_CLEAR ),  // has_background
			false,                                              // is_oriented
			Transform3D( Basis(), position )                    // orientation
		} );
	}

	void JWDebugDraw::draw_text_xform( const String& text, const Transform3D& transform, const Color& text_color, const Color& outline_color, const Color& background_color, const BitField<TextFlags> flags )
	{
		ERR_FAIL_COND( !_initialized || !_viewport );

		_add_text( Text {
			false,                                              // is_2d
			Vector2(),                                          // pos_2d
			static_cast<bool>( flags & TEXT_FLAGS_FIXED_SIZE ), // fixed_size
			text,                                               // content
			text_color,                                         // color
			outline_color,                                      // outline
			background_color,                                   // background
			! outline_color.is_equal_approx( COLOR_CLEAR ),     // has_outline
			! background_color.is_equal_approx( COLOR_CLEAR ),  // has_background
			true,                                               // is_oriented
			transform                                           // orientation
		} );
	}

	void JWDebugDraw::draw_text_2d( const String& text, const Vector2& position, const Color& text_color, const Color& outline_color, const Color& background_color, const BitField<TextFlags> flags )
	{
		ERR_FAIL_COND( !_initialized || !_viewport );

		if ( static_cast<bool>( flags & TEXT_FLAGS_APPEND ) )
		{
			const bool   is_physics = Engine::get_singleton()->is_in_physics_frame();
			const int    &count     = is_physics ? _text_num_phys : _text_num_frame;
			Vector<Text> &arr       = is_physics ? *_texts_phys : *_texts_frame;

			if ( count > 0 )
			{
				arr.ptrw()[ count - 1 ].content += "\n" + text;
				return;
			}
		}

		_add_text( Text {
			true,                                               // is_2d
			position * ( static_cast<bool>( flags & TEXT_FLAGS_RELATIVE ) ? _viewport->get_visible_rect().get_size() : VEC2_ONE ), // pos_2d
			static_cast<bool>( flags & TEXT_FLAGS_FIXED_SIZE ), // fixed_size
			text,                                               // content
			text_color,                                         // color
			outline_color,                                      // outline
			background_color,                                   // background
			! outline_color.is_equal_approx( COLOR_CLEAR ),     // has_outline
			! background_color.is_equal_approx( COLOR_CLEAR ),  // has_background
			false,                                              // is_oriented
			Transform3D()                                       // orientation
		} );
	}

	void JWDebugDraw::draw_point( const Vector3& position, const float size, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_sphere( position, size, color, flags | DRAW_FLAGS_FILLED | DRAW_FLAGS_SCREEN_SPACE );
	}

	void JWDebugDraw::draw_line( const Vector3& start, const Vector3& end, const Color& color, [[maybe_unused]] const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape( 2, start, end, color, Quaternion(), Vector4( 2, 0, 0, 0 ) );
	}

	void JWDebugDraw::draw_ray( const Vector3& origin, const Vector3& direction, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_arrow( origin, origin + direction, 0.06f, 0.16f, color, flags );
	}

	void JWDebugDraw::draw_triangle( const Vector3& vertex_a, const Vector3& vertex_b, const Vector3& vertex_c, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			6,
			vertex_a,
			vertex_b,
			color,
			Quaternion( vertex_c.x, vertex_c.y, vertex_c.z, 0 ),
			Vector4( 6, static_cast<float>( static_cast<std::uint32_t>( flags ) ), 0, 0 )
		);
	}

	void JWDebugDraw::draw_arc( const Vector3& position, const Vector3& axis, const float radius, const float angle_start, const float angle_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			3,
			position,
			Vector3( radius, 0, 0 ),
			color,
			_rotation_from_normal( axis ),
			Vector4( 3, angle_start, angle_end, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_curve( const TypedArray<Vector3>& points, const float tension, const int segments, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const int size = static_cast<int>( points.size() );

		if ( size < 2 ) return;

		const auto sf = static_cast<float>( segments );

		for ( int i = 0; i < size - 1; i++ )
		{
			const Vector3 p0 = i > 0 ? static_cast<Vector3>( points[ i - 1 ] ) : static_cast<Vector3>( points[ i ] );
			const auto    p1 = static_cast<Vector3>( points[ i ] );
			const auto    p2 = static_cast<Vector3>( points[ i + 1 ] );
			const Vector3 p3 = i < size - 2 ? static_cast<Vector3>( points[ i + 2 ] ) : static_cast<Vector3>( points[ i + 1 ] );

			for ( int j = 0; j < segments; j++ )
			{
				const auto jf = static_cast<float>( j );

				draw_line(
					_catmull_rom( p0, p1, p2, p3, jf / sf, tension ),
					_catmull_rom( p0, p1, p2, p3, ( jf + 1 ) / sf, tension ),
					color,
					flags
				);
			}
		}
	}

	void JWDebugDraw::draw_circle( const Vector3& position, const Vector3& axis, const float radius, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_arc( position, axis, radius, 0, 360, color, flags );
	}

	void JWDebugDraw::draw_circle_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_arc( transform.origin, transform.basis.get_column(2), transform.basis.get_column(0).length(), 0, 360, color, flags );
	}

	void JWDebugDraw::draw_box( const Vector3& position, const Vector3& size, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			1,
			position,
			size,
			color,
			Quaternion(),
			Vector4( 1, size.x * 0.5f, size.y * 0.5f, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_box_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 scale = transform.basis.get_scale();

		_add_shape(
			1,
			transform,
			color,
			Vector4( 1, scale.x * 0.5f, scale.y * 0.5f, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_frustum( const Vector3& position, const Vector2& size_start, const Vector2& size_end, const float length, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			1,
			position,
			Vector3( size_start.x, size_start.y, length ),
			color,
			Quaternion(),
			Vector4( 1, size_end.x * 0.5f, size_end.y * 0.5f, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_frustum_xform( const Transform3D &transform, const Vector2& size_start, const Vector2& size_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 sc = transform.basis.get_scale();

		_add_shape(
			1,
			transform.origin,
			Vector3( size_start.x * sc.x, size_start.y * sc.y, sc.z ),
			color,
			_extract_rotation( transform.basis ),
			Vector4( 1, size_end.x * sc.x * 0.5f, size_end.y * sc.y * 0.5f, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_pyramid( const Vector3& position, const Vector2& size, const float height, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			1,
			position,
			Vector3( size.x, size.y, height ),
			color,
			Quaternion( VEC3_RIGHT, -Math_PI / 2.0f ),
			Vector4( 1, 0, 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_pyramid_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			1,
			transform,
			color,
			Vector4( 1, 0, 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_sphere_arc( const Vector3& position, const float radius, const float angle_start, const float angle_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			0,
			position,
			Vector3( radius, 0, 0 ),
			color,
			Quaternion(),
			Vector4( 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ), angle_start, angle_end )
		);
	}

	void JWDebugDraw::draw_sphere_arc_xform( const Transform3D &transform, const float angle_start, const float angle_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			0,
			transform.origin,
			Vector3( transform.basis.get_scale().x, 0, 0 ),
			color,
			_extract_rotation( transform.basis ),
			Vector4( 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ), angle_start, angle_end )
		);
	}

	void JWDebugDraw::draw_hemisphere( const Vector3& position, const float radius, const Vector3& up, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			0,
			position,
			Vector3( radius, 0, 0 ),
			color,
			_rotation_from_normal( up ),
			Vector4( 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ), 0, 180 )
		);
	}

	void JWDebugDraw::draw_sphere( const Vector3& position, const float radius, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_sphere_arc( position, radius, 0, 360, color, flags );
	}

	void JWDebugDraw::draw_sphere_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		draw_sphere_arc_xform( transform, 0, 360, color, flags );
	}

	void JWDebugDraw::draw_capsule( const Vector3& start, const Vector3& end, const float radius, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			4,
			start,
			end,
			color,
			Quaternion(),
			Vector4( 4, radius, radius, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_capsule_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u = transform.basis.get_column(1) * 0.5f;
		const float   s = transform.basis.get_scale().x;

		_add_shape(
			4,
			transform.origin - u,
			transform.origin + u,
			color,
			_extract_rotation( transform.basis ),
			Vector4( 4, s, s, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_tapered_capsule( const Vector3& start, const Vector3& end, const float radius_start, const float radius_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			4,
			start,
			end,
			color,
			Quaternion(),
			Vector4( 4, radius_start, radius_end, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_tapered_capsule_xform( const Transform3D &transform, const float radius_start, const float radius_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u  = transform.basis.get_column(1) * 0.5f;
		const float   sc = transform.basis.get_scale().x;

		_add_shape(
			4,
			transform.origin - u,
			transform.origin + u,
			color,
			_extract_rotation( transform.basis ),
			Vector4( 4, radius_start * sc, radius_end * sc, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_cylinder( const Vector3& start, const Vector3& end, const float radius, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			5,
			start,
			end,
			color,
			Quaternion(),
			Vector4( 5, radius, radius, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_cylinder_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u = transform.basis.get_column(1) * 0.5f;
		const float   s = transform.basis.get_scale().x;

		_add_shape(
			5,
			transform.origin - u,
			transform.origin + u,
			color,
			_extract_rotation( transform.basis ),
			Vector4( 5, s, s, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_tapered_cylinder( const Vector3& start, const Vector3& end, const float radius_start, const float radius_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			5,
			start,
			end,
			color,
			Quaternion(),
			Vector4( 5, radius_start, radius_end, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_tapered_cylinder_xform( const Transform3D &transform, const float radius_start, const float radius_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u  = transform.basis.get_column(1) * 0.5f;
		const float   sc = transform.basis.get_scale().x;

		_add_shape(
			5,
			transform.origin - u,
			transform.origin + u,
			color,
			_extract_rotation( transform.basis ),
			Vector4( 5, radius_start * sc, radius_end * sc, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_cylinder_arc( const Vector3& start, const Vector3& end, const float radius, const float angle_start, const float angle_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		_add_shape(
			7,
			start,
			Vector3( ( end - start ).length(), radius, 0 ),
			color,
			_rotation_from_normal( ( end - start ).normalized() ),
			Vector4( 7, static_cast<float>( static_cast<std::uint32_t>( flags ) ), angle_start, angle_end )
		);
	}

	void JWDebugDraw::draw_cylinder_arc_xform( const Transform3D &transform, const float angle_start, const float angle_end, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 s = transform.basis.get_scale();

		_add_shape(
			7,
			transform.origin - transform.basis.get_column(1) * 0.5f,
			Vector3( s.y, s.x, 0 ),
			color,
			_extract_rotation( transform.basis ),
			Vector4( 7, static_cast<float>( static_cast<std::uint32_t>( flags ) ), angle_start, angle_end )
		);
	}

	void JWDebugDraw::draw_cone( const Vector3& position, const float radius, const float height, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u = VEC3_UP * ( height / 2.0f );

		_add_shape(
			5,
			position - u,
			position + u,
			color,
			Quaternion(),
			Vector4( 5, radius, 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_cone_xform( const Transform3D &transform, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 u = transform.basis.get_column(1) * 0.5f;

		_add_shape(
			5,
			transform.origin - u,
			transform.origin + u,
			color,
			_extract_rotation( transform.basis ),
			Vector4( 5, transform.basis.get_scale().x, 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_arrow( const Vector3& start, const Vector3& end, const float head_radius, const float head_length, const Color& color, const BitField<DrawFlags> flags )
	{
		ERR_FAIL_COND( !_initialized );

		const Vector3 d = end - start;
		const float   l = d.length();

		if ( l < 1e-4f ) return;

		const Color   col = color;
		const Vector3 dir = d / l;
		const float   hl  = MIN( head_length < 0 ? head_radius * 2.0f : head_length, l );
		const float   sl  = l - hl;
		const Vector3 hb  = start + dir * sl;

		if ( sl > 0.001f ) _add_shape( 2, start, hb, col, Quaternion(), Vector4( 2, 0, 0, 0 ) );

		_add_shape(
			5,
			hb + dir * MIN( 0.002f, hl * 0.1f ),
			end,
			col,
			Quaternion(),
			Vector4( 5, head_radius, 0, static_cast<float>( static_cast<std::uint32_t>( flags ) ) )
		);
	}

	void JWDebugDraw::draw_axis( const Transform3D &transform, const float size )
	{
		ERR_FAIL_COND( !_initialized );

		draw_arrow(
			transform.origin,
			transform.origin + transform.basis.get_column(0) * size,
			0.06f * size,
			0.16f * size,
			COLOR_RED
		);

		draw_arrow(
			transform.origin,
			transform.origin + transform.basis.get_column(1) * size,
			0.06f * size,
			0.16f * size,
			COLOR_GREEN
		);

		draw_arrow(
			transform.origin,
			transform.origin + transform.basis.get_column(2) * size,
			0.06f * size,
			0.16f * size,
			COLOR_BLUE
		);
	}
}