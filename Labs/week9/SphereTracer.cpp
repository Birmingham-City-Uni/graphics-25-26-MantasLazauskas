// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>
#include <cfloat>
#include <stdexcept>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"

// =========== Week 9 Lab ==============
// Let's make our ultimate Sphere tracer!
//
// Task 1: Add Ray-Sphere intersection
// Task 2: Find the sphere normal
// Task 3: Try raytracing your scene - you should see your first raytraced spheres!
// Task 4: Add mirror reflection
// Task 5: Add a mirror reflective sphere to your scene, and raytrace again!
// Task 6: Add refraction
// Task 7: Add a refractive sphere to your scene, and raytrace again!
// Task 8: Look at the sphere intersection testing code here to prep for task 9.
// Task 9: Add shadow testing, trace and check it works!//
// At the end, you should have a result similar to example.png (assuming you haven't changed
// the scene setup).
// 
// Bonus Task: Add (Blinn)-Phong shading as an extra material type!
//             (no code assistance for this one, but I recommend adding an extra Material type and implementing
//              in the traceRay function).

// For this week - just as a reminder if you prefer you can use the Eigen namespace.
// This saves some typing, but is generally a bit frowned on as it can cause name conflicts.
// The "using" keyword is less frowned upon in source (.cpp) files, but using it in .hpp files 
// is generally a really bad idea (it can lead to hard-to-find bugs very easily).
using namespace Eigen;

// ***** Important Constants *****

// This colour is the default one used if e.g. we don't hit anything,
// or we exceed the maximum bounce limit.
const Vector3f ambientColour(0.1f, 0.1f, 0.1f); 

// Maximum number of bounces (includes both reflection and refraction).
// This limit ensures we don't carry on bouncing forever!
const int maxBounces = 5;

struct Ray {
	Vector3f origin, direction;
};

enum Material {
	DIFFUSE, MIRROR, REFRACTIVE, BLINN_PHONG
};

struct Sphere {
	// Geometric properties
	Vector3f centre;
	float radius;

	// Material properties
	Material material;
	Vector3f colour;

	// IOR for refractive spheres (kept here last so existing initialisers remain valid).
	float ior; // Index of refraction (only used for refractive spheres).

	// Additional Blinn-Phong parameters (optional; value-initialised if not provided).
	Vector3f specular;
	float shininess;
};

struct Camera {
	Vector3f position, direction, up;
	float horzFov;
};


bool raySphereIntersection(const Ray& ray, const Sphere& sphere, Vector3f& intersection, float& t, float minT=0.001f)
{
	// Task 1: Add Ray-Sphere intersection
	// Find the intersection point between the ray and the sphere. 
	// If an intersection exists, set the value of "intersection" and "t", and return true!
	// If no intersection is found, or the value of t is below minT, return false.

	// Solve ||O + tD - C||^2 = r^2
	Vector3f oc = ray.origin - sphere.centre;
	float A = ray.direction.dot(ray.direction);
	float B = 2.f * ray.direction.dot(oc);
	float C = oc.dot(oc) - sphere.radius * sphere.radius;

	float disc = B * B - 4.f * A * C;
	if (disc < 0.f) return false;

	float sqrtD = std::sqrt(disc);
	float t0 = (-B - sqrtD) / (2.f * A);
	float t1 = (-B + sqrtD) / (2.f * A);

	// Choose the smallest t that is > minT
	float chosenT = FLT_MAX;
	if (t0 > minT) chosenT = t0;
	if (t1 > minT && t1 < chosenT) chosenT = t1;

	if (chosenT == FLT_MAX) return false;

	t = chosenT;
	intersection = ray.origin + ray.direction * t;
	return true;
} 

Vector3f getSphereNormal(const Sphere& sphere, const Vector3f& location) {
	// Task 2: Find the sphere normal
	// The normal at a point on a sphere is (location - centre) normalized.
	return (location - sphere.centre).normalized();
}

