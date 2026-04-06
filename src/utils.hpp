#pragma once

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include "debug_draw.hpp"

namespace JW
{
#ifdef DEBUG_ENABLED
	#define JWTRACE() print_line( vformat( "JWDebugDraw::%s called!", __func__ ) )
#else
	#define JWTRACE()
#endif

	#define DEFVALBIND DEFVAL( COLOR_DEFAULT ), DEFVAL( DRAW_FLAGS_NONE )

	inline float JWDebugDraw::_calculate_pixel_scale()
	{
		if ( ! _viewport ) return 1.0f;

		const float vp_height = MAX( 1.0f, _viewport->get_visible_rect().size.y );

		if ( _camera )
		{
			if ( _camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL )
			{
				return _camera->get_size() / vp_height;
			}

			return 2 * Math::tan( Math::deg_to_rad( _camera->get_fov() ) * 0.5f ) / vp_height;
		}

		if ( _camera_override )
		{
			if ( const float m11 = _override_proj.columns[1][1]; m11 > 0 ) return 2.0f / ( m11 * vp_height );
		}

		return 1.0f;
	}

	inline Quaternion JWDebugDraw::_rotation_from_normal( Vector3 normal )
	{
		if ( normal.length_squared() < 1e-6f ) return {};

		normal.normalize();

		if ( normal.is_equal_approx( VEC3_UP ) )   return {};
		if ( normal.is_equal_approx( VEC3_DOWN ) ) return { VEC3_RIGHT, Math_PI };

		return { VEC3_UP.cross( normal ).normalized(), Math::acos( VEC3_UP.dot( normal ) ) };
	}

	inline Quaternion JWDebugDraw::_extract_rotation( const Basis& b )
	{
		if ( constexpr float EPSILON = 0.002f;
			 Math::abs( b.get_column(0).length_squared() - 1.0f ) < EPSILON &&
			 Math::abs( b.get_column(1).length_squared() - 1.0f ) < EPSILON &&
			 Math::abs( b.get_column(2).length_squared() - 1.0f ) < EPSILON )
		{
			return b.get_rotation_quaternion();
		}

		return b.orthonormalized().get_rotation_quaternion();
	}

	inline Vector3 JWDebugDraw::_catmull_rom( const Vector3 p0, const Vector3 p1, const Vector3 p2, const Vector3 p3,
	                                          const float t, const float tension )
	{
		const float t2  = t * t;
		const float t3  = t2 * t;
		const float t2t = 3.0f * t2;
		const float t3t = 2.0f * t3;
		const auto  x   = Vector4( t3t - t2t + 1.0f, t3 - 2.0f * t2 + t, t3 - t2, -t3t + t2t );
		      auto  a   = []( const float i0, const float i1, const float i2, const float i3, const Vector4 v, const float s ) {
		                      return v.x * i1 + v.y * ( ( i2 - i0 ) * s ) + v.z * ( ( i3 - i1 ) * s ) + v.w * i2;
		                  };

		return {
			a( p0.x, p1.x, p2.x, p3.x, x, tension ),
			a( p0.y, p1.y, p2.y, p3.y, x, tension ),
			a( p0.z, p1.z, p2.z, p3.z, x, tension )
		};
	}

	inline bool JWDebugDraw::_unproject( const Vector3 position, const Transform3D& view, const Projection& proj,
	                                     const Vector2 vp_size, Vector2& screen_pos )
	{
		const Vector3 p = view.xform( position );

		if ( p.z > -0.05f ) return false;

		const auto c = Vector4(
			p.x * proj.columns[0].x + p.y * proj.columns[1].x + p.z * proj.columns[2].x + proj.columns[3].x,
			p.x * proj.columns[0].y + p.y * proj.columns[1].y + p.z * proj.columns[2].y + proj.columns[3].y,
			p.x * proj.columns[0].z + p.y * proj.columns[1].z + p.z * proj.columns[2].z + proj.columns[3].z,
			p.x * proj.columns[0].w + p.y * proj.columns[1].w + p.z * proj.columns[2].w + proj.columns[3].w
		);

		if ( Math::abs( c.w ) < 1e-5f ) return false;

		const float rw = 1.0f / c.w;
		screen_pos = Vector2( ( c.x * rw + 1.0f ) * 0.5f * vp_size.x, ( 1.0f - c.y * rw ) * 0.5f * vp_size.y );

		return true;
	}

	inline RID JWDebugDraw::_create_unit_cube_mesh( RenderingServer *rs )
	{
		RID mesh = rs->mesh_create();

		PackedVector3Array verts;
		verts.resize( 8 );
		Vector3 *vw = verts.ptrw();
		vw[0] = Vector3(-0.5f,-0.5f,-0.5f );
		vw[1] = Vector3( 0.5f,-0.5f,-0.5f );
		vw[2] = Vector3( 0.5f, 0.5f,-0.5f );
		vw[3] = Vector3(-0.5f, 0.5f,-0.5f );
		vw[4] = Vector3(-0.5f,-0.5f, 0.5f );
		vw[5] = Vector3( 0.5f,-0.5f, 0.5f );
		vw[6] = Vector3( 0.5f, 0.5f, 0.5f );
		vw[7] = Vector3(-0.5f, 0.5f, 0.5f );

		PackedInt32Array indices;
		indices.resize( 36 );
		int32_t *iw = indices.ptrw();
		const int idx_data[] = { 0,1,2, 0,2,3, 5,4,7, 5,7,6, 4,0,3, 4,3,7, 1,5,6, 1,6,2, 3,2,6, 3,6,7, 4,5,1, 4,1,0 };
		for ( int i = 0; i < 36; i++ ) iw[ i ] = idx_data[ i ];

		Array arrays;
		arrays.resize( RenderingServer::ARRAY_MAX );
		arrays[ RenderingServer::ARRAY_VERTEX ] = verts;
		arrays[ RenderingServer::ARRAY_INDEX ]  = indices;

		rs->mesh_add_surface_from_arrays( mesh, RenderingServer::PRIMITIVE_TRIANGLES, arrays );

		return mesh;
	}
}
