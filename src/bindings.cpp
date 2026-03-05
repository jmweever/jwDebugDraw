#include "utils.hpp"
#include "debug_draw.hpp"

using namespace godot;

namespace JW
{
    void JWDebugDraw::_bind_methods()
	{
		const StringName& _class = get_class_static();

		ClassDB::bind_integer_constant( _class, "TextFlags", "TEXT_NONE",            TEXT_FLAGS_NONE );
		ClassDB::bind_integer_constant( _class, "TextFlags", "TEXT_FIXED_SIZE",      TEXT_FLAGS_FIXED_SIZE );
		ClassDB::bind_integer_constant( _class, "TextFlags", "TEXT_RELATIVE",        TEXT_FLAGS_RELATIVE );
		ClassDB::bind_integer_constant( _class, "TextFlags", "TEXT_APPEND",          TEXT_FLAGS_APPEND );
		ClassDB::bind_integer_constant( _class, "DrawFlags", "DRAW_NONE",            DRAW_FLAGS_NONE );
		ClassDB::bind_integer_constant( _class, "DrawFlags", "DRAW_FILLED",          DRAW_FLAGS_FILLED );
		ClassDB::bind_integer_constant( _class, "DrawFlags", "DRAW_BACKGROUND",      DRAW_FLAGS_BACKGROUND );
		ClassDB::bind_integer_constant( _class, "DrawFlags", "DRAW_SILHOUETTE_ONLY", DRAW_FLAGS_SILHOUETTE_ONLY );
		ClassDB::bind_integer_constant( _class, "DrawFlags", "DRAW_SCREEN_SPACE",    DRAW_FLAGS_SCREEN_SPACE );

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_text", "text", "position", "text_color", "outline_color", "background_color", "flags" ),
			&JWDebugDraw::draw_text,
			DEFVAL( COLOR_WHITE ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( TEXT_FLAGS_NONE )
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_text_xform", "text", "transform", "text_color", "outline_color", "background_color", "flags" ),
			&JWDebugDraw::draw_text_xform,
			DEFVAL( COLOR_WHITE ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( TEXT_FLAGS_NONE )
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_text_2d", "text", "position", "text_color", "outline_color", "background_color", "flags" ),
			&JWDebugDraw::draw_text_2d,
			DEFVAL( COLOR_WHITE ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( COLOR_CLEAR ),
			DEFVAL( TEXT_FLAGS_NONE )
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_point", "position", "size", "color", "flags" ),
			&JWDebugDraw::draw_point,
			DEFVAL( 0.01f ),
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_line", "start", "end", "color", "flags" ),
			&JWDebugDraw::draw_line,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_ray", "origin", "direction", "color", "flags" ),
			&JWDebugDraw::draw_ray,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_triangle", "vertex_a", "vertex_b", "vertex_c", "color", "flags" ),
			&JWDebugDraw::draw_triangle,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_arc", "position", "axis", "radius", "angle_start", "angle_end", "color", "flags" ),
			&JWDebugDraw::draw_arc,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_curve", "points", "tension", "segments", "color", "flags" ),
			&JWDebugDraw::draw_curve,
			DEFVAL( 0.5f ),
			DEFVAL( 10 ),
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_circle", "position", "axis", "radius", "color", "flags" ),
			&JWDebugDraw::draw_circle,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_circle_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_circle_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_box", "position", "size", "color", "flags" ),
			&JWDebugDraw::draw_box,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_box_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_box_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_frustum", "position", "size_start", "size_end", "length", "color", "flags" ),
			&JWDebugDraw::draw_frustum,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_frustum_xform", "transform", "size_start", "size_end", "color", "flags" ),
			&JWDebugDraw::draw_frustum_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_pyramid", "position", "size", "height", "color", "flags" ),
			&JWDebugDraw::draw_pyramid,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_pyramid_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_pyramid_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_sphere_arc", "position", "radius", "angle_start", "angle_end", "color", "flags" ),
			&JWDebugDraw::draw_sphere_arc,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_sphere_arc_xform", "transform", "angle_start", "angle_end", "color", "flags" ),
			&JWDebugDraw::draw_sphere_arc_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_hemisphere", "position", "radius", "up", "color", "flags" ),
			&JWDebugDraw::draw_hemisphere,
			DEFVAL( VEC3_UP ),
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_sphere", "position", "radius", "color", "flags" ),
			&JWDebugDraw::draw_sphere,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_sphere_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_sphere_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_capsule", "start", "end", "radius", "color", "flags" ),
			&JWDebugDraw::draw_capsule,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_capsule_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_capsule_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_tapered_capsule", "start", "end", "radius_start", "radius_end", "color", "flags" ),
			&JWDebugDraw::draw_tapered_capsule,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_tapered_capsule_xform", "transform", "radius_start", "radius_end", "color", "flags" ),
			&JWDebugDraw::draw_tapered_capsule_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cylinder", "start", "end", "radius", "color", "flags" ),
			&JWDebugDraw::draw_cylinder,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cylinder_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_cylinder_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_tapered_cylinder", "start", "end", "radius_start", "radius_end", "color", "flags" ),
			&JWDebugDraw::draw_tapered_cylinder,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_tapered_cylinder_xform", "transform", "radius_start", "radius_end", "color", "flags" ),
			&JWDebugDraw::draw_tapered_cylinder_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cylinder_arc", "start", "end", "radius", "angle_start", "angle_end", "color", "flags" ),
			&JWDebugDraw::draw_cylinder_arc,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cylinder_arc_xform", "transform", "angle_start", "angle_end", "color", "flags" ),
			&JWDebugDraw::draw_cylinder_arc_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cone", "position", "radius", "height", "color", "flags" ),
			&JWDebugDraw::draw_cone,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_cone_xform", "transform", "color", "flags" ),
			&JWDebugDraw::draw_cone_xform,
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_arrow", "start", "end", "head_radius", "head_length", "color", "flags" ),
			&JWDebugDraw::draw_arrow,
			DEFVAL( 0.15f ),
			DEFVAL( -1.0f ),
			DEFVALBIND
		);

		ClassDB::bind_static_method(
			_class,
			D_METHOD( "draw_axis", "transform", "size" ),
			&JWDebugDraw::draw_axis,
			DEFVAL( 1.0f )
		);

		JWTRACE();
	}
}