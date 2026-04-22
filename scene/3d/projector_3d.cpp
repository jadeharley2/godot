
#include "projector_3d.h"

void Projector3D::_setup() {
    if(projector.is_valid()) return; 
    Camera3D *source = Object::cast_to<Camera3D>(get_node_or_null(source_path));
    if(!source) return; 
    RID source_scenario = source->get_world_3d()->get_scenario();
    if(source_scenario.is_null()) return;
    RID target_scenario = get_world_3d()->get_scenario();
    if(target_scenario.is_null()) return;

    projector = RenderingServer::get_singleton()->projector_create();
    RenderingServer::get_singleton()->projector_set_source_scenario(projector,source_scenario);
    RenderingServer::get_singleton()->projector_set_target_scenario(projector,target_scenario);
    RenderingServer::get_singleton()->projector_set_layer_mask(projector,layers);
    RenderingServer::get_singleton()->projector_set_type_filter(projector,types);
    set_process(true);
}
void Projector3D::_cleanup() {
    set_process(false);
    RenderingServer::get_singleton()->free_rid(projector);
    projector = RID();
}

void Projector3D::_update_visibility() {
	if (!is_inside_tree()) {
		return;
	}
	bool visible = is_visible_in_tree();
    if(visible && projector.is_null()){
        _setup();
    }
    else if (!visible && projector.is_valid())
    {  
        _cleanup();
    } 
}
void Projector3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_visibility();
		} break;
		case NOTIFICATION_EXIT_TREE: {
            _cleanup();
            request_ready();
		} break;
		case NOTIFICATION_READY: {
			_update_visibility();
		} break;
		case NOTIFICATION_PROCESS: {
			if(projector.is_valid()){
                RenderingServer::get_singleton()->projector_update(projector,get_global_transform());
            }
		} break;
    }
}



void Projector3D::set_source_camera(NodePath camera){
    if(camera!=source_path){
        source_path = camera; 
        _cleanup();
        _update_visibility();
    }
}
NodePath Projector3D::get_source_camera() const{
    return source_path;
}

void Projector3D::set_layer_mask(uint32_t p_mask) {
	layers = p_mask;
    if(projector.is_valid()){
	    RenderingServer::get_singleton()->projector_set_layer_mask(projector, p_mask);
    }
}

uint32_t Projector3D::get_layer_mask() const {
	return layers;
}

void Projector3D::set_layer_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_layer_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_layer_mask(mask);
}

bool Projector3D::get_layer_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return layers & (1 << (p_layer_number - 1));
}


void Projector3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_source_camera", "camera"), &Projector3D::set_source_camera);
	ClassDB::bind_method(D_METHOD("get_source_camera"), &Projector3D::get_source_camera);
	ClassDB::bind_method(D_METHOD("set_layer_mask", "mask"), &Projector3D::set_layer_mask);
	ClassDB::bind_method(D_METHOD("get_layer_mask"), &Projector3D::get_layer_mask);
	ClassDB::bind_method(D_METHOD("set_type_mask", "mask"), &Projector3D::set_type_mask);
	ClassDB::bind_method(D_METHOD("get_type_mask"), &Projector3D::get_type_mask);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "source_camera", PROPERTY_HINT_NODE_TYPE, "Camera3D"), "set_source_camera", "get_source_camera"); 
	ADD_PROPERTY(PropertyInfo(Variant::INT, "type_mask", PROPERTY_HINT_FLAGS,
        "Mesh:2,Multimesh:4,Particles:8,Particle Collision:16,Light:32,Reflection Probe:64,Decal:128,Voxel GI:256, Lightmap:512, Occluder:1024, Visibility Notifier:2048,Fog Volume:4096"), "set_type_mask", "get_type_mask");
    
	ADD_PROPERTY(PropertyInfo(Variant::INT, "layer_mask", PROPERTY_HINT_LAYERS_3D_RENDER), "set_layer_mask", "get_layer_mask");
}
/*
    type_mask reference:
		INSTANCE_NONE,
		INSTANCE_MESH,
		INSTANCE_MULTIMESH,
		INSTANCE_PARTICLES,
		INSTANCE_PARTICLES_COLLISION,
		INSTANCE_LIGHT,
		INSTANCE_REFLECTION_PROBE,
		INSTANCE_DECAL,
		INSTANCE_VOXEL_GI,
		INSTANCE_LIGHTMAP,
		INSTANCE_OCCLUDER,
		INSTANCE_VISIBLITY_NOTIFIER, // TODO: Fix typo in "VISIBILITY" (in 5.0).
		INSTANCE_FOG_VOLUME,
*/

void Projector3D::set_type_mask(uint32_t p_mask) {
	types = p_mask;
    if(projector.is_valid()){
	    RenderingServer::get_singleton()->projector_set_type_filter(projector, p_mask);
    }
}

uint32_t Projector3D::get_type_mask() const {
	return types;
}



Projector3D::Projector3D() {
}
