#pragma once
#include <Eigen/Dense>

// Subtask 1: Implement the Reflect function in Shading.hpp, so you can find the reflected vector.
/// <summary>
/// Reflect an incoming vector in a normal.
/// </summary>
/// <param name="incoming">Incoming direction unit vector, pointing into surface.</param>
/// <param name="normal">Surface normal</param>
/// <returns>Reflected vector, pointing out from surface point.</returns>
Eigen::Vector3f reflect(const Eigen::Vector3f& incoming, const Eigen::Vector3f& normal)
{
	// replace this with the reflected vector.
	return incoming - 2 * incoming.dot(normal) * normal;
}

// Subtask 2: Implement the Phong specular term in Shading.hpp, to find the intensity of specular reflection
/// <summary>
/// Return the Phong specular term for a given lighting direction, normal, view direction and specular exponent.
/// NOTE: All input vectors must be normalised unit vectors.
/// </summary>
/// <param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
/// <param name="normal">Normal at surface point.</param>
/// <param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
/// <param name="exponent">Specular exponent (higher=shinier)</param>
/// <returns>Specular term (number from 0 to 1)</returns>
float phongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	// Find the reflected direction using the reflect function
	Eigen::Vector3f reflectionDir = reflect(-incomingLightDir, normal);

	// Find dot product between reflected and view directions.
	float dotRefView = reflectionDir.dot(viewDir);

	// Make sure dot product is non-negative (if it's less than 0, set it to 0!)
	if (dotRefView < 0) 
	{
		dotRefView = 0;
	}

	// Finally, raise to specular exponent and return.
	return powf(dotRefView, exponent);
}

// Subtask 5: Implement the Blinn-Phong reflection model in Shading.hpp
/// <summary>
/// Return the Blinn-Phong specular term for a given lighting direction, normal, view direction and specular exponent.
/// NOTE: All input vectors must be normalised unit vectors.
/// </summary>
/// <param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
/// <param name="normal">Normal at surface point.</param>
/// <param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
/// <param name="exponent">Specular exponent (higher=shinier)</param>
/// <returns>Specular term (number from 0 to 1)</returns>
float blinnPhongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	// Find the half-vector (average of view dir and light dir)
	Eigen::Vector3f halfVec = (viewDir + (-incomingLightDir)).normalized();

	// Find dot product of half-vector and normal.
	float halfDotNorm = halfVec.dot(normal);
	
	// Force the dot product to be non-negative (if <0, set to 0)
	if (halfDotNorm < 0) 
	{
		halfDotNorm = 0;
	}

	//Return the dot product raised to the exponent
	return powf(halfDotNorm, exponent);
}

