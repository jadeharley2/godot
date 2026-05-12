#pragma once

#include "scene/3d/node_3d.h"
#include "scene/3d/camera_3d.h"

class Projector3D : public Node3D {
	GDCLASS(Projector3D, Node3D);
 
	bool project_render = true, project_collision;

    NodePath source_path;

	RID projector;

	uint32_t layers = 1;
	uint32_t types = RS::INSTANCE_GEOMETRY_MASK | (1<<RS::INSTANCE_LIGHT) | (1<<RS::INSTANCE_DECAL) | (1<<RS::INSTANCE_FOG_VOLUME);

	RID physics_projector;
	uint32_t physics_layers = ~0;


protected:
    void _setup_render();
    void _setup_collision();
    void _cleanup_render();
    void _cleanup_collision();
	void _update_visibility();

	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_project_render(bool enable);
	bool get_project_render() const;
	void set_project_collision(bool enable);
	bool get_project_collision() const;

	void set_source_camera(NodePath camera);
	NodePath get_source_camera() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

    void set_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_layer_mask_value(int p_layer_number) const;

	void set_type_mask(uint32_t p_mask);
	uint32_t get_type_mask() const;


	void set_collision_layer_mask(uint32_t p_mask);
	uint32_t get_collision_layer_mask() const;

    void set_collision_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_collision_layer_mask_value(int p_layer_number) const;

    

	Projector3D();
};
