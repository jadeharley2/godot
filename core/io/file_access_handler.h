
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
		
        GDVIRTUAL_BIND(_dir_exists, "path");
        GDVIRTUAL_BIND(_make_dir, "path");
    }
	
	GDVIRTUAL1RC(bool, _exists, String) 
	GDVIRTUAL2RC(Variant, _open, String, int32_t) 
	
	GDVIRTUAL1RC(bool, _dir_exists, String) 
	GDVIRTUAL1RC(Error, _make_dir, String) 

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

	bool dir_exists(const String &p_path){
		bool ret = false;
		if (GDVIRTUAL_CALL(_exists, p_path, ret)) {
			return ret;
		}
		return false;
	}
	Error make_dir(const String &p_path){
		Error ret = ERR_SKIP;
		if (GDVIRTUAL_CALL(_make_dir, p_path, ret)) {
			return ret;
		}
		return FAILED;
	}
};
