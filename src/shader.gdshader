shader_type spatial;
render_mode unshaded, cull_front, depth_test_disabled;

const int  PIXELS_PER_SHAPE = 5;
const int  TEX_WIDTH        = 1024;
const vec3 ZERO3            = vec3(0);
const vec3 BOX_NORMALS[6]   = { vec3(-1,0,0), vec3(1,0,0), vec3(0,-1,0), vec3(0,1,0), vec3(0,0,-1), vec3(0,0,1) };
const int  BOX_EDGES[]      = { 0,1,2,4,1,2,1,4,2,3,3,4,3,0,0,4,4,5,2,5,5,6,1,5,6,7,3,5,7,4,0,5,0,4,0,2,1,5,1,2,2,6,1,3,3,7,0,3 };

struct Hit {
	float dist;
	vec4  color;
};

struct Context {
	vec3 ray_origin;
	vec3 ray_dir;
	Hit  solid;
	Hit  line1;
	Hit  line2;
};

struct ShapeFlags {
	bool filled;
	bool has_bg;
	bool is_sil;
	bool screen_scale;
};

struct AngleWedge {
	vec3 normal1;
	vec3 normal2;
	vec3 radial1;
	vec3 radial2;
};

uniform sampler2D data_tex : filter_nearest;
uniform vec3      cam_pos        = vec3(0);
uniform float     thickness      = 0.5;
uniform float     backface_alpha = 0.2;
uniform float     pixel_scale    = 1.0;
uniform bool      screen_space   = true;
uniform bool      antialiasing   = true;
uniform bool      show_bounds    = false;

varying flat int   instance_idx;
varying      vec3  pixel_world_pos;
varying flat vec4  v_data0;
varying flat vec4  v_data1;
varying flat vec4  v_data2;
varying flat vec4  v_data3;
varying flat vec4  v_data4;
varying flat mat3  v_rot_matrix;
varying flat vec3  v_cull_center;
varying flat float v_cull_radius;
varying flat vec3  v_aabb_extents;
varying flat float v_cull_height;

mat3 quat_to_mat3( vec4 q )
{
	vec3 q2  = q.xyz + q.xyz;
	vec3 qq  = q.xyz * q2;
	vec3 qw  = q2 * q.w;
	vec3 qxy = q.xxy * q2.yzz;

	return mat3(
		vec3( 1.0 - qq.y - qq.z, qxy.x + qw.z, qxy.y - qw.y ),
		vec3( qxy.x - qw.z, 1.0 - qq.x - qq.z, qxy.z + qw.x ),
		vec3( qxy.y + qw.y, qxy.z - qw.x, 1.0 - qq.x - qq.y )
	);
}

vec4 fetch_shape_data( int index, int offset )
{
	int linear = index * PIXELS_PER_SHAPE + offset;
	return texelFetch( data_tex, ivec2( linear % TEX_WIDTH, linear / TEX_WIDTH ), 0 );
}

float safe_ray_dist( float t )
{
	return max( t, 1e-4 );
}

ShapeFlags parse_flags( int flags )
{
	return ShapeFlags( ( flags & 1 ) != 0, ( flags & 2 ) != 0, ( flags & 4 ) != 0, ( flags & 8 ) != 0 );
}

bool is_partial_angle( vec2 angles )
{
	float diff = angles.y - angles.x;
	return diff * diff < 129600.0; // 360.0 * 360.0
}

Context make_local_context( Context ctx, vec3 local_origin, vec3 local_dir )
{
	Context local_ctx = ctx;
	local_ctx.ray_origin = local_origin;
	local_ctx.ray_dir    = local_dir;
	return local_ctx;
}

float calculate_alpha_modifier( vec3 normal, vec3 ray_dir )
{
	return mix( 1, backface_alpha, step( 0.001, dot( normal, ray_dir ) ) );
}

AngleWedge make_angle_wedge( vec2 angles, vec4 quat )
{
	float a1  = radians( angles.x );
	float a2  = radians( angles.y );
	vec2  sc1 = vec2( sin( a1 ), cos( a1 ) );
	vec2  sc2 = vec2( sin( a2 ), cos( a2 ) );

	return AngleWedge(
		v_rot_matrix * vec3(-sc1.x, sc1.y, 0 ),
		v_rot_matrix * vec3(-sc2.x, sc2.y, 0 ),
		v_rot_matrix * vec3( sc1.y, sc1.x, 0 ),
		v_rot_matrix * vec3( sc2.y, sc2.x, 0 )
	);
}

vec3 get_box_face_normal( int face_idx, vec3 norm_l, vec3 norm_r, vec3 norm_b, vec3 norm_t )
{
	if ( face_idx < 2 ) return face_idx == 0 ? norm_l : norm_r;
	if ( face_idx < 4 ) return face_idx == 2 ? norm_b : norm_t;
	return BOX_NORMALS[ face_idx ];
}

void get_ortho_basis( vec3 dir, vec4 quat, out vec3 right, out vec3 forward )
{
	float diff = quat.w - 1.0;

	if ( dot( quat.xyz, quat.xyz ) < 1e-6 && diff * diff < 1e-6 )
	{
		vec3 up = abs( dir.y ) > 0.99 ? BOX_NORMALS[1] : BOX_NORMALS[3];
		right   = normalize( cross( dir, up ) );
		forward = cross( right, dir );
	}
	else
	{
		right   = v_rot_matrix * BOX_NORMALS[1];
		forward = v_rot_matrix * BOX_NORMALS[5];
	}
}

void process_hit( float ray_dist, float sdf_dist, float alpha_mult, vec4 base_color, int shape_type, inout Context ctx )
{
	if ( ray_dist >= ctx.line2.dist ) return;

	float alpha = base_color.a * alpha_mult;
	if ( alpha < 0.001 ) return;

	float safe_dist   = max( ray_dist, 1e-4 );
	float line_width  = thickness * ( shape_type == 2 ? 2.0 : 1.0 ) * mix( 1, safe_dist, float( screen_space ) );
	float feather     = safe_dist * pixel_scale * 1.5;
	float threshold   = line_width + feather * 0.5 * float( antialiasing );
	bool is_line      = sdf_dist > 0.0;
	bool outside_line = is_line && sdf_dist >= threshold;

	if ( outside_line ) return;

	if ( is_line && antialiasing )
	{
		alpha *= 1.0 - smoothstep( line_width - feather * 0.5, line_width + feather * 0.5, sdf_dist );
	}

	vec4 color = vec4( base_color.rgb, alpha );

	if ( ! is_line )
	{
		if ( ray_dist < ctx.solid.dist )
		{
			ctx.solid = Hit( ray_dist, color );
		}
	}
	else if ( ray_dist < ctx.line1.dist )
	{
		ctx.line2 = ctx.line1;
		ctx.line1 = Hit( ray_dist, color );
	}
	else
	{
		ctx.line2 = Hit( ray_dist, color );
	}
}

float get_angle_visibility( vec2 p, vec2 angles, float feather )
{
	float diff = angles.y - angles.x; diff += 360.0 * float( diff < 0.0 );

	if ( diff >= 360.0 ) return 1.0;

	float radius = length( p );
	if ( radius < 1e-4 ) return 1.0;

	float rel_deg   = degrees( atan( p.y, p.x ) ) - angles.x; rel_deg -= floor( rel_deg / 360.0 ) * 360.0;
	float edge_dist = rel_deg >= 0.0 && rel_deg <= diff ? min( rel_deg, diff - rel_deg ) : -min( rel_deg - diff, 360.0 - rel_deg );

	return clamp( 0.5 + radians( edge_dist ) * radius / feather, 0.0, 1.0 );
}

bool solve_quadratic( float a, float b, float c, out float t1, out float t2 )
{
	float inv_a   = 1.0 / ( a + sign( a ) * 1e-6 );
	float b_half  = b * 0.5 * inv_a;
	float discrim = b_half * b_half - ( c * inv_a );

	if ( discrim < 0.0 )
	{
		t1 = t2 = -1.0;
		return false;
	}

	float sqrt_d = sqrt( discrim );

	t1 = -b_half - sqrt_d;
	t2 = -b_half + sqrt_d;

	return true;
}

