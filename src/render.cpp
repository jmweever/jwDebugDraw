#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include "utils.hpp" // IWYU pragma: keep
#include "debug_draw.hpp"

namespace JW
{
	const JWDebugDraw::Shape& JWDebugDraw::_get_shape( const int i )
	{
		return i < _shape_num_phys ? _shapes_phys->ptr()[ i ] : _shapes_frame->ptr()[ i - _shape_num_phys ];
	}

	void JWDebugDraw::_maybe_advance_frame( const Engine *engine )
	{
		if ( const uint64_t current_frame = engine->get_process_frames(); current_frame != _last_frame )
		{
			_last_frame      = current_frame;
			_shape_num_frame = 0;
			_text_num_frame  = 0;
		}
	}

	void JWDebugDraw::_update_texture()
	{
		if ( _shape_num == 0 ) return;

		RenderingServer *rs = RenderingServer::get_singleton();

		static int s_tex_h = 0;

		const int total_pixels = _shape_num * PIXELS_PER_SHAPE;
		const int tex_h        = ( total_pixels + TEX_WIDTH - 1 ) / TEX_WIDTH;
		const int req_bytes    = TEX_WIDTH * tex_h * 16;

		if ( tex_h != s_tex_h )
		{
			s_tex_h = tex_h;
			_texture_bytes->resize( req_bytes );
		}

		auto      *dest    = reinterpret_cast<Shape *>( _texture_bytes->ptrw() );
		const int *indices = _indices->ptr();

		for ( int i = 0; i < _shape_num; i++ )
		{
			dest[ i ] = _get_shape( indices[ i ] );
		}

		if ( _data_image->is_null() || (*_data_image)->get_height() != tex_h )
		{
			*_data_image   = Image::create_from_data( TEX_WIDTH, tex_h, false, Image::FORMAT_RGBAF, *_texture_bytes );
			*_data_texture = ImageTexture::create_from_image( *_data_image );
			rs->material_set_param( *_material_rid, "data_tex", (*_data_texture)->get_rid() );
		}
		else
		{
			(*_data_image)->set_data( TEX_WIDTH, tex_h, false, Image::FORMAT_RGBAF, *_texture_bytes );
			(*_data_texture)->update( *_data_image );
		}
	}

	void JWDebugDraw::_update_transforms()
	{
		if ( _shape_num == 0 ) return;

		RenderingServer  *rs  = RenderingServer::get_singleton();
		WorkerThreadPool *wtp = WorkerThreadPool::get_singleton();

		if ( _allocated_count < _shape_num )
		{
			_allocated_count = MAX( 4096, static_cast<int>( next_power_of_2( _shape_num ) ) );

			_transform_buffer->resize( _allocated_count * 12 );

			rs->multimesh_allocate_data( *_multimesh_rid, _allocated_count, RenderingServer::MULTIMESH_TRANSFORM_3D );
			rs->multimesh_set_custom_aabb( *_multimesh_rid, AABB( VEC3_ONE * -1e7f, VEC3_ONE * 2e7f ) );
		}

		float *buf = _transform_buffer->ptrw();

		if ( _shape_num > 2048 )
		{
			const WorkerThreadPool::GroupID group = wtp->add_native_group_task(
				[]( void *buff, const uint32_t i ) {
					_process_transform( static_cast<int>( i ), _indices->ptr()[ i ], _camera_pos, _pixel_scale * 1.5f, static_cast<float*>( buff ) );
				},
				buf,
				_shape_num
			);
			wtp->wait_for_group_task_completion( group );
		}
		else
		{
			for ( int i = 0; i < _shape_num; i++ )
			{
				_process_transform( i, _indices->ptr()[ i ], _camera_pos, _pixel_scale * 1.5f, buf );
			}
		}

		rs->multimesh_set_buffer( *_multimesh_rid, *_transform_buffer );

		if ( _visible_instances != _shape_num )
		{
			rs->multimesh_set_visible_instances( *_multimesh_rid, _shape_num );
			_visible_instances = _shape_num;
		}
	}