bool refract(const Vector3f& incident, const Vector3f& norm, float eta, Vector3f& refracted)
{
	// Compute refraction using standard formula.
	// incident: incoming (should be normalized)
	// norm: surface normal (should be normalized and oriented so that dot(incident, norm) <= 0 for entering)
	// eta: n1/n2

	float cosi = -incident.dot(norm);
	float k = 1.f - eta * eta * (1.f - cosi * cosi);
	if (k < 0.f) {
		// Total internal reflection
		return false;
	}
	refracted = (eta * incident) + (eta * cosi - std::sqrt(k)) * norm;
	refracted.normalize();
	return true;
}

Vector3f traceRay(const Ray& ray, const std::vector<Sphere>& spheres, const std::vector<std::unique_ptr<Light>>& lights, int bounce=0)
{
	// First, if we get to too many bounces, we need to exit and do something reasonable 
	// I've chosen to return the ambient default colour.
	if (bounce > maxBounces) return ambientColour;

	// Task 8: Look at the sphere intersection testing code here to prep for task 9.
	// Keep track of the minimum t thus far,
	// where our closest hit location is,
	// and the closest sphere we've hit.
	float minHitT = FLT_MAX;
	Vector3f hitIntersection;
	const Sphere* hitSphere = nullptr;

	// Intersect each sphere, and if the hit is closer and valid,
	// update the closest hit info.
	for (const Sphere& sphere : spheres) {
		float t;
		Vector3f intersection;
		if (raySphereIntersection(ray, sphere, intersection, t)) {
			if (t < minHitT) {
				minHitT = t;
				hitSphere = &sphere;
				hitIntersection = intersection;
			}
		}
	}

	// If we didn't hit anything, exit early and return the default colour.
	if (!hitSphere) {
		return ambientColour;
	}

	// If we hit a diffuse material, do lighting calculations!
	// These are currently pretty much what we did for rasterisation before - do the dot product,
	// and a coefft-wise product with the albedo.
	else if (hitSphere->material == Material::DIFFUSE) {
		Vector3f color = Vector3f::Zero();

		for (const auto& light : lights) {
			if (light->getType() == Light::AMBIENT) {
				// Ambient lighting.
				// No need for a shadow test here!
				color += coeffWiseMultiply(hitSphere->colour, light->getLightIntensity());
			}
			else {
				Vector3f lightDir = light->getDirection(hitIntersection);
				// shadow test
				bool inShadow = false;

				// Task 9: Add shadow testing, trace and check it works!
				// Add shadow testing to cast pixel-perfect shadows.
				// Construct shadow ray from the hit point towards the light.
				Ray shadowRay;
				// lightDir is the incoming direction (from light to surface). For a shadow ray we want from
				// surface towards the light: that's -lightDir.
				shadowRay.direction = -lightDir;
				// Offset origin slightly along the surface normal to avoid self-intersection.
				Vector3f surfNormal = getSphereNormal(*hitSphere, hitIntersection);
				shadowRay.origin = hitIntersection + surfNormal * 0.001f;

				for (const Sphere& s : spheres) {
					// Optional: ignore refractive spheres for simple shadows
					if (s.material == Material::REFRACTIVE) continue;

					float tShadow;
					Vector3f shadowHit;
					if (raySphereIntersection(shadowRay, s, shadowHit, tShadow)) {
						if (light->getType() == Light::DIRECTIONAL) {
							inShadow = true;
							break;
						}
						else {
							// For point/spot lights, only consider blocking if the object is closer than the light.
							float distToLight = (light->getLightLocation() - hitIntersection).norm();
							if (tShadow < distToLight) {
								inShadow = true;
								break;
							}
						}
					}
				}

				// If we're in shadow, this light source doesn't contribute to the colour so continue to the next.
				if (inShadow) continue;

				// Normal diffuse lighting calculations
				float dotProd = -lightDir.dot(getSphereNormal(*hitSphere, hitIntersection));
				dotProd = std::max(dotProd, 0.f);
				Vector3f reflectance = hitSphere->colour * dotProd;
				color += coeffWiseMultiply(reflectance, light->getIntensityAt(hitIntersection));
			}
		}
		return color;
	}

	// Blinn-Phong material: ambient + diffuse + specular
	else if (hitSphere->material == Material::BLINN_PHONG) {
		Vector3f color = Vector3f::Zero();

		for (const auto& light : lights) {
			if (light->getType() == Light::AMBIENT) {
				color += coeffWiseMultiply(hitSphere->colour, light->getLightIntensity());
			}
			else {
				Vector3f lightDir = light->getDirection(hitIntersection);
				// shadow test (same as diffuse)
				bool inShadow = false;
				Ray shadowRay;
				shadowRay.direction = -lightDir;
				Vector3f surfNormal = getSphereNormal(*hitSphere, hitIntersection);
				shadowRay.origin = hitIntersection + surfNormal * 0.001f;

				for (const Sphere& s : spheres) {
					if (s.material == Material::REFRACTIVE) continue;
					float tShadow;
					Vector3f shadowHit;
					if (raySphereIntersection(shadowRay, s, shadowHit, tShadow)) {
						if (light->getType() == Light::DIRECTIONAL) {
							inShadow = true;
							break;
						} else {
							float distToLight = (light->getLightLocation() - hitIntersection).norm();
							if (tShadow < distToLight) {
								inShadow = true;
								break;
							}
						}
					}
				}
				if (inShadow) continue;

				Vector3f N = surfNormal;
				Vector3f L = -lightDir;
				L.normalize();
				float diff = std::max(N.dot(L), 0.f);
				Vector3f diffuse = hitSphere->colour * diff;

				Vector3f V = -ray.direction.normalized();
				Vector3f H = (L + V).normalized();
				float specFactor = std::max(N.dot(H), 0.f);
				float spec = powf(specFactor, hitSphere->shininess > 0.f ? hitSphere->shininess : 32.f);
				Vector3f specular = hitSphere->specular * spec;

				Vector3f combined = diffuse + specular;
				color += coeffWiseMultiply(combined, light->getIntensityAt(hitIntersection));
			}
		}
		return color;
	}
	else if (hitSphere->material == Material::MIRROR) {
		// Task 4: Add mirror reflection
		// Find the reflected ray, and call traceRay again recursively
		Vector3f normal = getSphereNormal(*hitSphere, hitIntersection);
		Vector3f dir = ray.direction.normalized();
		Vector3f reflectedDir = dir - 2.f * dir.dot(normal) * normal;
		reflectedDir.normalize();

		Ray reflectedRay;
		// Offset origin slightly along normal to avoid self intersection.
		reflectedRay.origin = hitIntersection + normal * 0.001f;
		reflectedRay.direction = reflectedDir;

		Vector3f reflectedColour = traceRay(reflectedRay, spheres, lights, bounce + 1);
		// Optional: coloured mirrors - coeff-wise multiply by sphere colour.
		return coeffWiseMultiply(reflectedColour, hitSphere->colour);
	}
	else if (hitSphere->material == Material::REFRACTIVE) {
		// I've handled a few fiddly bits of the refraction code for you here:
		float eta; // This is n1/n2, the ratio of IORs.
		Vector3f normal = getSphereNormal(*hitSphere, hitIntersection);

		// Check if we're going into a sphere, or coming out of a sphere.
		bool enteringSphere = ray.direction.dot(normal) < 0;

		// The below code to find eta assumes all spheres are surrounded by air.
		// If you plan to set up scenes with spheres that intersect or surround each other, this will need revising!

		// If we're entering the sphere, we assume we're coming from free air so n1=1, n2=the IOR of the sphere.
		if (enteringSphere) eta = 1.f / hitSphere->ior;

		// If we're leaving the sphere, we assume we're going from the sphere into air, so n1=IOR, n2=1
		else eta = hitSphere->ior;

		// Finally, if we're leaving the sphere the normal will need flipping so our refract() function works
		// correctly.
		if (!enteringSphere) normal = -normal;

		// Task 6: Add refraction
		Vector3f refractedDir;
		if (refract(ray.direction.normalized(), normal, eta, refractedDir)) {
			// Successfully refracted
			Ray refractedRay;
			// Offset origin slightly along refracted direction to avoid self intersection
			refractedRay.origin = hitIntersection + refractedDir * 0.001f;
			refractedRay.direction = refractedDir;

			Vector3f refractedColour = traceRay(refractedRay, spheres, lights, bounce + 1);
			// Optional: coloured glass
			return coeffWiseMultiply(refractedColour, hitSphere->colour);
		}
		else {
			// Total internal reflection - treat as mirror
			Vector3f normalForReflect = normal; // already oriented correctly
			Vector3f dir = ray.direction.normalized();
			Vector3f reflectedDir = dir - 2.f * dir.dot(normalForReflect) * normalForReflect;
			reflectedDir.normalize();

			Ray reflectedRay;
			reflectedRay.origin = hitIntersection + normalForReflect * 0.001f;
			reflectedRay.direction = reflectedDir;

			return traceRay(reflectedRay, spheres, lights, bounce + 1);
		}
	}
	// Should not reach here, but return ambient as fallback.
	return ambientColour;
}