bool intersect_segment( vec3 seg_start, vec3 seg_end, inout float out_ray_dist, inout float out_sdf_dist, Context ctx )
{
	vec3  seg         = seg_end - seg_start;
	vec3  to_origin   = ctx.ray_origin - seg_start;
	float dot_seg_ray = dot( seg, ctx.ray_dir );
	float dot_ray     = dot( ctx.ray_dir, to_origin );
	float denom       = dot( seg, seg ) - dot_seg_ray * dot_seg_ray;
	float seg_t       = denom < 1e-6 ? 0.0 : clamp( ( dot( seg, to_origin ) - dot_seg_ray * dot_ray ) / denom, 0, 1 );
	float ray_t       = dot_seg_ray * seg_t - dot_ray;

	if ( ray_t <= 0.0 ) return false;

	vec3  closest_diff = ( seg_start + seg * seg_t ) - ( ctx.ray_origin + ctx.ray_dir * ray_t );
	float dist_sq      = dot( closest_diff, closest_diff );

	if ( dist_sq >= out_sdf_dist * out_sdf_dist ) return false;

	out_sdf_dist = sqrt( dist_sq );
	out_ray_dist = ray_t;

	return true;
}

bool intersect_plane( vec3 plane_point, vec3 plane_normal, out float t, Context ctx )
{
	float denom = dot( ctx.ray_dir, plane_normal );
	if ( denom * denom < 1e-10 ) return false;

	t = dot( plane_point - ctx.ray_origin, plane_normal ) / denom;
	return t > 0.0;
}

bool intersect_aabb( vec3 ro, vec3 inv_rd, vec3 extents, out float t_min, out float t_max )
{
	vec3 t1     = (-extents - ro ) * inv_rd;
	vec3 t2     = ( extents - ro ) * inv_rd;
	vec3 tmin_v = min( t1, t2 );
	vec3 tmax_v = max( t1, t2 );

	t_min = max( max( tmin_v.x, tmin_v.y ), tmin_v.z );
	t_max = min( min( tmax_v.x, tmax_v.y ), tmax_v.z );

	return t_max >= max( t_min, 0.0 );
}

void intersect_disk(
	vec3 center,
	vec3 normal,
	float radius,
	vec4 color,
	float alpha,
	vec4 quat,
	vec3 origin_ref,
	vec2 angles,
	bool use_xz,
	inout Context ctx
)
{
	float t;
	if ( ! intersect_plane( center, normal, t, ctx ) ) return;

	vec3 hit  = ctx.ray_origin + ctx.ray_dir * t;
	vec3 diff = hit - center;

	if ( dot( diff, diff ) > radius * radius ) return;

	vec3 local = ( hit - origin_ref ) * v_rot_matrix;

	if ( get_angle_visibility( use_xz ? vec2( local.z, local.x ) : local.xy, angles, safe_ray_dist( t ) * pixel_scale ) > 0.0 )
	{
		process_hit( t, 0, alpha * calculate_alpha_modifier( normal, ctx.ray_dir ), color, 1, ctx );
	}
}

void intersect_disk_simple( vec3 center, vec3 normal, float radius, vec4 color, float alpha, inout Context ctx )
{
	float t;
	if ( ! intersect_plane( center, normal, t, ctx ) ) return;

	vec3 diff = ctx.ray_origin + ctx.ray_dir * t - center;

	if ( dot( diff, diff ) <= radius * radius )
	{
		process_hit( t, 0, alpha * calculate_alpha_modifier( normal, ctx.ray_dir ), color, 1, ctx );
	}
}

void intersect_halfplane( vec3 center, vec3 normal, vec3 radial, float radius, vec4 color, float alpha, inout Context ctx )
{
	float t;
	if ( ! intersect_plane( center, normal, t, ctx ) ) return;

	vec3 diff = ctx.ray_origin + ctx.ray_dir * t - center;

	if ( dot( diff, diff ) <= radius * radius && dot( diff, radial ) >= 0.0 )
	{
		process_hit( t, 0, alpha * calculate_alpha_modifier( normal, ctx.ray_dir ), color, 1, ctx );
	}
}

void intersect_rect_solid( vec3 center, vec3 axis, vec3 radial, vec2 dims, vec4 color, float alpha_mult, inout Context ctx )
{
	vec3  plane_normal = normalize( cross( axis, radial ) );
	float t;

	if ( intersect_plane( center, plane_normal, t, ctx ) )
	{
		vec3  hit_offset = ctx.ray_origin + ctx.ray_dir * t - center;
		float proj_u     = dot( hit_offset, axis );
		float proj_v     = dot( hit_offset, radial );

		if ( proj_u >= 0.0 && proj_u <= dims.x && proj_v >= 0.0 && proj_v <= dims.y )
		{
			process_hit( t, 0, alpha_mult * calculate_alpha_modifier( plane_normal, ctx.ray_dir ), color, 1, ctx );
		}
	}
}

bool intersect_infinite_cylinder( vec3 cyl_center, vec3 cyl_axis, float radius, out float t1, out float t2, Context ctx )
{
	vec3  to_center   = ctx.ray_origin - cyl_center;
	vec3  origin_perp = to_center - cyl_axis * dot( to_center, cyl_axis );
	vec3  dir_perp    = ctx.ray_dir - cyl_axis * dot( ctx.ray_dir, cyl_axis );
	float a           = dot( dir_perp, dir_perp );

	if ( a < 1e-6 )
	{
		t1 = t2 = -1.0;
		return false;
	}

	return solve_quadratic( a, 2.0 * dot( origin_perp, dir_perp ), dot( origin_perp, origin_perp ) - radius * radius, t1, t2 );
}

float intersect_sphere_solid( vec3 center, float radius, Context ctx )
{
	vec3  oc   = ctx.ray_origin - center;
	float b    = dot( oc, ctx.ray_dir );
	float disc = b * b - ( dot( oc, oc ) - radius * radius );

	if ( disc < 0.0 ) return -1.0;

	float sqrt_d = sqrt( disc );
	float t1     = -b - sqrt_d;

	return t1 > 0.0 ? t1 : ( -b + sqrt_d > 0.0 ? 0.0 : -1.0 );
}

void draw_segment( vec3 pos_a, vec3 pos_b, vec4 color, float alpha, inout Context ctx )
{
	float t = 0.0, dist = 1e5;

	if ( intersect_segment( pos_a, pos_b, t, dist, ctx ) )
	{
		process_hit( t, dist, alpha, color, 1, ctx );
	}
}

void draw_ring(
	vec3 center,
	vec3 axis,
	float radius,
	vec4 color,
	vec3 plane_normal,
	float tilt,
	vec4 quat,
	vec2 angles,
	bool use_xz,
	bool silhouette_mode,
	inout Context ctx
)
{
	float t_plane, t1, t2;
	bool  dot_plane = ! ( dot( plane_normal, plane_normal ) >= 0.25 ) || dot( plane_normal, ctx.ray_dir ) >= 0.0;
	bool  has_plane = intersect_plane( center, axis, t_plane, ctx );
	bool  has_cyl   = intersect_infinite_cylinder( center, axis, radius, t1, t2, ctx );

	if ( ! has_plane && ! has_cyl ) return;

	float ts[3]     = { t_plane, t1, t2 };
	bool  is_cyl[3] = { false, true, true };

	for ( int i = 0; i < 3; i++ )
	{
		bool valid = i == 0 ? has_plane : has_cyl;
		if ( ! valid || ts[ i ] <= 0.0 ) continue;

		vec3  diff      = ( ctx.ray_origin + ctx.ray_dir * ts[ i ] ) - center;
		float dot_axis  = dot( diff, axis );
		vec3  to_p_perp = diff - axis * dot_axis;
		float perp_len  = length( to_p_perp );
		float dist      = is_cyl[ i ] ? abs( dot_axis ) : abs( perp_len - radius );
		float safe_t    = safe_ray_dist( ts[ i ] );

		if ( dist > ( thickness * ( screen_space ? safe_t : 1.0 ) + safe_t * pixel_scale ) ) continue;

		vec3  local = diff * v_rot_matrix;
		float vis   = 1.0;

		if ( is_partial_angle( angles ) )
		{
			vis = get_angle_visibility( use_xz ? vec2( local.z, local.x ) : local.xy, angles, safe_t * pixel_scale );
			if ( vis <= 0.0 ) continue;
		}

		vec3 tilt_norm = normalize( ( perp_len > 1e-6 ? to_p_perp / perp_len : to_p_perp ) + axis * tilt );

		if ( silhouette_mode )
		{
			float side_dot = dot( tilt_norm, ctx.ray_dir );

			if ( dot( plane_normal, ctx.ray_dir ) > 0.0 ? ( side_dot > 0.002 ) : ( side_dot < -0.002 ) ) continue;
		}

		process_hit(
			ts[ i ],
			dist,
			( ( dot_plane && ! silhouette_mode ) ? calculate_alpha_modifier( tilt_norm, ctx.ray_dir ) : 1.0 ) * vis,
			color,
			1,
			ctx
		);
	}
}