	void JWDebugDraw::_process_transform( const int buffer_index, const int shape_index, const Vector3& camera_position,
	                                      const float safe_px_scale, float* buf )
	{
		const Shape &s   = _get_shape( shape_index );
		      int   type = s.get_type();
		      float dist;

		switch ( type )
		{
			case 0:
				dist = camera_position.distance_to( s.param_a ) + s.param_b.x;
				break;

			case 1:
				dist = camera_position.distance_to( s.param_a )
				       + MAX(
				            s.param_b.length(),
				            Vector3( s.params.y * 2.0f, s.params.z * 2.0f, s.param_b.z ).length()
				        );
				break;

			case 3:
				dist = camera_position.distance_to( s.param_a );
				break;

			case 6:
				dist = camera_position.distance_to( ( s.param_a + s.param_b + Vector3( s.rotation.x, s.rotation.y, s.rotation.z ) ) / 3.0f );
				break;

			case 7:
				dist = camera_position.distance_to( s.param_a ) + s.param_b.x;
				break;

			default:
				dist = Math::sqrt( MAX(
					camera_position.distance_squared_to( s.param_a ),
					camera_position.distance_squared_to( s.param_b )
				) );
				break;
		}

		float world_px = dist * safe_px_scale;
		float pad      = ( CONSTANT_THICKNESS_DEFAULT ? THICKNESS_DEFAULT * world_px : THICKNESS_DEFAULT ) + 3.0f * world_px + 0.01f;

		Transform3D t;

		switch ( type )
		{
			case 6:
			{
				Vector3 a   = s.param_a;
				Vector3 b   = s.param_b;
				auto    c   = Vector3( s.rotation.x, s.rotation.y, s.rotation.z );
				auto    min = Vector3(
					MIN( a.x, MIN( b.x, c.x ) ),
					MIN( a.y, MIN( b.y, c.y ) ),
					MIN( a.z, MIN( b.z, c.z ) )
				);
				auto    max = Vector3(
					MAX( a.x, MAX( b.x, c.x ) ),
					MAX( a.y, MAX( b.y, c.y ) ),
					MAX( a.z, MAX( b.z, c.z ) )
				);

				t.origin = ( min + max ) * 0.5f;
				t.basis  = Basis().scaled( max - min + VEC3_ONE * pad );

				break;
			}

			case 7:
			{
				float len    = s.param_b.x;
				float radius = s.param_b.y;
				auto  b      = Basis( s.rotation );

				t.origin = s.param_a + b.get_column( 1 ) * ( len * 0.5f );
				t.basis  = Basis(
					b.get_column(0).normalized() * ( radius * 2.0f + pad ),
					b.get_column(1) * ( len + pad ),
					b.get_column(2).normalized() * ( radius * 2.0f + pad )
				);

				break;
			}

			case 2:
			case 4:
			case 5:
			{
				Vector3 dir = s.param_b - s.param_a;
				float   len = dir.length();

				t.origin = ( s.param_a + s.param_b ) * 0.5f;

				if ( len > 1e-5f )
				{
					Vector3 y     = dir / len;
					Vector3 x     = y.cross( Math::abs( y.y ) > 0.99f ? VEC3_RIGHT : VEC3_UP ).normalized();
					float   geo_d = type == 2 ? 0.0f : MAX( s.params.y, s.params.z ) * 2.0f;
					float   thick = geo_d + pad;

					t.basis = Basis(
						x * thick,
						y * ( len + ( type == 4 ? geo_d : 0.0f ) + pad + ( type == 2 ? dist * 0.001f : 0.0f ) ),
						x.cross( y ) * thick
					);
				}
				else
				{
					float sz = ( type == 2 ? 0.0f : MAX( s.params.y, s.params.z ) * 2.0f ) + pad;
					t.basis  = Basis().scaled( VEC3_ONE * sz );
				}

				break;
			}

			default:
			{
				t.origin = s.param_a;

				Vector3 sz;

				if ( type == 1 )
				{
					sz = Vector3( MAX( s.param_b.x, s.params.y * 2.0f ), MAX( s.param_b.y, s.params.z * 2.0f ), s.param_b.z )
					   + Vector3( pad, pad, pad );
				}
				else
				{
					float d = type == 0 || type == 3 ? s.param_b.x * 2.0f : 1.0f;
					if ( type == 0 && ( static_cast<int>( s.params.y ) & 8 ) != 0 ) d *= dist;
					sz = VEC3_ONE * ( d + pad );
				}

				if ( s.rotation.is_equal_approx( Quaternion() ) )
				{
					t.basis = Basis( sz.x, 0, 0, 0, sz.y, 0, 0, 0, sz.z );
				}
				else
				{
					Quaternion q = type == 1 || type == 3
					                   ? (s.rotation.length_squared() < 1e-6f ? Quaternion() : s.rotation.normalized())
					                   : Quaternion();
					auto       b = Basis( q );

					t.basis = Basis( b.get_column(0) * sz.x, b.get_column(1) * sz.y, b.get_column(2) * sz.z );
				}

				break;
			}
		}

		int idx = buffer_index * 12;

		buf[ idx ]      = t.basis.get_column(0).x;
		buf[ idx + 1 ]  = t.basis.get_column(1).x;
		buf[ idx + 2 ]  = t.basis.get_column(2).x;
		buf[ idx + 3 ]  = t.origin.x;
		buf[ idx + 4 ]  = t.basis.get_column(0).y;
		buf[ idx + 5 ]  = t.basis.get_column(1).y;
		buf[ idx + 6 ]  = t.basis.get_column(2).y;
		buf[ idx + 7 ]  = t.origin.y;
		buf[ idx + 8 ]  = t.basis.get_column(0).z;
		buf[ idx + 9 ]  = t.basis.get_column(1).z;
		buf[ idx + 10 ] = t.basis.get_column(2).z;
		buf[ idx + 11 ] = t.origin.z;
	}

