#include <algorithm>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include "shader.hpp"
#include "utils.hpp"
#include "debug_draw.hpp"

using namespace godot;

namespace JW
{
	bool                        JWDebugDraw::_initialized       = false;
	SceneTree                  *JWDebugDraw::_scene_tree        = nullptr;
	Viewport                   *JWDebugDraw::_viewport          = nullptr;
	Camera3D                   *JWDebugDraw::_camera            = nullptr;
	Vector3                     JWDebugDraw::_camera_pos        = VEC3_ZERO;
	float                       JWDebugDraw::_pixel_scale       = 1.0;
	float                       JWDebugDraw::_line_thickness    = 0.5;
	uint64_t                    JWDebugDraw::_last_frame        = 0;
	Vector3                     JWDebugDraw::_last_cam_pos      = VEC3_ZERO;
	float                       JWDebugDraw::_last_pixel_scale  = 1.0;
	float                       JWDebugDraw::_last_thickness    = 0.5;
	bool                        JWDebugDraw::_shapes_dirty      = false;
	Vector<JWDebugDraw::Shape> *JWDebugDraw::_shapes_frame      = nullptr;
	Vector<JWDebugDraw::Shape> *JWDebugDraw::_shapes_phys       = nullptr;
	int                         JWDebugDraw::_shape_num         = 0;
	int                         JWDebugDraw::_last_shape_num    = -1;
	int                         JWDebugDraw::_shape_num_frame   = 0;
	int                         JWDebugDraw::_shape_num_phys    = 0;
	int                         JWDebugDraw::_visible_instances = 0;
	Vector<JWDebugDraw::Text>  *JWDebugDraw::_texts             = nullptr;
	Vector<JWDebugDraw::Text>  *JWDebugDraw::_texts_frame       = nullptr;
	Vector<JWDebugDraw::Text>  *JWDebugDraw::_texts_phys        = nullptr;
	int                         JWDebugDraw::_text_num          = 0;
	int                         JWDebugDraw::_text_num_frame    = 0;
	int                         JWDebugDraw::_text_num_phys     = 0;
	Vector<float>              *JWDebugDraw::_sort_keys         = nullptr;
	Vector<int>                *JWDebugDraw::_indices           = nullptr;
	uint64_t                    JWDebugDraw::_phys_shape_frame  = 0;
	uint64_t                    JWDebugDraw::_phys_text_frame   = 0;
	RID                        *JWDebugDraw::_mesh_rid          = nullptr;
	RID                        *JWDebugDraw::_multimesh_rid     = nullptr;
	RID                        *JWDebugDraw::_instance_rid      = nullptr;
	RID                        *JWDebugDraw::_shader_rid        = nullptr;
	RID                        *JWDebugDraw::_material_rid      = nullptr;
	RID                        *JWDebugDraw::_text_canvas_rid   = nullptr;
	Ref<Font>                  *JWDebugDraw::_debug_font        = nullptr;
	Ref<Image>                 *JWDebugDraw::_data_image        = nullptr;
	Ref<ImageTexture>          *JWDebugDraw::_data_texture      = nullptr;
	PackedByteArray            *JWDebugDraw::_texture_bytes     = nullptr;
	PackedFloat32Array         *JWDebugDraw::_transform_buffer  = nullptr;
	int                         JWDebugDraw::_allocated_count   = 0;
	bool                        JWDebugDraw::_test_enabled      = false;