void draw_segment_wedge(
	vec3 p1,
	vec3 p2,
	vec4 color,
	vec3 pos_a,
	vec3 axis,
	vec3 rad_start,
	vec3 rad_end,
	float radius,
	float height,
	vec3 n_start,
	vec3 n_end,
	float base_alpha,
	bool is_reflex,
	inout Context ctx
)
{
	float t = 0.0, dist = 1e5;
	if ( ! intersect_segment( p1, p2, t, dist, ctx ) ) return;

	float alpha = base_alpha;

	if ( alpha > backface_alpha + 0.001 )
	{
		bool  inside = true;
		float t_hit;

		if ( intersect_plane( pos_a, n_start, t_hit, ctx ) && t_hit > 0.0 && t_hit < t - 0.001 )
		{
			vec3 d = ctx.ray_origin + ctx.ray_dir * t_hit - pos_a;

			if ( dot( d, rad_start ) >= 0.0 && dot( d, rad_start ) <= radius && dot( d, axis ) >= 0.0 && dot( d, axis ) <= height )
			{
				inside = false;
			}
		}

		if ( inside && intersect_plane( pos_a, n_end, t_hit, ctx ) && t_hit > 0.0 && t_hit < t - 0.001 )
		{
			vec3 d = ctx.ray_origin + ctx.ray_dir * t_hit - pos_a;

			if ( dot( d, rad_end ) >= 0.0 && dot( d, rad_end ) <= radius && dot( d, axis ) >= 0.0 && dot( d, axis ) <= height )
			{
				inside = false;
			}
		}

		if ( inside )
		{
			vec3 cap_positions[2] = { pos_a + axis * height, pos_a };
			vec3 cap_normals[2]   = { axis, -axis };

			for ( int i = 0; i < 2 && inside; i++ )
			{
				if ( intersect_plane( cap_positions[ i ], cap_normals[ i ], t_hit, ctx ) && t_hit > 0.0 && t_hit < t - 0.001 )
				{
					vec3 d      = ctx.ray_origin + ctx.ray_dir * t_hit - pos_a;
					vec3 d_proj = d - axis * dot( d, axis );

					if ( dot( d_proj, d_proj ) <= radius * radius )
					{
						bool dot_start = dot( d, n_start ) <= 0.001;
						bool dot_end   = dot( d, n_end ) <= 0.001;

						if ( is_reflex ? ( dot_start || dot_end ) : ( dot_start && dot_end ) )
						{
							inside = false;
						}
					}
				}
			}
		}

		if ( inside )
		{
			float t1, t2;
			if ( intersect_infinite_cylinder( pos_a, axis, radius, t1, t2, ctx ) )
			{
				float t_c = t1 > 0.0 && t1 < t - 0.001 ? t1 : ( t2 > 0.0 && t2 < t - 0.001 ? t2 : -1.0 );

				if ( t_c > 0.0 )
				{
					vec3  hit = ctx.ray_origin + ctx.ray_dir * t_c;
					float v   = dot( hit - pos_a, axis );

					if ( v >= 0.0 && v <= height )
					{
						vec3 d         = hit - pos_a;
						bool dot_start = dot( d, n_start ) <= 0.001;
						bool dot_end   = dot( d, n_end ) <= 0.001;

						if ( is_reflex ? ( dot_start || dot_end ) : ( dot_start && dot_end ) )
						{
							inside = false;
						}
					}
				}
			}
		}

		if ( ! inside ) alpha = backface_alpha;
	}

	process_hit( t, dist, alpha, color, 1, ctx );
}

void draw_sphere_silhouette(
	vec3 center,
	float radius,
	vec4 color,
	vec3 clip_normal,
	float clip_sign,
	float clip_offset,
	vec4 quat,
	vec2 angles,
	inout Context ctx
)
{
	vec3  to_cam    = cam_pos - center;
	float dist_sq   = dot( to_cam, to_cam );
	float radius_sq = radius * radius;

	if ( dist_sq <= radius_sq ) return;

	float dist_cam_inv   = inversesqrt( dist_sq );
	float dist_cam       = dist_sq * dist_cam_inv;
	float tangent_dist   = radius_sq * dist_cam_inv;
	vec3  normal         = to_cam * dist_cam_inv;
	vec3  center_shifted = center + normal * tangent_dist;

	float t;
	if ( ! intersect_plane( center_shifted, normal, t, ctx ) ) return;

	vec3 hit_pos = ctx.ray_origin + ctx.ray_dir * t;

	if ( dot( clip_normal, clip_normal ) >= 0.25
	     && ( dot( hit_pos - center, clip_normal ) * clip_sign ) <= ( clip_offset * clip_sign - 0.001 ) )
	{
		return;
	}

	float vis = 1.0;
	if ( is_partial_angle( angles ) )
	{
		vis = get_angle_visibility( ( ( hit_pos - center ) * v_rot_matrix ).xy, angles, safe_ray_dist( t ) * pixel_scale );
	}

	if ( vis > 0.0 )
	{
		process_hit(
			t,
			abs( length( hit_pos - center_shifted ) - sqrt( radius_sq - tangent_dist * tangent_dist ) ),
			vis,
			color,
			1,
			ctx
		);
	}
}

