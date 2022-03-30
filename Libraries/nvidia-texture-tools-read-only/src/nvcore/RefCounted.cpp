// This code is in the public domain -- castanyo@yahoo.es

#include "RefCounted.h"

using namespace nv;

int nv::RefCounted::s_total_ref_count = 0;
int nv::RefCounted::s_total_obj_count = 0;

