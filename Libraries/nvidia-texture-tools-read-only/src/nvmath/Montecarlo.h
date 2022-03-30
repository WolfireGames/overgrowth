// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_MONTECARLO_H
#define NV_MATH_MONTECARLO_H

#include <nvmath/Vector.h>
#include <nvmath/Random.h>

namespace nv
{

/// A random sample distribution.
class SampleDistribution
{
public:
	
	// Sampling method.
	enum Method {
		Method_Random,
		Method_Stratified,
		Method_NRook
	};

	// Distribution functions.
	enum Distribution {
		Distribution_UniformSphere,
		Distribution_UniformHemisphere,
		Distribution_CosineHemisphere
	};
	
	/// Constructor.
	SampleDistribution(uint num)
	{
		m_sampleArray.resize(num);
	}

	uint count() const { return m_sampleArray.count(); }
	
	void redistribute(Method method=Method_NRook, Distribution dist=Distribution_CosineHemisphere);
	
	/// Get parametric coordinates of the sample.
	Vector2 sample(int i) const { return m_sampleArray[i].uv; }
	
	/// Get sample direction.
	Vector3 sampleDir(int i) const { return m_sampleArray[i].dir; }

	/// Get number of samples.
	uint sampleCount() const { return m_sampleArray.count(); }
	
private:
	
	void redistributeRandom(const Distribution dist);
	void redistributeStratified(const Distribution dist);
	void multiStageNRooks(const int size, int* cells);
	void redistributeNRook(const Distribution dist);
	
	
	/// A sample of the random distribution.
	struct Sample
	{
		/// Set sample given the 3d coordinates.
		void setDir(float x, float y, float z) {
			dir.set(x, y, z);
			uv.set(acosf(z), atan2f(y, x));
		}
		
		/// Set sample given the 2d parametric coordinates.
		void setUV(float u, float v) {
			uv.set(u, v);
			dir.set(sinf(u) * cosf(v), sinf(u) * sinf(v), cosf(u));
		}
		
		Vector2 uv;
		Vector3 dir;
	};

	inline void setSample(uint i, Distribution dist, float x, float y)
	{
		// Map uniform distribution in the square to desired domain.
		if( dist == Distribution_UniformSphere ) {
			m_sampleArray[i].setUV(acosf(1 - 2 * x), 2 * PI * y);
		}
		else if( dist == Distribution_UniformHemisphere ) {
			m_sampleArray[i].setUV(acosf(x), 2 * PI * y);
		}
		else {
			nvDebugCheck(dist == Distribution_CosineHemisphere);
			m_sampleArray[i].setUV(acosf(sqrtf(x)), 2 * PI * y);
		}
	}

	
	/// Random seed.
	MTRand m_rand;
	
	/// Samples.
	Array<Sample> m_sampleArray;
	
};

} // nv namespace

#endif // NV_MATH_MONTECARLO_H