void draw_cone_silhouette( vec3 pos_a, vec3 pos_b, vec2 radii, vec4 color, vec4 quat, vec2 angles, inout Context ctx )
{
	vec3  seg     = pos_b - pos_a;
	float seg_len = length( seg );

	if ( seg_len < 1e-6 ) return;

	vec3 axis = seg / seg_len;
	vec3 starts[2], ends[2], local_ps[2];
	bool valid = false;

	if ( abs( radii.x - radii.y ) < 1e-3 )
	{
		vec3  cam_rel     = cam_pos - pos_a;
		vec3  cam_proj    = cam_rel - axis * dot( cam_rel, axis );
		float cam_proj_sq = dot( cam_proj, cam_proj );

		if ( cam_proj_sq > radii.x * radii.x )
		{
			float dist_inv   = inversesqrt( cam_proj_sq );
			float proj_len   = radii.x * radii.x * dist_inv;
			vec3  cam_norm   = cam_proj * dist_inv;
			vec3  tan_center = pos_a + cam_norm * proj_len;
			vec3  tan_dir    = normalize( cross( axis, cam_norm ) );
			float half_w     = sqrt( radii.x * radii.x - proj_len * proj_len );

			starts[0]   = tan_center + tan_dir * half_w;
			starts[1]   = tan_center - tan_dir * half_w;
			ends[0]     = starts[0] + seg;
			ends[1]     = starts[1] + seg;
			local_ps[0] = ( starts[0] - pos_a ) * v_rot_matrix;
			local_ps[1] = ( starts[1] - pos_a ) * v_rot_matrix;
			valid       = true;
		}
	}
	else
	{
		bool  swap      = radii.x < radii.y;
		vec3  base_pos  = swap ? pos_b : pos_a;
		vec3  top_pos   = swap ? pos_a : pos_b;
		vec3  cone_axis = swap ? -axis : axis;
		float base_r    = max( radii.x, radii.y );
		float top_r     = min( radii.x, radii.y );
		vec3  apex_pos  = base_pos + cone_axis * ( base_r * seg_len / ( base_r - top_r ) );
		vec3  cam_apex  = cam_pos - apex_pos;
		float apex_axis = dot( cam_apex, cone_axis );

		if ( abs( apex_axis ) < 1e-4 )
		{
			vec3  tan_dir = normalize( cross( cone_axis, normalize( cam_apex ) ) ) * base_r;
			float ratio   = top_r / base_r;

			starts[0]   = base_pos + tan_dir;
			starts[1]   = base_pos - tan_dir;
			ends[0]     = top_pos + tan_dir * ratio;
			ends[1]     = top_pos - tan_dir * ratio;
			local_ps[0] = ( starts[0] - pos_a ) * v_rot_matrix;
			local_ps[1] = ( starts[1] - pos_a ) * v_rot_matrix;
			valid       = true;
		}
		else
		{
			vec3  diff    = ( apex_pos + cam_apex * ( dot( base_pos - apex_pos, cone_axis ) / apex_axis ) ) - base_pos;
			float diff_sq = dot( diff, diff );

			if ( diff_sq > base_r * base_r )
			{
				float diff_len_inv = inversesqrt( diff_sq );
				float cos_t        = base_r * diff_len_inv;
				float sin_t        = sqrt( 1.0 - cos_t * cos_t );
				vec3  rad_dir      = diff * diff_len_inv;
				vec3  tangent      = cross( cone_axis, rad_dir );
				float ratio        = top_r / base_r;

				starts[0]   = base_pos + ( ( rad_dir * cos_t + tangent * sin_t ) * base_r );
				starts[1]   = base_pos + ( ( rad_dir * cos_t - tangent * sin_t ) * base_r );
				ends[0]     = apex_pos + ( starts[0] - apex_pos ) * ratio;
				ends[1]     = apex_pos + ( starts[1] - apex_pos ) * ratio;
				local_ps[0] = ( starts[0] - pos_a ) * v_rot_matrix;
				local_ps[1] = ( starts[1] - pos_a ) * v_rot_matrix;
				valid       = true;
			}
		}
	}

	if ( valid )
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( get_angle_visibility( vec2( local_ps[ i ].z, local_ps[ i ].x ), angles, 1.0 ) > 0.0 )
			{
				draw_segment( starts[ i ], ends[ i ], color, 1, ctx );
			}
		}
	}
}

void draw_cone_solid(
	vec3 pos_a,
	vec3 pos_b,
	vec2 radii,
	vec4 color,
	float alpha,
	vec4 quat,
	vec2 angles,
	bool use_xz,
	inout Context ctx
)
{
	vec3  seg      = pos_b - pos_a;
	float axis_len = length( seg );

	if ( axis_len < 1e-6 ) return;

	vec3  axis  = seg / axis_len;
	float t_min = 1e5;
	float t1    = -1.0, t2 = -1.0;
	bool  hit   = false;

	if ( abs( radii.x - radii.y ) > 1e-3 )
	{
		bool  swap        = radii.x < radii.y;
		vec3  cone_axis   = swap ? -axis : axis;
		float base_r      = max( radii.x, radii.y );
		float apex_height = base_r * axis_len / ( base_r - min( radii.x, radii.y ) );
		vec3  apex        = ctx.ray_origin - ( ( swap ? pos_b : pos_a ) + cone_axis * apex_height );
		vec3  axis_neg    = -cone_axis;
		float h_sq        = apex_height * apex_height;
		float cos_sq      = h_sq / ( h_sq + base_r * base_r );
		float d_axis      = dot( ctx.ray_dir, axis_neg );
		float a_axis      = dot( apex, axis_neg );

		hit = solve_quadratic(
			d_axis * d_axis - cos_sq * dot( ctx.ray_dir, ctx.ray_dir ),
			2.0 * ( d_axis * a_axis - cos_sq * dot( ctx.ray_dir, apex ) ),
			a_axis * a_axis - cos_sq * dot( apex, apex ),
			t1, t2
		);
	}
	else
	{
		hit = intersect_infinite_cylinder( pos_a, axis, radii.x, t1, t2, ctx );
	}

	if ( hit )
	{
		float ts[2] = { t1, t2 };

		for ( int i = 0; i < 2; i++ )
		{
			float t = ts[ i ];
			if ( t <= 0.0 ) continue;

			vec3  hit_pos = ctx.ray_origin + ctx.ray_dir * t;
			float d       = dot( hit_pos - pos_a, axis );

			if ( d >= 0.0 && d <= axis_len )
			{
				vec3 local = ( hit_pos - pos_a ) * v_rot_matrix;

				if ( get_angle_visibility( use_xz ? vec2( local.z, local.x ) : local.xy, angles, max( t, 1e-4 ) * pixel_scale ) > 0.0 )
				{
					t_min = min( t_min, t );
				}
			}
		}
	}

	if ( t_min < 1e4 )
	{
		process_hit( t_min, 0, alpha, color, 1, ctx );
	}
}

void draw_sphere( vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	ShapeFlags sf      = parse_flags( int( params.y ) );
	vec3       center  = pos_a;
	float      radius  = pos_b.x;
	bool       partial = is_partial_angle( params.zw );

	AngleWedge w;
	if ( partial ) w = make_angle_wedge( params.zw, quat );

	if ( sf.screen_scale ) radius *= length( center - ctx.ray_origin );

	if ( ! sf.filled )
	{
		if ( ! sf.is_sil )
		{
			draw_ring( center, v_rot_matrix[0], radius, color, ZERO3, 0.0, quat, params.zw, false, false, ctx );
			draw_ring( center, v_rot_matrix[1], radius, color, ZERO3, 0.0, quat, params.zw, false, false, ctx );
			draw_ring( center, v_rot_matrix[2], radius, color, ZERO3, 0.0, quat, params.zw, false, false, ctx );
		}

		if ( partial )
		{
			draw_ring(
				center,
				w.normal1,
				radius,
				color,
				-w.normal1,
				0,
				quat,
				vec2( params.z - 90.0, params.z + 90.0 ),
				false,
				sf.is_sil,
				ctx
			);
			draw_ring(
				center,
				w.normal2,
				radius,
				color,
				w.normal2,
				0,
				quat,
				vec2( params.w - 90.0, params.w + 90.0 ),
				false,
				sf.is_sil,
				ctx
			);

			if ( abs( abs( params.z - params.w ) - 180.0 ) > 0.001 )
			{
				float d1 = dot( -w.normal1, ctx.ray_dir );
				float d2 = dot( w.normal2, ctx.ray_dir );

				if ( ! sf.is_sil || ( ( d1 > 0.0 ) != ( d2 > 0.0 ) ) )
				{
					float diff   = params.w - params.z; if ( diff < 0.0 ) diff += 360.0;
					bool  f1_vis = d1 < 0.001;
					bool  f2_vis = d2 < 0.001;
					vec3  z_axis = v_rot_matrix * vec3( 0, 0, radius );

					draw_segment(
						center + z_axis,
						center - z_axis,
						color,
						( diff > 180.0 ? ( f1_vis && f2_vis ) : ( f1_vis || f2_vis ) ) ? 1.0 : backface_alpha,
						ctx
					);
				}
			}
		}

		draw_sphere_silhouette( center, radius, color, ZERO3, 0, 0, quat, params.zw, ctx );
	}

	if ( sf.filled || sf.has_bg )
	{
		float base_alpha = sf.filled ? 1.0 : backface_alpha;
		float t          = intersect_sphere_solid( center, radius, ctx );

		if ( t >= 0.0 )
		{
			float vis = get_angle_visibility(
				( ( ctx.ray_origin + ctx.ray_dir * t - center ) * v_rot_matrix ).xy,
				params.zw,
				safe_ray_dist( t ) * pixel_scale
			);

			if ( vis > 0.0 )
			{
				process_hit( t, 0, base_alpha * vis, color, 1, ctx );
			}
		}

		if ( partial )
		{
			intersect_halfplane( center, -w.normal1, w.radial1, radius, color, base_alpha, ctx );
			intersect_halfplane( center, w.normal2, w.radial2, radius, color, base_alpha, ctx );
		}
	}
}

