#pragma once

#include <cmath>

#include <glm/glm.hpp>

struct Camera
{
	float fovY;
	float aspectRatio;
	float zn;
	float zf;

	Camera(float fovY, float aspectRatio, float zn, float zf)
	{
		this->fovY = fovY;
		this->aspectRatio = aspectRatio;
		this->zn = zn;
		this->zf = zf;
	}

	glm::mat4 getProjection()
	{
		float yScale = std::tan(1.0f / (glm::radians(fovY) * 0.5f));
		float xScale = yScale / aspectRatio;

		return glm::mat4(xScale, 0, 0, 0,
			0, yScale, 0, 0,
			0, 0, zf / (zn - zf), -1,
			0, 0, zn * zf / (zn - zf), 0);
	}

};