	void JWDebugDraw::initialize()
	{
		ERR_FAIL_COND( _initialized );

		RenderingServer *rs = RenderingServer::get_singleton();
		_scene_tree = cast_to<SceneTree>( Engine::get_singleton()->get_main_loop() );

		ERR_FAIL_COND( ! _scene_tree );

		if ( Engine::get_singleton()->is_editor_hint() )
		{
			_viewport = EditorInterface::get_singleton()->get_editor_viewport_3d(0);
		}
		else
		{
			_viewport = cast_to<Viewport>( _scene_tree->get_root() );
		}

		ERR_FAIL_COND( ! _viewport );

		_mesh_rid         = memnew( RID );
		_shader_rid       = memnew( RID );
		_material_rid     = memnew( RID );
		_multimesh_rid    = memnew( RID );
		_instance_rid     = memnew( RID );
		_text_canvas_rid  = memnew( RID );
		_shapes_dirty     = false;
		_last_shape_num   = -1;
		_shapes_frame     = memnew( Vector<Shape> );
		_shapes_phys      = memnew( Vector<Shape> );
		_texts            = memnew( Vector<Text> );
		_texts_frame      = memnew( Vector<Text> );
		_texts_phys       = memnew( Vector<Text> );
		_sort_keys        = memnew( Vector<float> );
		_indices          = memnew( Vector<int> );
		_debug_font       = memnew( Ref<Font> );
		_data_image       = memnew( Ref<Image> );
		_data_texture     = memnew( Ref<ImageTexture> );
		_texture_bytes    = memnew( PackedByteArray );
		_transform_buffer = memnew( PackedFloat32Array );

		*_shader_rid = rs->shader_create();
		rs->shader_set_path_hint( *_shader_rid, "res://jwdebugdraw/debug_draw_shader.gdshader" );
		rs->shader_set_code( *_shader_rid, SHADER_CODE );

		*_material_rid = rs->material_create();
		rs->material_set_shader( *_material_rid, *_shader_rid );
		(void)rs->get_shader_parameter_list( *_shader_rid );

		*_multimesh_rid = rs->multimesh_create();
		rs->multimesh_allocate_data( *_multimesh_rid, INITIAL_ALLOCATION, RenderingServer::MULTIMESH_TRANSFORM_3D );
		rs->multimesh_set_custom_aabb( *_multimesh_rid, AABB( VEC3_ONE * -1e7f, VEC3_ONE * 2e7f ) );

		*_mesh_rid = _create_unit_cube_mesh( rs );
		rs->multimesh_set_mesh( *_multimesh_rid, *_mesh_rid );

		*_instance_rid = rs->instance_create();
		rs->instance_set_base( *_instance_rid, *_multimesh_rid );
		rs->instance_geometry_set_material_override( *_instance_rid, *_material_rid );
		rs->instance_set_extra_visibility_margin( *_instance_rid, 100 );
		rs->instance_geometry_set_cast_shadows_setting( *_instance_rid, RenderingServer::SHADOW_CASTING_SETTING_OFF );
		rs->instance_set_scenario( *_instance_rid, _viewport->find_world_3d()->get_scenario() );

		*_text_canvas_rid = rs->canvas_item_create();
		rs->canvas_item_set_parent( *_text_canvas_rid, _viewport->find_world_2d()->get_canvas() );
		rs->canvas_item_set_z_index( *_text_canvas_rid, 4096 );

		*_debug_font      = ThemeDB::get_singleton()->get_fallback_font();
		_camera           = _viewport->get_camera_3d();
		_camera_pos       = _camera ? _camera->get_global_position() : VEC3_ZERO;
		_last_cam_pos     = Vector3( 1e38f, 1e38f, 1e38f );
		_pixel_scale      = _calculate_pixel_scale();
		_last_pixel_scale = -1.0f;
		_line_thickness   = CONSTANT_THICKNESS_DEFAULT ? THICKNESS_DEFAULT * _pixel_scale : THICKNESS_DEFAULT;
		_last_thickness   = -1.0f;

		rs->material_set_param( *_material_rid, "thickness",      _line_thickness );
		rs->material_set_param( *_material_rid, "screen_space",   CONSTANT_THICKNESS_DEFAULT );
		rs->material_set_param( *_material_rid, "backface_alpha", BACKFACE_ALPHA_DEFAULT );
		rs->material_set_param( *_material_rid, "antialiasing",   ANTIALIASING_DEFAULT );
		rs->material_set_param( *_material_rid, "pixel_scale",    _pixel_scale );

#ifdef DEBUG_ENABLED
		if ( Engine::get_singleton()->is_editor_hint() )
		{
			OS::get_singleton()->set_low_processor_usage_mode( false );
		}

		if ( _test_enabled )
		{
			_scene_tree->connect( "process_frame", callable_mp_static( &JWDebugDraw::_test ) );
		}
#endif

		_initialized = true;

		JWTRACE();
	}

