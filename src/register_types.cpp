#include <godot_cpp/classes/scene_tree.hpp>
#include "debug_draw.hpp"

using namespace godot;

namespace JW
{
	void debugdraw_setup()
	{
		const auto *scene_tree = Object::cast_to<SceneTree>( Engine::get_singleton()->get_main_loop() );
		CRASH_COND( !scene_tree );

		const auto *root = scene_tree->get_root();
		CRASH_COND( !root );

		JWDebugDraw::initialize();

		RenderingServer::get_singleton()->connect( "frame_pre_draw", callable_mp_static( &JWDebugDraw::on_frame ) );
	}

	void debugdraw_init( const ModuleInitializationLevel p_level )
	{
		if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) return;

		ClassDB::register_class<JWDebugDraw>();

		const Callable def = callable_mp_static( &debugdraw_setup );
		def.call_deferred();
	}

	void debugdraw_uninit( const ModuleInitializationLevel p_level )
	{
		if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) return;

		RenderingServer::get_singleton()->disconnect( "frame_pre_draw", callable_mp_static( &JWDebugDraw::on_frame ) );

		JWDebugDraw::shutdown();
	}
}

extern "C"
{
	GDExtensionBool GDE_EXPORT jwdebugdraw_init( const GDExtensionInterfaceGetProcAddress proc_addr,
	                                             GDExtensionClassLibraryPtr lib, GDExtensionInitialization *init )
	{
		const GDExtensionBinding::InitObject init_obj( proc_addr, lib, init );
		init_obj.register_initializer( JW::debugdraw_init );
		init_obj.register_terminator( JW::debugdraw_uninit );
		init_obj.set_minimum_library_initialization_level( MODULE_INITIALIZATION_LEVEL_SCENE );
		return init_obj.init();
	}
}