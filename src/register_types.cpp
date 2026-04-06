#include <godot_cpp/classes/scene_tree.hpp>
#include "debug_draw.hpp"

using namespace godot;

namespace JW
{
	void debugdraw_init( const ModuleInitializationLevel p_level )
	{
		if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) return;

		ClassDB::register_class<JWDebugDraw>();
	}

	void debugdraw_startup()
	{
		RenderingServer::get_singleton()->connect( "frame_pre_draw", callable_mp_static( &JWDebugDraw::on_frame ) );
	}

	void debugdraw_uninit( const ModuleInitializationLevel p_level )
	{
		if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) return;

		RenderingServer::get_singleton()->disconnect( "frame_pre_draw", callable_mp_static( &JWDebugDraw::on_frame ) );

		if ( JWDebugDraw::is_initialized() ) JWDebugDraw::shutdown();
	}
}

extern "C"
{
	GDExtensionBool GDE_EXPORT jwdebugdraw_init( const GDExtensionInterfaceGetProcAddress proc_addr,
	                                             GDExtensionClassLibraryPtr lib, GDExtensionInitialization *init )
	{
		const GDExtensionBinding::InitObject init_obj( proc_addr, lib, init );
		init_obj.register_initializer( JW::debugdraw_init );
		init_obj.register_startup_callback( JW::debugdraw_startup );
		init_obj.register_terminator( JW::debugdraw_uninit );
		init_obj.set_minimum_library_initialization_level( MODULE_INITIALIZATION_LEVEL_SCENE );
		return init_obj.init();
	}
}