void draw_triangle( vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	vec3       pos_c = quat.xyz;
	ShapeFlags sf    = parse_flags( int( params.y ) );

	if ( ! sf.filled )
	{
		draw_segment( pos_a, pos_b, color, 1, ctx );
		draw_segment( pos_b, pos_c, color, 1, ctx );
		draw_segment( pos_c, pos_a, color, 1, ctx );
	}

	if ( sf.filled || sf.has_bg )
	{
		vec3  edge1 = pos_b - pos_a;
		vec3  edge2 = pos_c - pos_a;
		vec3  h     = cross( ctx.ray_dir, edge2 );
		float det   = dot( edge1, h );

		if ( abs( det ) > 1e-5 )
		{
			float inv_det = 1.0 / det;
			vec3  s       = ctx.ray_origin - pos_a;
			float bary_u  = inv_det * dot( s, h );

			if ( bary_u >= 0.0 && bary_u <= 1.0 )
			{
				vec3  q      = cross( s, edge1 );
				float bary_v = inv_det * dot( ctx.ray_dir, q );

				if ( bary_v >= 0.0 && bary_u + bary_v <= 1.0 )
				{
					float t = inv_det * dot( edge2, q );
					if ( t > 1e-4 ) process_hit( t, 0, sf.filled ? 1.0 : backface_alpha, color, 1, ctx );
				}
			}
		}
	}
}

void draw_box( vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	vec2  he_a       = pos_b.xy * 0.5;
	vec2  he_b       = params.yz;
	float he_z       = pos_b.z * 0.5;
	bool  is_frustum = abs( he_a.x - he_b.x ) > 1e-4 || abs( he_a.y - he_b.y ) > 1e-4;
	vec3  local_ro   = ( ctx.ray_origin - pos_a ) * v_rot_matrix;
	vec3  local_rd   = ctx.ray_dir * v_rot_matrix;

	if ( ! is_frustum ) he_b = he_a;

	ShapeFlags sf     = parse_flags( int( params.w ) );
	vec3       norm_l = BOX_NORMALS[0];
	vec3       norm_r = BOX_NORMALS[1];
	vec3       norm_b = BOX_NORMALS[2];
	vec3       norm_t = BOX_NORMALS[3];

	if ( is_frustum )
	{
		float dx  = he_a.x - he_b.x;
		float dy  = he_a.y - he_b.y;
		float len = 2.0 * he_z;

		norm_r = normalize( vec3( len, 0, dx ) );
		norm_l = normalize( vec3(-len, 0, dx ) );
		norm_t = normalize( vec3( 0, len, dy ) );
		norm_b = normalize( vec3( 0,-len, dy ) );
	}

	if ( ! sf.filled )
	{
		vec3 p[8] = {
			vec3(-he_a.x,-he_a.y,-he_z ),
			vec3( he_a.x,-he_a.y,-he_z ),
			vec3( he_a.x, he_a.y,-he_z ),
			vec3(-he_a.x, he_a.y,-he_z ),
			vec3(-he_b.x,-he_b.y, he_z ),
			vec3( he_b.x,-he_b.y, he_z ),
			vec3( he_b.x, he_b.y, he_z ),
			vec3(-he_b.x, he_b.y, he_z )
		};

		bool face_back[6];
		if ( sf.is_sil )
		{
			face_back[0] = dot( norm_l, local_rd ) > 0.0;
			face_back[1] = dot( norm_r, local_rd ) > 0.0;
			face_back[2] = dot( norm_b, local_rd ) > 0.0;
			face_back[3] = dot( norm_t, local_rd ) > 0.0;
			face_back[4] = dot( BOX_NORMALS[4], local_rd ) > 0.0;
			face_back[5] = dot( BOX_NORMALS[5], local_rd ) > 0.0;
		}

		float   t         = 0.0, dist = 1e5;
		int     best_j    = -1;
		Context local_ctx = make_local_context( ctx, local_ro, local_rd );

		for ( int j = 0; j < 48; j += 4 )
		{
			if ( sf.is_sil && face_back[ BOX_EDGES[ j + 2 ] ] == face_back[ BOX_EDGES[ j + 3 ] ] ) continue;

			if ( intersect_segment( p[ BOX_EDGES[ j ] ], p[ BOX_EDGES[ j + 1 ] ], t, dist, local_ctx ) )
			{
				best_j = j;
			}
		}

		if ( best_j >= 0 )
		{
			vec3 n1 = get_box_face_normal( BOX_EDGES[ best_j + 2 ], norm_l, norm_r, norm_b, norm_t );
			vec3 n2 = get_box_face_normal( BOX_EDGES[ best_j + 3 ], norm_l, norm_r, norm_b, norm_t );

			process_hit(
				t,
				dist,
				( dot( n1, local_rd ) > 0.0 && dot( n2, local_rd ) > 0.0 ) ? backface_alpha : 1.0,
				color,
				1,
				ctx
			);
		}
	}

	if ( sf.filled || sf.has_bg )
	{
		float t_start = 0.0, t_end = 1e5;

		if ( ! is_frustum )
		{
			if ( ! intersect_aabb( local_ro, 1.0 / local_rd, pos_b * 0.5, t_start, t_end ) ) return;
		}
		else
		{
			vec3 N[4] = { norm_r, norm_l, norm_t, norm_b };
			vec3 P[4] = { vec3( he_a.x, 0, -he_z ), vec3( -he_a.x, 0, -he_z ), vec3( 0, he_a.y, -he_z ), vec3( 0, -he_a.y, -he_z ) };

			for ( int i = 0; i < 4; i++ )
			{
				float denom = dot( local_rd, N[ i ] );
				float numer = dot( local_ro - P[ i ], N[ i ] );

				if ( abs( denom ) < 1e-6 )
				{
					if ( numer > 1e-6 ) return;
				}
				else
				{
					float t = -numer / denom;
					if ( denom < 0.0 ) t_start = max( t_start, t ); else t_end = min( t_end, t );
				}
			}

			float inv_z = 1.0 / local_rd.z;
			float tz1   = ( -he_z - local_ro.z ) * inv_z;
			float tz2   = ( he_z - local_ro.z ) * inv_z;

			t_start = max( t_start, min( tz1, tz2 ) );
			t_end   = min( t_end, max( tz1, tz2 ) );
		}

		if ( t_start < t_end && t_end > 0.0 )
		{
			float t_min         = max( 0.0, t_start );
			vec3  hit_pos_local = local_ro + local_rd * t_min;
			bool  valid         = true;

			if ( is_frustum )
			{
				vec2 he_at_z = mix( he_a, he_b, ( hit_pos_local.z + he_z ) / ( 2.0 * he_z ) );

				if ( abs( hit_pos_local.x ) > he_at_z.x + 1e-4 || abs( hit_pos_local.y ) > he_at_z.y + 1e-4 )
				{
					valid = false;
				}
			}

			if ( valid ) process_hit( t_min, 0, sf.filled ? 1.0 : backface_alpha, color, 1, ctx );
		}
	}
}