	void JWDebugDraw::shutdown()
	{
		ERR_FAIL_COND( !_initialized );

		RenderingServer *rs = RenderingServer::get_singleton();

		if ( _instance_rid->is_valid() ) rs->free_rid( *_instance_rid );
		if ( _multimesh_rid->is_valid() ) rs->free_rid( *_multimesh_rid );
		if ( _material_rid->is_valid() ) rs->free_rid( *_material_rid );
		if ( _shader_rid->is_valid() ) rs->free_rid( *_shader_rid );
		if ( _mesh_rid->is_valid() ) rs->free_rid( *_mesh_rid );
		if ( _text_canvas_rid->is_valid() ) rs->free_rid( *_text_canvas_rid );

		memdelete( _mesh_rid );
		memdelete( _shader_rid );
		memdelete( _material_rid );
		memdelete( _multimesh_rid );
		memdelete( _instance_rid );
		memdelete( _text_canvas_rid );
		memdelete( _shapes_frame );
		memdelete( _shapes_phys );
		memdelete( _texts );
		memdelete( _texts_frame );
		memdelete( _texts_phys );
		memdelete( _sort_keys );
		memdelete( _indices );
		memdelete( _debug_font );
		memdelete( _data_image );
		memdelete( _data_texture );
		memdelete( _texture_bytes );
		memdelete( _transform_buffer );

		_mesh_rid         = nullptr;
		_shader_rid       = nullptr;
		_material_rid     = nullptr;
		_multimesh_rid    = nullptr;
		_instance_rid     = nullptr;
		_text_canvas_rid  = nullptr;
		_shapes_frame     = nullptr;
		_shapes_phys      = nullptr;
		_texts            = nullptr;
		_texts_frame      = nullptr;
		_texts_phys       = nullptr;
		_sort_keys        = nullptr;
		_indices          = nullptr;
		_debug_font       = nullptr;
		_data_image       = nullptr;
		_data_texture     = nullptr;
		_texture_bytes    = nullptr;
		_transform_buffer = nullptr;

		_initialized = false;

		JWTRACE();
	}

