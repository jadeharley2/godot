
#pragma once

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/io/compression.h"
#include "core/math/math_defs.h"
#include "core/os/memory.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"
#include "core/variant/type_info.h"

#include "core/object/gdvirtual.gen.h"

class FileAccessHandler : public RefCounted {
	GDCLASS(FileAccessHandler, RefCounted);

protected:
	static void _bind_methods() { 
        GDVIRTUAL_BIND(_exists, "path");
        GDVIRTUAL_BIND(_open, "path", "mode_flags");
    }
	
	GDVIRTUAL1RC(bool, _exists, String) 
	GDVIRTUAL2RC(Variant, _open, String, int32_t) 

public:
	
	bool exists(const String &p_path){
		bool ret = false;
		if (GDVIRTUAL_CALL(_exists, p_path, ret)) {
			return ret;
		}
		return false;
	}
	Variant open(const String &p_path, int32_t p_mode_flags){
		Variant ret;
		if (GDVIRTUAL_CALL(_open, p_path, p_mode_flags, ret)) {
			return ret;
		}
		return ret;
	}
};