void draw_cylinder_or_capsule( vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, bool is_capsule, inout Context ctx )
{
	float      radius_a = params.y, radius_b = params.z;
	ShapeFlags sf       = parse_flags( int( params.w ) );
	vec3       ba       = pos_b - pos_a;
	float      height   = length( ba );
	vec3       axis     = ba / height;
	float      sin_a    = 0.0;
	float      cos_a    = 1.0;
	vec3       center_a = pos_a;
	vec3       center_b = pos_b;
	vec2       rads     = vec2( radius_a, radius_b );

	if ( is_capsule )
	{
		sin_a    = clamp( ( radius_b - radius_a ) / height, -0.999, 0.999 );
		cos_a    = sqrt( 1.0 - sin_a * sin_a );
		center_a = pos_a - axis * ( radius_a * sin_a );
		center_b = pos_b - axis * ( radius_b * sin_a );
		rads     = vec2( radius_a, radius_b ) * cos_a;
	}

	float tilt = is_capsule ? -sin_a / cos_a : ( radius_a - radius_b ) / height;

	if ( ! sf.filled )
	{
		if ( ! is_capsule || ! sf.is_sil )
		{
			vec3  centers[2] = { is_capsule ? center_a : pos_a, is_capsule ? center_b : pos_b };
			float radii[2]   = { rads.x, rads.y };
			vec3  normals[2] = { is_capsule ? ZERO3 : -axis, is_capsule ? ZERO3 : axis };

			for ( int i = 0; i < 2; i++ )
			{
				if ( radii[ i ] > 1e-3 )
				{
					draw_ring(
						centers[ i ],
						axis,
						radii[ i ],
						color,
						normals[ i ],
						tilt,
						quat,
						vec2( 0, 360 ),
						false,
						sf.is_sil,
						ctx
					);
				}
			}
		}

		if ( ! sf.is_sil )
		{
			vec3 right, forward;
			get_ortho_basis( axis, quat, right, forward );

			vec3  pos_A      = is_capsule ? center_a : pos_a;
			vec3  pos_B      = is_capsule ? center_b : pos_b;
			vec3  seg        = pos_B - pos_A;
			float seg_len_sq = dot( seg, seg );

			if ( seg_len_sq < 1e-6 ) return;

			vec3 axis       = seg * inversesqrt( seg_len_sq );
			vec3 offsets[4] = { right, -right, forward, -forward };

			for ( int j = 0; j < 4; j++ )
			{
				vec3  offset = offsets[ j ];
				vec3  p1     = pos_A + offset * rads.x;
				vec3  p2     = pos_B + offset * rads.y;

				float t = 0.0, dist = 1e5;
				if ( intersect_segment( p1, p2, t, dist, ctx ) )
				{
					vec3 normal = normalize( cross( cross( axis, offset ), p2 - p1 ) );
					normal *= sign( dot( normal, offset ) + 1e-6 );

					process_hit( t, dist, calculate_alpha_modifier( normal, ctx.ray_dir ), color, 1, ctx );
				}
			}
		}

		if ( is_capsule && ! sf.is_sil )
		{
			vec3 right, forward;
			get_ortho_basis( axis, quat, right, forward );

			for ( int j = 0; j < 2; j++ )
			{
				vec3  center       = j == 0 ? pos_a : pos_b;
				float radius       = j == 0 ? radius_a : radius_b;
				float limit        = -radius * sin_a;
				vec3  ring_axes[2] = { right, forward };

				for ( int k = 0; k < 2; k++ )
				{
					float t_plane, t1, t2;
					vec3  normal    = ring_axes[ k ];
					bool  has_plane = intersect_plane( center, normal, t_plane, ctx );
					bool  has_cyl   = intersect_infinite_cylinder( center, normal, radius, t1, t2, ctx );

					for ( int i = 0; i < 3; i++ )
					{
						bool  is_cyl = i > 0;
						float t      = i == 0 ? t_plane : ( i == 1 ? t1 : t2 );

						if ( ( is_cyl ? has_cyl : has_plane ) && t > 0.0 )
						{
							vec3  p       = ctx.ray_origin + ctx.ray_dir * t;
							float dot_val = dot( p - center, axis );

							if ( j == 0 ? dot_val < limit : dot_val > limit )
							{
								process_hit(
									t,
									is_cyl ? abs( dot( p - center, normal ) ) : abs( length( p - center ) - radius ),
									calculate_alpha_modifier( normalize( p - center ), ctx.ray_dir ),
									color,
									1,
									ctx
								);
							}
						}
					}
				}
			}
		}

		if ( is_capsule )
		{
			draw_sphere_silhouette( pos_a, radius_a, color, axis, -1, -radius_a * sin_a, quat, vec2( 0, 360 ), ctx );
			draw_sphere_silhouette( pos_b, radius_b, color, axis, 1, -radius_b * sin_a, quat, vec2( 0, 360 ), ctx );
		}

		draw_cone_silhouette(
			is_capsule ? center_a : pos_a,
			is_capsule ? center_b : pos_b,
			rads,
			color,
			quat,
			vec2( 0, 360 ),
			ctx
		);
	}

	if ( sf.filled || sf.has_bg )
	{
		float alpha = sf.filled ? 1.0 : backface_alpha;

		if ( is_capsule )
		{
			vec3  sphere_centers[2] = { pos_a, pos_b };
			float sphere_radii[2]   = { radius_a, radius_b };

			for ( int i = 0; i < 2; i++ )
			{
				float t = intersect_sphere_solid( sphere_centers[ i ], sphere_radii[ i ], ctx );

				if ( t >= 0.0 ) process_hit( t, 0, alpha, color, 1, ctx );
			}
		}

		draw_cone_solid(
			is_capsule ? center_a : pos_a,
			is_capsule ? center_b : pos_b,
			rads,
			color,
			alpha,
			quat,
			vec2( 0, 360 ),
			false,
			ctx
		);

		if ( ! is_capsule )
		{
			intersect_disk_simple( pos_a, -axis, radius_a, color, alpha, ctx );
			intersect_disk_simple( pos_b, axis, radius_b, color, alpha, ctx );
		}
	}
}

