// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Stream.h>

#include <nvmath/Vector.h>
#include <nvmath/Matrix.h>
#include <nvmath/Quaternion.h>
#include <nvmath/Basis.h>
#include <nvmath/Box.h>
#include <nvmath/Plane.h>

#include <nvmath/TypeSerialization.h>

using namespace nv;

Stream & nv::operator<< (Stream & s, Vector2 & v)
{
	float x = v.x();
	float y = v.y();

	s << x << y;

	if (s.isLoading())
	{
		v.set(x, y);
	}

	return s;
}

Stream & nv::operator<< (Stream & s, Vector3 & v)
{
	float x = v.x();
	float y = v.y();
	float z = v.z();

	s << x << y << z;

	if (s.isLoading())
	{
		v.set(x, y, z);
	}

	return s;
}

Stream & nv::operator<< (Stream & s, Vector4 & v)
{
	float x = v.x();
	float y = v.y();
	float z = v.z();
	float w = v.w();

	s << x << y << z << w;

	if (s.isLoading())
	{
		v.set(x, y, z, w);
	}

	return s;
}

Stream & nv::operator<< (Stream & s, Matrix & m)
{
	return s;
}

Stream & nv::operator<< (Stream & s, Quaternion & q)
{
	return s << q.asVector();
}

Stream & nv::operator<< (Stream & s, Basis & basis)
{
	return s << basis.tangent << basis.bitangent << basis.normal;
}

Stream & nv::operator<< (Stream & s, Box & box)
{
	return s << box.m_mins << box.m_maxs;
}

Stream & nv::operator<< (Stream & s, Plane & plane)
{
	return s << plane.asVector();
}
