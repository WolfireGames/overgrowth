// This code is in the public domain -- icastano@gmail.com

#include "Fitting.h"

#include <nvcore/Algorithms.h> // max
#include <nvcore/Containers.h> // swap
#include <float.h> // FLT_MAX

using namespace nv;


Vector3 nv::ComputeCentroid(int n, const Vector3 * points, const float * weights, Vector3::Arg metric)
{
	Vector3 centroid(zero);
	float total = 0.0f;

	for (int i = 0; i < n; i++)
	{
		total += weights[i];
		centroid += weights[i]*points[i];
	}
	centroid /= total;

	return centroid;
}


void nv::ComputeCovariance(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, float * covariance)
{
	// compute the centroid
	Vector3 centroid = ComputeCentroid(n, points, weights, metric);

	// compute covariance matrix
	for (int i = 0; i < 6; i++)
	{
		covariance[i] = 0.0f;
	}

	for (int i = 0; i < n; i++)
	{
		Vector3 a = (points[i] - centroid) * metric;
		Vector3 b = weights[i]*a;
		
		covariance[0] += a.x()*b.x();
		covariance[1] += a.x()*b.y();
		covariance[2] += a.x()*b.z();
		covariance[3] += a.y()*b.y();
		covariance[4] += a.y()*b.z();
		covariance[5] += a.z()*b.z();
	}
}

Vector3 nv::ComputePrincipalComponent(int n, const Vector3 * points, const float * weights, Vector3::Arg metric)
{
	float matrix[6];
	ComputeCovariance(n, points, weights, metric, matrix);

	if (matrix[0] == 0 || matrix[3] == 0 || matrix[5] == 0)
	{
		return Vector3(zero);
	}
	
	const int NUM = 8;

	Vector3 v(1, 1, 1);
	for (int i = 0; i < NUM; i++)
	{
		float x = v.x() * matrix[0] + v.y() * matrix[1] + v.z() * matrix[2];
		float y = v.x() * matrix[1] + v.y() * matrix[3] + v.z() * matrix[4];
		float z = v.x() * matrix[2] + v.y() * matrix[4] + v.z() * matrix[5];
		
		float norm = max(max(x, y), z);
	
		v = Vector3(x, y, z) / norm;
	}

	return v;	
}



int nv::Compute4Means(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, Vector3 * cluster)
{
	Vector3 centroid = ComputeCentroid(n, points, weights, metric);
	
	// Compute principal component.
	Vector3 principal = ComputePrincipalComponent(n, points, weights, metric);
	
	// Pick initial solution.
	int mini, maxi;
	mini = maxi = 0;
	
	float mindps, maxdps;
	mindps = maxdps = dot(points[0] - centroid, principal);
	
	for (int i = 1; i < n; ++i)
	{
		float dps = dot(points[i] - centroid, principal);
		
		if (dps < mindps) {
			mindps = dps;
			mini = i;
		}
		else {
			maxdps = dps;
			maxi = i;
		}
	}

	cluster[0] = centroid + mindps * principal;
	cluster[1] = centroid + maxdps * principal;
	cluster[2] = (2 * cluster[0] + cluster[1]) / 3;
	cluster[3] = (2 * cluster[1] + cluster[0]) / 3;

	// Now we have to iteratively refine the clusters.
	while (true)
	{
		Vector3 newCluster[4] = { Vector3(zero), Vector3(zero), Vector3(zero), Vector3(zero) };
		float total[4] = {0, 0, 0, 0};
		
		for (int i = 0; i < n; ++i)
		{
			// Find nearest cluster.
			int nearest = 0;
			float mindist = FLT_MAX;
			for (int j = 0; j < 4; j++)
			{
				float dist = length_squared((cluster[j] - points[i]) * metric);
				if (dist < mindist)
				{
					mindist = dist;
					nearest = j;
				}
			}
			
			newCluster[nearest] += weights[i] * points[i];
			total[nearest] += weights[i];
		}

		for (int j = 0; j < 4; j++)
		{
            if (total[j] != 0)
			    newCluster[j] /= total[j];
		}

		if (equal(cluster[0], newCluster[0]) && equal(cluster[1], newCluster[1]) && 
			equal(cluster[2], newCluster[2]) && equal(cluster[3], newCluster[3]))
		{
			return (total[0] != 0) + (total[1] != 0) + (total[2] != 0) + (total[3] != 0);
		}

		cluster[0] = newCluster[0];
		cluster[1] = newCluster[1];
		cluster[2] = newCluster[2];
		cluster[3] = newCluster[3];

		// Sort clusters by weight.
		for (int i = 0; i < 4; i++)
		{
			for (int j = i; j > 0 && total[j] > total[j - 1]; j--)
			{
				swap( total[j], total[j - 1] );
				swap( cluster[j], cluster[j - 1] );
			}
		}
	}
}