	void JWDebugDraw::_add_shape( const int type, const Vector3& param_a, const Vector3& param_b, const Color& color,
	                              const Quaternion& rotation, const Vector4& params )
	{
		if ( !_initialized ) return;

		_shapes_dirty = true;

		const Engine *engine = Engine::get_singleton();

		_maybe_advance_frame( engine );

		const auto shape = Shape(
			param_a,
			param_b,
			color,
			rotation,
			Vector4( static_cast<float>( type ), params.y, params.z, params.w )
		);

		if ( engine->is_in_physics_frame() )
		{
			if ( const uint64_t current_frame = engine->get_physics_frames(); _phys_shape_frame != current_frame )
			{
				_phys_shape_frame = current_frame;
				_shape_num_phys   = 0;
			}

			if ( _shape_num_phys >= static_cast<int>( _shapes_phys->size() ) )
			{
				_shapes_phys->resize( MAX( 16, static_cast<int>( _shapes_phys->size() ) * 2 ) );
			}

			_shapes_phys->ptrw()[ _shape_num_phys++ ] = shape;
		}
		else
		{
			if ( _shape_num_frame >= static_cast<int>( _shapes_frame->size() ) )
			{
				_shapes_frame->resize( MAX( 16, static_cast<int>( _shapes_frame->size() ) * 2 ) );
			}

			_shapes_frame->ptrw()[ _shape_num_frame++ ] = shape;
		}
	}

	void JWDebugDraw::_add_shape( const int type, const Transform3D &transform, const Color& color, const Vector4& params )
	{
		_add_shape(
			type,
			transform.origin,
			transform.basis.get_scale(),
			color,
			_extract_rotation( transform.basis ),
			params
		);
	}

	void JWDebugDraw::_add_text( const Text &t )
	{
		const Engine *engine = Engine::get_singleton();

		_maybe_advance_frame( engine );

		if ( engine->is_in_physics_frame() )
		{
			if ( const uint64_t current_frame = engine->get_physics_frames(); _phys_text_frame != current_frame )
			{
				_phys_text_frame = current_frame;
				_text_num_phys = 0;
			}

			if ( _text_num_phys >= static_cast<int>( _texts_phys->size() ) )
			{
				_texts_phys->resize( MAX( 16, static_cast<int>( _texts_phys->size() ) * 2 ) );
			}

			_texts_phys->ptrw()[ _text_num_phys++ ] = t;
		}
		else
		{
			if ( _text_num_frame >= static_cast<int>( _texts_frame->size() ) )
			{
				_texts_frame->resize( MAX( 16, static_cast<int>( _texts_frame->size() ) * 2 ) );
			}

			_texts_frame->ptrw()[ _text_num_frame++ ] = t;
		}
	}