void draw_cylinder_arc( vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	ShapeFlags sf      = parse_flags( int( params.y ) );
	float      arc_len = pos_b.x;
	float      arc_rad = pos_b.y;
	vec3       axis    = v_rot_matrix * vec3(0,1,0);
	vec3       p_end   = pos_a + axis * arc_len;
	bool       partial = is_partial_angle( params.zw );

	if ( ! sf.filled )
	{
		draw_ring( pos_a, axis, arc_rad, color, -axis, 0, quat, params.zw, true, sf.is_sil, ctx );
		draw_ring( p_end, axis, arc_rad, color, axis, 0, quat, params.zw, true, sf.is_sil, ctx );

		if ( partial )
		{
			float diff       = params.w - params.z; if ( diff < 0.0 ) diff += 360.0;
			bool  is_reflex  = diff > 180.0;
			float rad_z      = radians( params.z );
			float rad_w      = radians( params.w );
			vec3  rad_start  = v_rot_matrix * vec3( sin( rad_z ), 0, cos( rad_z ) );
			vec3  rad_end    = v_rot_matrix * vec3( sin( rad_w ), 0, cos( rad_w ) );
			vec3  n_start    = normalize( cross( rad_start, axis ) );
			vec3  n_end      = normalize( cross( axis, rad_end ) );
			float dn_start   = dot( n_start, ctx.ray_dir );
			float dn_end     = dot( n_end, ctx.ray_dir );
			float dn_top     = dot( axis, ctx.ray_dir );
			float dn_bot     = dot( -axis, ctx.ray_dir );
			float dn_rad_s   = dot( rad_start, ctx.ray_dir );
			float dn_rad_e   = dot( rad_end, ctx.ray_dir );
			vec3  corners[4] = {
				pos_a + rad_start * arc_rad,
				pos_a + rad_end * arc_rad,
				p_end + rad_start * arc_rad,
				p_end + rad_end * arc_rad
			};
			vec3  starts[7]  = { pos_a, pos_a, p_end, p_end, pos_a, corners[0], corners[1] };
			vec3  ends[7]    = { corners[0], corners[1], corners[2], corners[3], p_end, corners[2], corners[3] };
			float alphas[7]  = {
				mix( 1, backface_alpha, step( 0, dn_bot ) * step( 0, dn_start ) ),
				mix( 1, backface_alpha, step( 0, dn_bot ) * step( 0, dn_end ) ),
				mix( 1, backface_alpha, step( 0, dn_top ) * step( 0, dn_start ) ),
				mix( 1, backface_alpha, step( 0, dn_top ) * step( 0, dn_end ) ),
				mix( 1, backface_alpha, step( 0, dn_start ) * step( 0, dn_end ) ),
				mix( 1, backface_alpha, step( 0, dn_start ) * step( 0, dn_rad_s ) ),
				mix( 1, backface_alpha, step( 0, dn_end ) * step( 0, dn_rad_e ) )
			};
			bool  is_sil[7]  = {
				( dn_start > 0.0 ) != ( dn_bot > 0.0 ),
				( dn_end > 0.0 ) != ( dn_bot > 0.0 ),
				( dn_start > 0.0 ) != ( dn_top > 0.0 ),
				( dn_end > 0.0 ) != ( dn_top > 0.0 ),
				( dn_start > 0.0 ) != ( dn_end > 0.0 ),
				( dn_start > 0.0 ) != ( dn_rad_s > 0.0 ),
				( dn_end > 0.0 ) != ( dn_rad_e > 0.0 )
			};

			for ( int i = 0; i < 7; i++ )
			{
				if ( ! sf.is_sil || is_sil[ i ] )
				{
					draw_segment_wedge(
						starts[ i ],
						ends[ i ],
						color,
						pos_a,
						axis,
						rad_start,
						rad_end,
						arc_rad,
						arc_len,
						n_start,
						n_end,
						alphas[ i ],
						is_reflex,
						ctx
					);
				}
			}
		}

		vec3  local_norm = normalize( cross( ctx.ray_dir, axis ) ) * v_rot_matrix;
		float ang_sil    = degrees( atan( local_norm.x, local_norm.z ) );
		float range      = params.w - params.z; if ( range < 0.0 ) range += 360.0;

		for ( int i = 0; i < 2; i++ )
		{
			float a     = ang_sil + float( i ) * 180.0;
			float rel_a = a - params.z; rel_a -= floor( rel_a / 360.0 ) * 360.0;

			if ( rel_a > 1.0 && rel_a < range - 1.0 )
			{
				draw_cone_silhouette( pos_a, p_end, vec2( arc_rad ), color, quat, vec2( a - 0.5, a + 0.5 ), ctx );
			}
		}
	}

	if ( sf.filled || sf.has_bg )
	{
		float a = sf.filled ? 1.0 : backface_alpha;

		draw_cone_solid( pos_a, p_end, vec2( arc_rad ), color, a, quat, params.zw, true, ctx );

		if ( partial )
		{
			float rad_z    = radians( params.z );
			float rad_w    = radians( params.w );
			vec3 rad_start = v_rot_matrix * vec3( sin( rad_z ), 0, cos( rad_z ) );
			vec3 rad_end   = v_rot_matrix * vec3( sin( rad_w ), 0, cos( rad_w ) );

			intersect_rect_solid( pos_a, rad_start, axis, vec2( arc_rad, arc_len ), color, a, ctx );
			intersect_rect_solid( pos_a, axis, rad_end, vec2( arc_len, arc_rad ), color, a, ctx );
			draw_segment( pos_a + rad_start * arc_rad, p_end + rad_start * arc_rad, color, 1, ctx );
			draw_segment( pos_a + rad_end * arc_rad, p_end + rad_end * arc_rad, color, 1, ctx );
		}

		intersect_disk( pos_a, -axis, arc_rad, color, a, quat, pos_a, params.zw, true, ctx );
		intersect_disk( p_end, axis, arc_rad, color, a, quat, pos_a, params.zw, true, ctx );
	}
}

void draw_arc( vec3 center, vec3 radius_info, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	vec3       lro     = ( ctx.ray_origin - center ) * v_rot_matrix;
	vec3       lrd     = ctx.ray_dir * v_rot_matrix;
	ShapeFlags sf      = parse_flags( int( params.w ) );
	Context    lctx    = make_local_context( ctx, lro, lrd );
	float      ang_a   = params.y;
	float      ang_b   = params.z;
	float      t_plane = ( abs( lrd.z ) > 1e-5 ) ? -lro.z / lrd.z : -1.0;
	float      t1      = -1.0;
	float      t2      = -1.0;
	bool       has_cyl = ! sf.filled && solve_quadratic( dot( lrd.xy, lrd.xy ), 2.0 * dot( lro.xy, lrd.xy ),
	                                                     dot( lro.xy, lro.xy ) - radius_info.x * radius_info.x, t1, t2 );
	float      ts[3]   = { t_plane, t1, t2 };

	for ( int i = 0; i < 3; i++ )
	{
		if ( ts[ i ] > 0.0 && ( i == 0 || has_cyl ) )
		{
			float ti     = ts[ i ];
			vec3  p      = lro + lrd * ti;
			float len    = length( p.xy );
			float ang    = degrees( atan( p.y, p.x ) ); if ( ang < 0.0 ) ang += 360.0;
			bool  ang_ge = ang >= ang_a;
			bool  ang_le = ang <= ang_b;

			if ( ang_a < ang_b ? ( ang_ge && ang_le ) : ( ang_ge || ang_le ) )
			{
				if ( sf.has_bg && i == 0 )
				{
					process_hit( ti, len < radius_info.x ? 0.0 : 1e5, backface_alpha, color, 1, ctx );
				}

				if ( sf.filled )
				{
					if ( i == 0 ) process_hit( ti, len < radius_info.x ? 0.0 : 1e5, 1, color, 2, ctx );
				}
				else
				{
					process_hit( ti, i == 0 ? abs( len - radius_info.x ) : abs( p.z ), 1, color, 2, ctx );
				}
			}
		}
	}

	if ( sf.filled )
	{
		float angles[2] = { radians( params.y ), radians( params.z ) };

		for ( int i = 0; i < 2; i++ )
		{
			float t, dist;
			if ( intersect_segment( ZERO3, vec3( cos( angles[ i ] ), sin( angles[ i ] ), 0 ) * radius_info.x, t, dist, lctx ) )
			{
				process_hit( t, dist, 1, color, 1, ctx );
			}
		}
	}
}

void draw_shape( int shape_type, vec3 pos_a, vec3 pos_b, vec4 color, vec4 quat, vec4 params, inout Context ctx )
{
	switch ( shape_type )
	{
		case 0: draw_sphere( pos_a, pos_b, color, quat, params, ctx ); break;
		case 1: draw_box( pos_a, pos_b, color, quat, params, ctx ); break;
		case 2: draw_segment( pos_a, pos_b, color, 1, ctx ); break;
		case 3: draw_arc( pos_a, pos_b, color, quat, params, ctx ); break;
		case 4: draw_cylinder_or_capsule( pos_a, pos_b, color, quat, params, true, ctx ); break;
		case 5: draw_cylinder_or_capsule( pos_a, pos_b, color, quat, params, false, ctx ); break;
		case 6: draw_triangle( pos_a, pos_b, color, quat, params, ctx ); break;
		case 7: draw_cylinder_arc( pos_a, pos_b, color, quat, params, ctx ); break;
	}
}

