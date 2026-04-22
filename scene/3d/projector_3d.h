#pragma once

#include "scene/3d/node_3d.h"
#include "scene/3d/camera_3d.h"

class Projector3D : public Node3D {
	GDCLASS(Projector3D, Node3D);
 
    NodePath source_path;

	RID projector;

	uint32_t layers = 1;
	uint32_t types = RS::INSTANCE_GEOMETRY_MASK | (1<<RS::INSTANCE_LIGHT) | (1<<RS::INSTANCE_DECAL) | (1<<RS::INSTANCE_FOG_VOLUME);


protected:
    void _setup();
    void _cleanup();
	void _update_visibility();

	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_source_camera(NodePath camera);
	NodePath get_source_camera() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

    void set_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_layer_mask_value(int p_layer_number) const;

	void set_type_mask(uint32_t p_mask);
	uint32_t get_type_mask() const;
    

	Projector3D();
};