int main()
{
	std::string outputFilename = "output.png";

	const int width = 512, height = 512;
	const int nChannels = 4;

	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
		}
	}

	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Vector3f(0.1f, 0.1f, 0.1f)));
	lights.emplace_back(new DirectionalLight(Vector3f(0.4f, 0.4f, 0.4f), Vector3f(1.f, -1.f, 0.0f)));

	// This code sets up a scene with a bunch of spheres.
	// I've initially set up the scene with diffuse spheres only. Once you've implemented 
	// MIRROR and REFRACTIVE spheres, you can uncomment these or add your own!
	std::vector<Sphere> spheres;
	spheres.push_back({ Vector3f(2.f, 0.f, 4.f), 1.f, Material::DIFFUSE, Vector3f(0.f, 0.8f, 0.8f) });
	spheres.push_back({ Vector3f(-2.f, 0.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.8f, 0.f, 0.8f) });
	spheres.push_back({ Vector3f(0.f, 2.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.8f, 0.8f, 0.f) });
	spheres.push_back({ Vector3f(0.f, -2.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.2f, 0.2f, 0.8f) });
	spheres.push_back({ Vector3f(0.f, 1.f, 6.f), 0.3f, Material::DIFFUSE, Vector3f(0.8f, 0.8f, 0.f) });
	// Task 5: Add a mirror reflective sphere to your scene, and raytrace again!
	spheres.push_back({ Vector3f(2.f, 2.f, 4.f), 0.5f, Material::MIRROR, Vector3f(0.9f, 0.9f, 0.9f) });
	// Task 7: Add a refractive sphere to your scene, and raytrace again!
	spheres.push_back({ Vector3f(0.f, 0.f, 3.f), 0.5f, Material::REFRACTIVE, Vector3f(0.9f, 0.8f, 0.8f), 1.4f });
	// Bonus Task: Add (Blinn)-Phong shading as an extra material type!
	spheres.push_back({ Vector3f(-1.5f, 1.5f, 4.f), 0.5f, Material::BLINN_PHONG, Vector3f(0.4f, 0.6f, 0.2f), 0.f, Vector3f(0.9f, 0.9f, 0.9f), 64.f });
	
	Camera camera{
		Vector3f(0.f, 0.f, 0.f), // position
		Vector3f(0.f, 0.f, 1.f), // direction
		Vector3f(0.f, 1.f, 0.f), // up
		M_PI_2 // horzFov
	};

	float vertFov = (camera.horzFov * height) / width;
	float minX = -atanf(camera.horzFov * 0.5f);
	float xStep = -2.f * minX / width;
	float minY = -atanf(vertFov * 0.5f);
	float yStep = -2.f * minY / width;

	Vector3f across = -camera.direction.cross(camera.up).normalized();

	for (int x = 0; x < width; ++x)
		for (int y = 0; y < height; ++y) {
			Ray ray;
			ray.origin = camera.position;
			ray.direction = camera.direction + minX * across + minY * camera.up;
			ray.direction += across * x * xStep;
			ray.direction += camera.up * y * yStep;
			ray.direction.normalize();

			//ray.origin = Vector3f::Zero();
			//ray.direction = Vector3f(0.f, 0.f, 1.f);

			Vector3f color = traceRay(ray, spheres, lights);

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;
			c.a = 255;
			setPixel(imageBuffer, x, height-y-1, width, height, c);
		}
	

	// Save the image to png.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
