/**************************************************************************/
/*  a_compute.cpp                                                         */
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

#include "a_compute.h"
#include <servers/rendering_server.h>

void ACompute::_bind_methods() {
	ClassDB::bind_method(D_METHOD("dispatch", "kernel_index", "x_groups", "y_groups", "z_groups"), &ACompute::dispatch);
	ClassDB::bind_method(D_METHOD("initialize", "_shader_name"), &ACompute::initialize);
}

void ACompute::initialize(String _shader_name) {
	rd = RenderingServer::get_singleton()->get_rendering_device();

	uniform_set_cache = Vector<RD::Uniform>();

	shader_name = _shader_name;

	shader_id = RID();

	for (RID kernel : TypedArray<RID>()) {
		kernels.push_back(rd->compute_pipeline_create(kernel));
	}
}

void ACompute::dispatch(int kernel_index, int x_groups, int y_groups, int z_groups) {
	RID global_shader_id = RID();

	// Recreate kernel pipelines if shader was recompiled
	if (shader_id != global_shader_id) {
		shader_id = global_shader_id;
		kernels.clear();
		for (RID kernel : Array()) {
			kernels.push_back(rd->compute_pipeline_create(kernel));
		}
	}

	// Reallocate GPU memory if uniforms need updating
	if (refresh_uniforms) {
		if (uniform_set_gpu_id.is_valid()) {
			rd->free(uniform_set_gpu_id);
		}
		uniform_set_gpu_id = rd->uniform_set_create(uniform_set_cache, global_shader_id, 0);
		refresh_uniforms = true;

		RenderingDevice::ComputeListID compute_list = rd->compute_list_begin();
		rd->compute_list_bind_compute_pipeline(compute_list, kernels[kernel_index]);
		rd->compute_list_bind_uniform_set(compute_list, uniform_set_gpu_id, 0);
		rd->compute_list_set_push_constant(compute_list, &push_constant, push_constant.size());
		rd->compute_list_dispatch(compute_list, x_groups, y_groups, z_groups);
		rd->compute_list_end();
	}
}

ACompute::~ACompute() {
	for (RID kernel : kernels) {
		rd->free(kernel);
	}

	for (RID binding : uniform_buffer_id_cache.keys()) {
		rd->free(binding);
	}

	if (uniform_set_gpu_id.is_valid()) {
		rd->free(uniform_set_gpu_id);
	}
}
