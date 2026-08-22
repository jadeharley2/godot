/**************************************************************************/
/*  renderer_scene_occlusion_cull.h                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/math/projection.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_server.h"

class RendererSceneOcclusionCull {
protected:
	static RendererSceneOcclusionCull *singleton;

public:
	class HZBuffer {
	protected:
		LocalVector<float> data;
		LocalVector<Size2i> sizes;
		LocalVector<float *> mips;

		RID debug_texture;
		Ref<Image> debug_image;
		PackedByteArray debug_data;
		float debug_tex_range = 0.0f;

		uint64_t occlusion_frame = 0;
		Size2i occlusion_buffer_size;

		_FORCE_INLINE_ bool _is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal) const {
			if (is_empty()) {
				return false;
			}

			Vector3 closest_point = p_cam_position.clamp(Vector3(p_bounds[0], p_bounds[1], p_bounds[2]), Vector3(p_bounds[3], p_bounds[4], p_bounds[5]));

			if (closest_point == p_cam_position) {
				return false;
			}

			Vector3 closest_point_view = p_cam_inv_transform.xform(closest_point);
			if (closest_point_view.z > -p_near) {
				return false;
			}

			// Force distance calculation to use double precision to avoid floating-point overflow for distant objects.
			closest_point = closest_point - p_cam_position;
			float min_depth = Math::sqrt((double)closest_point.x * (double)closest_point.x + (double)closest_point.y * (double)closest_point.y + (double)closest_point.z * (double)closest_point.z);

			Vector2 rect_min = Vector2(FLT_MAX, FLT_MAX);
			Vector2 rect_max = Vector2(FLT_MIN, FLT_MIN);

			//custom
			//Vector3 view_corners[8];

			for (int j = 0; j < 8; j++) {
				// Bitmask to cycle through the corners of the AABB.
				Vector3 corner = Vector3(
						j & 4 ? p_bounds[0] : p_bounds[3],
						j & 2 ? p_bounds[1] : p_bounds[4],
						j & 1 ? p_bounds[2] : p_bounds[5]);
				Vector3 view = p_cam_inv_transform.xform(corner);

				// When using an orthogonal camera, the closest point of an AABB to the camera is guaranteed to be a corner.
				if (p_is_orthogonal) {
					min_depth = MIN(min_depth, -view.z);
				}

				Vector3 projected = p_cam_projection.xform(view);

				if (-view.z < 0.0) {
					rect_min = Vector2(0.0f, 0.0f);
					rect_max = Vector2(1.0f, 1.0f);
					break;
				}

				Vector2 normalized = Vector2(projected.x * 0.5f + 0.5f, projected.y * 0.5f + 0.5f);
				rect_min = rect_min.min(normalized);
				rect_max = rect_max.max(normalized);

				//custom
				//view_corners[j] = Vector3(normalized.x,normalized.y,-view.z);
			}

			rect_max = rect_max.minf(1);
			rect_min = rect_min.maxf(0);

			int mip_count = mips.size();

			Vector2 screen_diagonal = (rect_max - rect_min) * sizes[0];
			float size = MAX(screen_diagonal.x, screen_diagonal.y);
			float l = Math::ceil(Math::log2(size));
			int lod = CLAMP(l, 0, mip_count - 1);

			//custom 
			//return _is_occluded_box( p_bounds, p_cam_position, lod, rect_min, rect_max, view_corners);

			const int max_samples = 512;
			int sample_count = 0;
			bool visible = true;

			for (; lod >= 0; lod--) {
				int w = sizes[lod].x;
				int h = sizes[lod].y;

				int minx = CLAMP(rect_min.x * w - 1, 0, w - 1);
				int maxx = CLAMP(rect_max.x * w + 1, 0, w - 1);

				int miny = CLAMP(rect_min.y * h - 1, 0, h - 1);
				int maxy = CLAMP(rect_max.y * h + 1, 0, h - 1);

				sample_count += (maxx - minx + 1) * (maxy - miny + 1);

				if (sample_count > max_samples) {
					return false;
				}

				visible = false;
				for (int y = miny; y <= maxy; y++) {
					for (int x = minx; x <= maxx; x++) {
						float depth = mips[lod][y * w + x];
						if (depth > min_depth) {
							visible = true;
							break;
						}
					}
					if (visible) {
						break;
					}
				}

				if (!visible) {
					return true;
				}
			}

			return !visible;
		}

		//custom
		_FORCE_INLINE_ bool _is_occluded_box(const real_t p_bounds[6],const Vector3 &p_cam_position, int lod,
			const Vector2 &rect_min,const Vector2 &rect_max, const Vector3 view_corners[8])const{

			int plane_mask=0;
			
			if		(p_cam_position.x < p_bounds[0]) plane_mask|=1;
			else if	(p_cam_position.x > p_bounds[3]) plane_mask|=2;

			if 		(p_cam_position.y < p_bounds[1]) plane_mask|=4;
			else if (p_cam_position.y > p_bounds[4]) plane_mask|=8;

			if 		(p_cam_position.z < p_bounds[2]) plane_mask|=16;
			else if (p_cam_position.z > p_bounds[5]) plane_mask|=32;
			
			const int max_samples = 512;
			int sample_count = 0;
			//bool visible = true;

			for (; lod >= 0; lod--) {
				int w = sizes[lod].x;
				int h = sizes[lod].y;

				int minx = CLAMP(rect_min.x * w - 1, 0, w - 1);
				int maxx = CLAMP(rect_max.x * w + 1, 0, w - 1);

				int miny = CLAMP(rect_min.y * h - 1, 0, h - 1);
				int maxy = CLAMP(rect_max.y * h + 1, 0, h - 1);

				sample_count += (maxx - minx + 1) * (maxy - miny + 1);

				if (sample_count > max_samples) {
					return false;
				}

				Vector3 sz = Vector3(w,h,1);
				Vector2 sz2= Vector2(w,h);
				
				if(plane_mask&1){
					if(	  _draw_triangle(view_corners[6]*sz,view_corners[5]*sz,view_corners[4]*sz,sz2,lod)
						||_draw_triangle(view_corners[6]*sz,view_corners[5]*sz,view_corners[7]*sz,sz2,lod))
						return false;
				}
				if(plane_mask&2){
					if(	  _draw_triangle(view_corners[2]*sz,view_corners[1]*sz,view_corners[0]*sz,sz2,lod)
						||_draw_triangle(view_corners[2]*sz,view_corners[1]*sz,view_corners[3]*sz,sz2,lod))
						return false;
				}

				if(plane_mask&4){
					if(	  _draw_triangle(view_corners[7]*sz,view_corners[2]*sz,view_corners[3]*sz,sz2,lod)
						||_draw_triangle(view_corners[7]*sz,view_corners[2]*sz,view_corners[6]*sz,sz2,lod))
						return false;
				}
				if(plane_mask&8){
					if(	  _draw_triangle(view_corners[0]*sz,view_corners[5]*sz,view_corners[4]*sz,sz2,lod)
						||_draw_triangle(view_corners[0]*sz,view_corners[5]*sz,view_corners[1]*sz,sz2,lod))
						return false;
				}

				if(plane_mask&16){
					if(	  _draw_triangle(view_corners[5]*sz,view_corners[3]*sz,view_corners[1]*sz,sz2,lod)
						||_draw_triangle(view_corners[5]*sz,view_corners[3]*sz,view_corners[7]*sz,sz2,lod))
						return false;
				}
				if(plane_mask&32){
					if(	  _draw_triangle(view_corners[4]*sz,view_corners[2]*sz,view_corners[6]*sz,sz2,lod)
						||_draw_triangle(view_corners[4]*sz,view_corners[2]*sz,view_corners[0]*sz,sz2,lod))
						return false;
				}
			}
			return true;
		}
		_FORCE_INLINE_ bool _draw_triangle(const Vector3 &p1,const Vector3 &p2, const Vector3 &p3, const Vector2 &sz, const int &lod)const{
			Vector2 d1 = Vector2(p1.x,p1.y);
			Vector2 d2 = Vector2(p2.x,p2.y);
			Vector2 d3 = Vector2(p3.x,p3.y);
			
			Vector2 min = d1.min(d2).min(d3).maxf(0.0f);
			Vector2 max = d1.max(d2).max(d3).min(sz);
			int w = sz.x;
			for (int x = min.x; x < max.x; x++) { 
				for (int y = min.y; y < max.y; y++) { 
					Vector3 bar = _get_barycentric_2d(Vector2(x,y),d1,d2,d3);
					if (bar.x>0 && bar.y>0 && bar.z>0){
						float depth = mips[lod][y * w + x];
						float cdepth = p1.z*bar.x+p2.z*bar.y+p3.z*bar.z;
						if(depth>cdepth){
							return true;//is visible
						}
					}
				}
			}
			return false;
		}
		_FORCE_INLINE_ Vector3 _get_barycentric_2d(const Vector2 &p, const Vector2 &a, const Vector2 &b, const Vector2 &c)const{
			Vector2 v0 = b-a;
			Vector2 v1 = c-a;
			Vector2 v2 = p-a;
			float d00 = v0.dot(v0);
			float d01 = v0.dot(v1);
			float d11 = v1.dot(v1);
			float d20 = v2.dot(v0);
			float d21 = v2.dot(v1);
			float denom = d00*d11-d01*d01;
			if(abs(denom)<0.000001f) return Vector3(-1.0f,-1.0f,-1.0f);
			float v = (d11*d20-d01*d21)/denom;
			float w = (d00*d21-d01*d20)/denom;
			return Vector3(1.0f-v-w,v,w);
		}
		//custom end

	public:
		static bool occlusion_jitter_enabled;

		_FORCE_INLINE_ bool is_empty() const {
			return sizes.is_empty();
		}

		virtual void clear();
		virtual void resize(const Size2i &p_size);

		void update_mips();

		// Thin wrapper around _is_occluded(),
		// allowing occlusion timers to delay the disappearance
		// of objects to prevent flickering when using jittering.
		_FORCE_INLINE_ bool is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal, uint64_t &r_occlusion_timeout) const {
			bool occluded = _is_occluded(p_bounds, p_cam_position, p_cam_inv_transform, p_cam_projection, p_near, p_is_orthogonal);

			// Special case, temporal jitter disabled,
			// so we don't use occlusion timers.
			if (!occlusion_jitter_enabled) {
				return occluded;
			}

			if (!occluded) {
//#define DEBUG_RASTER_OCCLUSION_JITTER
#ifdef DEBUG_RASTER_OCCLUSION_JITTER
				r_occlusion_timeout = occlusion_frame + 1;
#else
				r_occlusion_timeout = occlusion_frame + 9;
#endif
			} else if (r_occlusion_timeout) {
				// Regular timeout, allow occlusion culling
				// to proceed as normal after the delay.
				if (occlusion_frame >= r_occlusion_timeout) {
					r_occlusion_timeout = 0;
				}
			}

			return occluded && !r_occlusion_timeout;
		}

		RID get_debug_texture();
		const Size2i &get_occlusion_buffer_size() const { return occlusion_buffer_size; }

		virtual ~HZBuffer() {}
	};

	static RendererSceneOcclusionCull *get_singleton() { return singleton; }

	void _print_warning() {
		WARN_PRINT_ONCE("Occlusion culling is disabled at build-time.");
	}

	virtual bool is_occluder(RID p_rid) { return false; }
	virtual RID occluder_allocate() { return RID(); }
	virtual void occluder_initialize(RID p_occluder) {}
	virtual void free_occluder(RID p_occluder) { _print_warning(); }
	virtual void occluder_set_mesh(RID p_occluder, const PackedVector3Array &p_vertices, const PackedInt32Array &p_indices) { _print_warning(); }

	virtual void add_scenario(RID p_scenario) {}
	virtual void remove_scenario(RID p_scenario) {}
	virtual void scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder, const Transform3D &p_xform, bool p_enabled) { _print_warning(); }
	virtual void scenario_remove_instance(RID p_scenario, RID p_instance) { _print_warning(); }

	virtual void add_buffer(RID p_buffer) { _print_warning(); }
	virtual void remove_buffer(RID p_buffer) { _print_warning(); }
	virtual HZBuffer *buffer_get_ptr(RID p_buffer) {
		return nullptr;
	}
	virtual void buffer_set_scenario(RID p_buffer, RID p_scenario) { _print_warning(); }
	virtual void buffer_set_size(RID p_buffer, const Vector2i &p_size) { _print_warning(); }
	virtual void buffer_update(RID p_buffer, const Transform3D &p_cam_transform, const Projection &p_cam_projection, bool p_cam_orthogonal) {}

	virtual RID buffer_get_debug_texture(RID p_buffer) {
		_print_warning();
		return RID();
	}

	virtual void set_build_quality(RS::ViewportOcclusionCullingBuildQuality p_quality) {}

	RendererSceneOcclusionCull() {
		singleton = this;
	}

	virtual ~RendererSceneOcclusionCull() {
		singleton = nullptr;
	}
};