void vertex()
{
	instance_idx    = INSTANCE_ID;
	pixel_world_pos = ( MODEL_MATRIX * vec4( VERTEX, 1 ) ).xyz;
	v_data0         = fetch_shape_data( instance_idx, 0 );
	v_data1         = fetch_shape_data( instance_idx, 1 );
	v_data2         = fetch_shape_data( instance_idx, 2 );
	v_data3         = fetch_shape_data( instance_idx, 3 );
	v_data4         = fetch_shape_data( instance_idx, 4 );
	v_rot_matrix    = quat_to_mat3( v_data3 );
	v_aabb_extents  = vec3( 1e5 );
	v_cull_height   = 1e5;

	int type = int( v_data4.x );

	if ( type == 0 )
	{
		v_cull_center  = v_data0.xyz;
		v_cull_radius  = v_data1.x;
		v_aabb_extents = vec3( v_data1.x );
	}
	else if ( type == 1 )
	{
		vec2 he_a = v_data1.xy * 0.5;
		vec2 he_b = v_data4.yz; if ( ! ( abs( he_a.x - he_b.x ) > 1e-4 || abs( he_a.y - he_b.y ) > 1e-4 ) ) he_b = he_a;

		v_cull_center  = v_data0.xyz;
		v_cull_radius  = length( v_data1.xyz );
		v_aabb_extents = vec3( max( he_a.x, he_b.x ), max( he_a.y, he_b.y ), v_data1.z * 0.5 ) + ( thickness * 10.0 + 0.1 );
	}
	else if ( type == 7 )
	{
		vec3  axis = v_rot_matrix * vec3( 0, 1, 0 );
		float len  = v_data1.x;
		float rad  = v_data1.y;

		v_cull_center    = v_data0.xyz + axis * ( len * 0.5 );
		v_cull_radius    = length( vec2( len * 0.5, rad ) );
		v_cull_height    = len * 0.5;
		v_aabb_extents.x = rad;
	}
	else if ( type == 2 || type == 4 || type == 5 )
	{
		float half_len = length( v_data0.xyz - v_data1.xyz ) * 0.5;
        float radius   = ( type == 2 ? thickness : max( v_data4.y, v_data4.z ) );

		v_cull_center    = ( v_data0.xyz + v_data1.xyz ) * 0.5;
        v_cull_radius    = half_len + radius;
        v_cull_height    = half_len; if ( type == 4 ) v_cull_height += radius;
        v_aabb_extents.x = radius;
	}
	else
	{
		v_cull_center = v_data0.xyz;
		v_cull_radius = 1e5;
	}

	float pad = thickness * ( screen_space ? length( cam_pos - v_cull_center ) * pixel_scale : 1.0 ) * 4.0 + 0.05;
	v_cull_radius += pad;

	if ( type == 0 || type == 1 )
	{
		v_aabb_extents += vec3( pad );
	}
	else
	{
		v_cull_height    += pad;
		v_aabb_extents.x += pad;
	}
}

void fragment()
{
	Context ctx = Context( cam_pos, normalize( pixel_world_pos - cam_pos ), Hit( 1e5, vec4(0) ), Hit( 1e5, vec4(0) ), Hit( 1e5, vec4(0) ) );
	vec3    oc  = ctx.ray_origin - v_cull_center;
	float   b   = dot( oc, ctx.ray_dir );
	float   c   = dot( oc, oc ) - v_cull_radius * v_cull_radius;
	float   ds  = b * b - c;

	if ( ( c > 0.0 && b > 0.0 ) || ds < 0.0 )
	{
		discard;
	}
	else
	{
		float bounds = b - sqrt( ds );
		int   type   = int( v_data4.x );

		if ( type == 1 )
		{
			vec3  local  = ( ctx.ray_origin - v_data0.xyz ) * v_rot_matrix;
			vec3  inv_rd = 1.0 / ( ( ctx.ray_dir * v_rot_matrix ) + vec3( 1e-6 ) );

			float t_enter, t_exit;
			if ( ! intersect_aabb( local, inv_rd, v_aabb_extents, t_enter, t_exit ) ) discard;

			bounds = t_enter;
		}
		else if ( type == 2 || type == 4 || type == 5 || type == 7 || type == 8 || type == 9 )
		{
			vec3 axis;

			if ( type == 7 )
			{
				axis = v_rot_matrix * vec3( 0, 1, 0 );
			}
			else
			{
				vec3  diff = v_data1.xyz - v_data0.xyz;
				float l    = length( diff );

				axis = l < 1e-5 ? vec3( 0, 1, 0 ) : diff / l;
			}

			vec3  oc      = ctx.ray_origin - v_cull_center;
			float ro_y    = dot( oc, axis );
			float rd_y    = dot( ctx.ray_dir, axis );
			vec3  ro_xz   = oc - axis * ro_y;
			vec3  rd_xz   = ctx.ray_dir - axis * rd_y;
			float radius  = v_aabb_extents.x;
			float hheight = v_cull_height;
			float A       = dot( rd_xz, rd_xz );
			float B       = dot( ro_xz, rd_xz );
			float C       = dot( ro_xz, ro_xz ) - radius * radius;
			float disc    = B * B - A * C;
			float t_min   = 1e5;

			if ( disc >= 0.0 )
			{
				float sqrt_disc = sqrt( disc );
				vec2  ts        = vec2( -B - sqrt_disc, -B + sqrt_disc ) * ( 1.0 / ( A + 1e-6 ) );
				vec2  ys        = ro_y + rd_y * ts;

				if ( abs( ys.x ) <= hheight && ts.x > 0.0 ) t_min = ts.x;
				if ( abs( ys.y ) <= hheight && ts.y > 0.0 && ts.y < t_min ) t_min = ts.y;
			}

			vec2 t_caps = vec2( hheight - ro_y, -hheight - ro_y ) * ( 1.0 / ( rd_y + sign( rd_y ) * 1e-6 ) );

			for ( int i = 0; i < 2; i++ )
			{
				float t = i == 0 ? t_caps.x : t_caps.y;

				if ( t > 0.0 && t < t_min )
				{
					vec3 p = ro_xz + rd_xz * t;

					if ( dot( p, p ) <= radius * radius ) t_min = t;
				}
			}

			if ( t_min >= 1e4 ) discard;

			bounds = t_min;
		}

		draw_shape( int( v_data4.x ), v_data0.xyz, v_data1.xyz, v_data2, v_data3, v_data4, ctx );

		float d0     = ctx.line1.dist;
		float d1     = ctx.line2.dist;
		float d2     = ctx.solid.dist;
		vec4  c0     = ctx.line1.color;
		vec4  c1     = ctx.line2.color;
		vec4  c2     = ctx.solid.color;
		float min01  = min( d0, d1 );
		float max01  = max( d0, d1 );
		vec4  cmin01 = d0 < d1 ? c0 : c1;
		vec4  cmax01 = d0 < d1 ? c1 : c0;
		vec4  cmin   = min01 < d2 ? cmin01 : c2;
		vec4  cmid   = min01 < d2 ? ( max01 < d2 ? cmax01 : c2 ) : cmin01;
		vec4  cmax   = max01 > d2 ? cmax01 : c2;

		if ( min( min01, d2 ) >= 1e4 )
		{
			if ( show_bounds )
			{
				vec4 clip_pos = PROJECTION_MATRIX * vec4( 0, 0, -bounds, 1 );

				ALBEDO = vec3( 0.2 );
				ALPHA  = 0.3;
				DEPTH  = ( clip_pos.z / clip_pos.w );
			}
			else
			{
				discard;
			}
		}
		else
		{
			float a0    = cmin.a;
			float alpha = a0;
			vec3  col   = cmin.rgb * a0;

			if ( max( min01, min( max01, d2 ) ) < 1e4 )
			{
				float a1 = cmid.a * ( 1.0 - alpha );

				col   += cmid.rgb * a1;
				alpha += a1;
			}

			if ( max( max01, d2 ) < 1e4 )
			{
				float a2 = cmax.a * ( 1.0 - alpha );

				col   += cmax.rgb * a2;
				alpha += a2;
			}

			if ( show_bounds )
			{
				vec4 bounds_col = vec4( 0.2, 0.2, 0.2, 0.3 );

				col   = bounds_col.rgb * bounds_col.a + col * ( 1.0 - bounds_col.a );
				alpha = bounds_col.a + alpha * ( 1.0 - bounds_col.a );
			}

			float d        = min( min01, d2 ); if ( show_bounds ) d = min( d, bounds );
			vec4  clip_pos = PROJECTION_MATRIX * vec4( 0, 0, -d, 1 );

			ALBEDO = col / alpha;
			ALPHA  = alpha;
			DEPTH  = ( clip_pos.z / clip_pos.w );
		}
	}
}