	void JWDebugDraw::on_frame()
	{
		ERR_FAIL_COND( !_initialized );

		const Engine *engine = Engine::get_singleton();

		_maybe_advance_frame( engine );

		RenderingServer *rs = RenderingServer::get_singleton();

		_camera = _viewport->get_camera_3d();

		const uint64_t current_phys_frame = engine->get_physics_frames();

		if ( _phys_shape_frame != current_phys_frame ) _shape_num_phys = 0;
		if ( _phys_text_frame != current_phys_frame ) _text_num_phys = 0;

		_shape_num = _shape_num_frame + _shape_num_phys;
		_text_num  = _text_num_frame + _text_num_phys;

		if ( static_cast<int>( _texts->size() ) < _text_num ) _texts->resize( MAX( _text_num, static_cast<int>( _texts->size() ) * 2 ) );

		if ( _text_num_phys > 0 ) std::copy_n( _texts_phys->ptr(), _text_num_phys, _texts->ptrw() );
		if ( _text_num_frame > 0 ) std::copy_n( _texts_frame->ptr(), _text_num_frame, _texts->ptrw() + _text_num_phys );

		if ( _shape_num == 0 && _text_num == 0 )
		{
			if ( _visible_instances > 0 )
			{
				rs->multimesh_set_visible_instances( *_multimesh_rid, 0 );
				_visible_instances = 0;
			}

			rs->canvas_item_clear( *_text_canvas_rid );

			return;
		}

		if ( _camera )
		{
			_camera_pos     = _camera->get_global_position();
			_pixel_scale    = _calculate_pixel_scale();
			_line_thickness = CONSTANT_THICKNESS_DEFAULT ? THICKNESS_DEFAULT * _pixel_scale : THICKNESS_DEFAULT;

			const bool cam_changed    = !_camera_pos.is_equal_approx( _last_cam_pos );
			const bool scale_changed  = !Math::is_equal_approx( _pixel_scale, _last_pixel_scale );
			const bool shapes_changed = _shapes_dirty || _shape_num != _last_shape_num;

			if ( cam_changed )
			{
				rs->material_set_param( *_material_rid, "cam_pos", _camera_pos );
				_last_cam_pos = _camera_pos;
			}

			if ( scale_changed )
			{
				rs->material_set_param( *_material_rid, "pixel_scale", _pixel_scale );
				_last_pixel_scale = _pixel_scale;
			}

			if ( ! Math::is_equal_approx( _line_thickness, _last_thickness ) )
			{
				rs->material_set_param( *_material_rid, "thickness", _line_thickness );
				_last_thickness = _line_thickness;
			}

			if ( shapes_changed || cam_changed || scale_changed )
			{
				if ( _shape_num > 1 )
				{
					if ( static_cast<int>( _sort_keys->size() ) < _shape_num )
					{
						const int new_size = MAX( _shape_num, static_cast<int>( _sort_keys->size() ) * 2 );

						_sort_keys->resize( new_size );
						_indices->resize( new_size );
					}

					int   *indices   = _indices->ptrw();
					float *sort_keys = _sort_keys->ptrw();

					for ( int i = 0; i < _shape_num; i++ )
					{
						indices[ i ] = i;

						const Shape &s = _get_shape( i );

						Vector3 center;
						switch ( s.get_type() )
						{
							case 7:  center = s.param_a + s.rotation.xform( VEC3_UP ) * ( s.param_b.x * 0.5f ); break;
							case 6:  center = ( s.param_a + s.param_b + Vector3( s.rotation.x, s.rotation.y, s.rotation.z ) ) / 3.0f; break;
							case 2:
							case 4:  center = ( s.param_a + s.param_b ) * 0.5f; break;
							default: center = s.param_a; break;
						}

						sort_keys[ i ] = -center.distance_squared_to( _camera_pos );
					}

					std::sort(
						_indices->ptrw(),
						_indices->ptrw() + _shape_num,
						[ sort_keys ]( const int a, const int b ) { return sort_keys[ a ] < sort_keys[ b ]; }
					);
				}
				else if ( _shape_num == 1 )
				{
					if ( static_cast<int>( _indices->size() ) < 1 ) _indices->resize(1);
					_indices->ptrw()[0] = 0;
				}

				_update_texture();
				_update_transforms();

				_shapes_dirty   = false;
				_last_shape_num = _shape_num;
			}
			else if ( _visible_instances > 0 )
			{
				rs->multimesh_set_visible_instances( *_multimesh_rid, 0 );
				_visible_instances = 0;
			}
		}

		_update_text();
	}

#ifdef DEBUG_ENABLED
	void JWDebugDraw::_test()
	{
		struct TestMode
		{
			const char         *name = "";
			bool                rot  = false;
			BitField<DrawFlags> flags;
		};

		static const char *shape_names[] = {
			"Sphere", "Hemisphere", "Box", "Capsule", "Cylinder", "Cone", "Pyramid",
			"TapCapsule", "TapCyl", "Triangle", "Frustum", "Line", "Ray", "Arrow",
			"Curve", "Axis", "Arc", "Circle", "SphereArc", "CylArc"
		};
		static constexpr int SHAPE_COUNT = 20;

		static TestMode test_modes[] = {
			{ "Wire Static",   false, 0 },
			{ "Wire Rot",      true,  0 },
			{ "Filled Static", false, 1 },
			{ "Filled Rot",    true,  1 },
			{ "Sil Static",    false, 4 },
			{ "Sil Rot",       true,  4 },
			{ "BG Static",     false, 2 },
			{ "BG Rot",        true,  2 },
			{ "BG+Sil Static", false, 6 },
			{ "BG+Sil Rot",    true,  6 },
		};
		static constexpr int MODE_COUNT = 10;

		const float   time     = static_cast<float>( Time::get_singleton()->get_ticks_msec() ) / 1000.0f;
		const Vector3 position = VEC3_ZERO;

		for ( int i = 0; i < MODE_COUNT; i++ )
		{
			constexpr float spacing              = 3.5f;
			          auto  & [name, rot, flags] = test_modes[ i ];
			const     auto  iF                   = static_cast<float>( i );

			draw_text(
				name,
				Vector3( position.x - 3.5f, position.y, position.z + iF * spacing ),
				Color( 1, 1, 0, 1 ),
				COLOR_CLEAR,
				COLOR_CLEAR,
				TEXT_FLAGS_FIXED_SIZE
			);

			for ( int j = 0; j < SHAPE_COUNT; j++ )
			{
				const auto jF = static_cast<float>( j );

				if ( i == 0 )
				{
					draw_text(
						shape_names[ j ],
						Vector3( position.x + jF * spacing, position.y, position.z - 3.5f ),
						Color( 1, 1, 0, 1 ),
						COLOR_CLEAR,
						COLOR_CLEAR,
						TEXT_FLAGS_FIXED_SIZE
					);
				}

				const Color col      = Color::from_hsv( jF / SHAPE_COUNT, 0.6f, 1.0f );
				Vector3     rot_axis = j % 3 == 0 ? VEC3_UP : j % 3 == 1 ? VEC3_RIGHT : Vector3( 0, 0, 1 );
				Transform3D trans(
					rot
						? Basis::from_euler( rot_axis * time + Vector3( time * 0.5f, 0, time * 0.2f ) )
						: Basis(),
					Vector3( position.x + jF * spacing, position.y, position.z + iF * spacing )
				);

				switch ( j )
				{
					case 0: draw_sphere_xform( trans, col, flags ); break;

					case 1: draw_hemisphere( trans.origin, 0.5f, trans.basis.get_column(1), col, flags ); break;

					case 2: draw_box_xform( trans, col, flags ); break;

					case 3: draw_capsule_xform( trans, col, flags ); break;

					case 4: draw_cylinder_xform( trans, col, flags ); break;

					case 5: draw_cone_xform( trans, col, flags ); break;

					case 6: draw_pyramid_xform( trans, col, flags ); break;

					case 7: draw_tapered_capsule_xform( trans, 0.4f, 0.15f, col, flags ); break;

					case 8: draw_tapered_cylinder_xform( trans, 0.4f, 0.15f, col, flags ); break;

					case 9:
						draw_triangle(
							trans.xform( Vector3( 0,    0.5f, 0 ) ),
							trans.xform( Vector3(-0.4f,-0.3f, 0 ) ),
							trans.xform( Vector3( 0.4f,-0.3f, 0 ) ),
							col,
							flags
						);
						break;

					case 10: draw_frustum_xform( trans, Vector2( 0.2f, 0.2f ), Vector2( 0.6f, 0.6f ), col, flags ); break;

					case 11:
						draw_line(
							trans.origin - trans.basis.get_column( 0 ) * 0.4f,
							trans.origin + trans.basis.get_column( 0 ) * 0.4f,
							col,
							flags
						);
						break;

					case 12: draw_ray( trans.origin, trans.basis.get_column( 2 ).normalized() * 0.8f, col, flags ); break;

					case 13: draw_arrow( trans.origin, trans.origin + trans.basis.get_column(2) * 0.8f, 0.15f, -1.0f, col, flags ); break;

					case 14:
					{
						TypedArray<Vector3> pts;
						pts.push_back( trans.xform( Vector3(-0.5f, 0,   -0.5f ) ) );
						pts.push_back( trans.xform( Vector3(-0.2f, 0.5f,-0.2f ) ) );
						pts.push_back( trans.xform( Vector3( 0.2f,-0.5f, 0.2f ) ) );
						pts.push_back( trans.xform( Vector3( 0.5f, 0,    0.5f ) ) );
						draw_curve( pts, 0.5f, 10, col, flags );
						break;
					}

					case 15: draw_axis( trans, 0.6f ); break;

					case 16: draw_arc( trans.origin, trans.basis.get_column(2), 0.5f, 0, 270, col, flags ); break;

					case 17: draw_circle_xform( trans, col, flags ); break;

					case 18: draw_sphere_arc_xform( trans, 0, 270, col, flags ); break;

					case 19: draw_cylinder_arc_xform( trans, 0, 270, col, flags ); break;

					default: break;
				}
			}
		}
	}
#endif
}