	void JWDebugDraw::_update_text()
	{
		RenderingServer *rs = RenderingServer::get_singleton();

		rs->canvas_item_clear( *_text_canvas_rid );

		if ( _debug_font->is_null() ) return;

		Transform3D cam_xform;
		Projection  proj;
		Transform3D view;
		bool        has_camera = _camera != nullptr;

		if ( has_camera )
		{
			cam_xform = _camera->get_global_transform();
			proj      = _camera->get_camera_projection();
			view      = cam_xform.inverse();
		}
		else if ( _camera_override )
		{
			view      = _override_view;
			cam_xform = _override_view.affine_inverse();
			proj      = _override_proj;
			has_camera = true;
		}

		Vector2     vp_size   = _viewport->get_visible_rect().size;
		Rect2       safe_area = Rect2( Vector2(), vp_size ).grow( vp_size.length() );
		const Text  *texts    = _texts->ptr();

		for ( int i = 0; i < _text_num; i++ )
		{
			const auto &[ is_2d, pos_2d, fixed_size, content, color, outline,
			              background, has_outline, has_background, is_oriented, orientation ] = texts[ i ];
			Vector2     screen_pos;
			int         targ_font_size = FONT_SIZE;
			Transform2D xform;

			if ( is_2d )
			{
				screen_pos = pos_2d;
				xform      = Transform2D( 0.0f, screen_pos );
			}
			else
			{
				if ( !has_camera
				     || !_unproject( orientation.origin, view, proj, vp_size, screen_pos )
				     || !safe_area.has_point( screen_pos ) ) continue;

				if ( is_oriented )
				{
					Vector3 o = orientation.origin;
					Basis   b = orientation.basis;
					Vector2 p_r, p_d;

					if ( !_unproject( o + b.get_column(0) * EPS, view, proj, vp_size, p_r ) ||
					     !_unproject( o + -b.get_column(1) * EPS, view, proj, vp_size, p_d ) )
					{
						continue;
					}

					Vector2 v_d   = ( p_d - screen_pos ) / EPS;
					float   scale = v_d.length() * PIXEL_SCALE;

					if ( scale > 100.0f ) continue;
					if ( scale > 1.0f ) targ_font_size = CLAMP( static_cast<int>( Math::round( static_cast<float>( FONT_SIZE ) * scale ) ), 8, 128 );

					float sf = static_cast<float>( FONT_SIZE ) / static_cast<float>( targ_font_size );
					xform = Transform2D(
						( p_r - screen_pos ) / EPS * PIXEL_SCALE * sf,
						v_d * PIXEL_SCALE * sf,
						screen_pos
					);
				}
				else
				{
					if ( !fixed_size )
					{
						if ( Vector2 top; _unproject( orientation.origin + cam_xform.basis.get_column(1) * 0.25f, view, proj, vp_size, top ) )
						{
							targ_font_size = CLAMP( static_cast<int>( Math::round( screen_pos.distance_to( top ) ) ), 8, 128 );
						}
					}

					xform = Transform2D( 0, screen_pos );
				}
			}

			Vector2 size     = (*_debug_font)->get_multiline_string_size( content, HORIZONTAL_ALIGNMENT_LEFT, -1, targ_font_size );
			Vector2 base_pos = -size * 0.5f + Vector2( 0, (*_debug_font)->get_ascent( targ_font_size ) );

			rs->canvas_item_add_set_transform( *_text_canvas_rid, xform );

			if ( has_background )
			{
				rs->canvas_item_add_rect(
					*_text_canvas_rid,
					Rect2( -size * 0.5f - VEC2_ONE * 2, size + VEC2_ONE * 4 ),
					background
				);
			}

			if ( has_outline )
			{
				(*_debug_font)->draw_multiline_string_outline(
					*_text_canvas_rid,
					base_pos,
					content,
					HORIZONTAL_ALIGNMENT_LEFT,
					-1,
					targ_font_size,
					-1,
					MAX( 1, targ_font_size / 12 ),
					outline
				);
			}

			(*_debug_font)->draw_multiline_string(
				*_text_canvas_rid,
				base_pos,
				content,
				HORIZONTAL_ALIGNMENT_LEFT,
				-1,
				targ_font_size,
				-1,
				color
			);
		}
	}
}
