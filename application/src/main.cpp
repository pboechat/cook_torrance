#define GLEW_STATIC


#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <memory>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <SOIL.h>
#include <AntTweakBar.h>

#include "objloader.hpp"
#include "GLUtils.h"
#include "Mesh.h"
#include "Shader.h"
#include "Navigator.h"
#include "Camera.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

const std::string SHADERS_DIR("shaders/");
const std::string MEDIA_DIR("media/");

//////////////////////////////////////////////////////////////////////////
struct Texture
{
	char filename[256];
	unsigned unit;

	Texture() = default;

	Texture(const std::string& filename, unsigned unit) : unit(unit)
	{
		auto count = std::min((size_t)255, filename.size());
		strncpy_s(this->filename, filename.c_str(), count);
		this->filename[count + 1] = (char)'/0';
	}

	Texture& operator=(const Texture& other)
	{
		strncpy_s(filename, other.filename, 256);
		unit = other.unit;
		return *this;
	}

};

//////////////////////////////////////////////////////////////////////////
struct Uniform
{
	GLuint program;
	std::string name;
	GLint type;
	GLint location;

	Uniform(GLuint program, const std::string& name, GLint type, GLint location) : program(program), name(name), type(type), location(location), value(std::unique_ptr<void, decltype(free)*>{nullptr, free}), hasTexture(false) { fetchValue(); }
	Uniform(Uniform& other) : program(other.program), name(other.name), type(other.type), location(other.location), texture(other.texture), hasTexture(other.hasTexture), value(std::move(other.value)) {}
	virtual ~Uniform()
	{
		if (hasTexture)
			glDeleteTextures(1, &texture);
	}

	void setValue(const Texture& arg0, bool forceUseProgram = false)
	{
		storeValue(arg0);
		if (forceUseProgram)
			glUseProgram(program);
		int width, height;
		auto image = SOIL_load_image((MEDIA_DIR + arg0.filename).c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if (hasTexture)
			glDeleteTextures(1, &texture);
		glGenTextures(1, &texture);
		hasTexture = true;
		glActiveTexture(GL_TEXTURE0 + arg0.unit);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		SOIL_free_image_data(image);
		glUniform1i(location, arg0.unit);
	}

	void setValue(float arg0, bool forceUseProgram = false)
	{
		storeValue(arg0);
		if (forceUseProgram)
			glUseProgram(program);
		glUniform1f(location, arg0);
	}

	void setValue(const glm::vec2& arg0, bool forceUseProgram = false)
	{
		storeValue(arg0);
		if (forceUseProgram)
			glUseProgram(program);
		glUniform2fv(location, 1, glm::value_ptr(arg0));
	}

	void setValue(const glm::vec3& value_, bool forceUseProgram = false)
	{
		storeValue(value_);
		if (forceUseProgram)
			glUseProgram(program);
		glUniform3fv(location, 1, glm::value_ptr(value_));
	}

	void setValue(const glm::vec4& arg0, bool forceUseProgram = false)
	{
		storeValue(arg0);
		if (forceUseProgram)
			glUseProgram(program);
		glUniform4fv(location, 1, glm::value_ptr(arg0));
	}

	void setValue(const glm::mat4& arg0, bool forceUseProgram = false)
	{
		storeValue(arg0);
		if (forceUseProgram)
			glUseProgram(program);
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(arg0));
	}

	template <typename T>
	T* getValuePtr()
	{
		return reinterpret_cast<T*>(value.get());
	}

private:
	GLuint texture;
	bool hasTexture;
	std::unique_ptr<void, decltype(free)*> value;

	void fetchValue()
	{
		switch (type)
		{
		case GL_BOOL:
		{
			GLint arg0;
			glGetUniformiv(program, location, &arg0);
			storeValue<bool>(arg0 != 0);
			break;
		}
		case GL_FLOAT:
		{
			GLfloat arg0;
			glGetUniformfv(program, location, &arg0);
			storeValue<float>(arg0);
			break;
		}
		case GL_FLOAT_VEC2:
		{
			GLfloat arg0[2];
			glGetUniformfv(program, location, arg0);
			storeValue<glm::vec2>(glm::vec2(arg0[0], arg0[1]));
			break;
		}
		case GL_FLOAT_VEC3:
		{
			GLfloat arg0[3];
			glGetUniformfv(program, location, arg0);
			storeValue<glm::vec3>(glm::vec3(arg0[0], arg0[1], arg0[2]));
			break;
		}
		case GL_FLOAT_VEC4:
		{
			GLfloat arg0[4];
			glGetUniformfv(program, location, arg0);
			storeValue<glm::vec4>(glm::vec4(arg0[0], arg0[1], arg0[2], arg0[3]));
			break;
		}
		}
	}

	template <typename T>
	void storeValue(T arg0)
	{
		value = std::unique_ptr<void, decltype(free)*>{
			malloc(sizeof(T)),
			free
		};
		memcpy(value.get(), &arg0, sizeof(T));
	}

};

//////////////////////////////////////////////////////////////////////////
Camera g_Camera(60.0f, SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
Navigator g_Navigator(1.0f, 0.01f, glm::vec3(0, 0, 3));
glm::vec3 g_LightPosition = glm::vec3(0, 1, 0);
glm::vec3 g_LightColor(1, 1, 0.8f);
float g_LightTransparency = 0.75f;
TwType g_vec2Type;
TwType g_vec3Type;
TwType g_vec4Type;
TwType g_textureType;
bool g_ShowLight = true;

//////////////////////////////////////////////////////////////////////////
void glfw_error_callback(int error, const char* description)
{
	std::cout << description << std::endl;
}

//////////////////////////////////////////////////////////////////////////
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (TwEventKeyGLFW(key, action))
		return;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}

	if (mods == GLFW_MOD_ALT)
	{
		switch (key)
		{
		case GLFW_KEY_LEFT:
		case GLFW_KEY_A:
			g_LightPosition += glm::vec3(-0.07f, 0, 0);
			break;
		case GLFW_KEY_UP:
		case GLFW_KEY_W:
			g_LightPosition += glm::vec3(0, 0, 0.07f);
			break;
		case GLFW_KEY_DOWN:
		case GLFW_KEY_S:
			g_LightPosition += glm::vec3(0, 0, -0.07f);
			break;
		case GLFW_KEY_RIGHT:
		case GLFW_KEY_D:
			g_LightPosition += glm::vec3(0.07f, 0, 0);
			break;
		case GLFW_KEY_KP_ADD:
		case GLFW_KEY_Q:
			g_LightPosition += glm::vec3(0, 0.07f, 0);
			break;
		case GLFW_KEY_KP_SUBTRACT:
		case GLFW_KEY_E:
			g_LightPosition += glm::vec3(0, -0.07f, 0);
			break;
		}
	}
	else
	{
		if (action == GLFW_PRESS)
		{
			g_Navigator.keyDown(key);
		}
		else if (action == GLFW_RELEASE)
			g_Navigator.keyUp(key);
	}
}

//////////////////////////////////////////////////////////////////////////
void char_callback(GLFWwindow* window, int codepoint)
{
	TwEventCharGLFW(codepoint, GLFW_PRESS);
}

//////////////////////////////////////////////////////////////////////////
void mousebuttons_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (TwEventMouseButtonGLFW(button, action))
		return;

	if (action == GLFW_PRESS)
		g_Navigator.buttonDown(button);
	else if (action == GLFW_RELEASE)
		g_Navigator.buttonUp(button);
}

//////////////////////////////////////////////////////////////////////////
void mousemove_callback(GLFWwindow* window, double x, double y)
{
	g_Navigator.mouseMove((int)x, (int)y);
	TwMouseMotion(int(x), int(y));
}

//////////////////////////////////////////////////////////////////////////
void windowsize_callback(GLFWwindow* window, int width, int height)
{
	g_Camera.aspectRatio = width / (float)height;
	glViewport(0, 0, width, height);
	TwWindowSize(width, height);
}

//////////////////////////////////////////////////////////////////////////
void createSphereData(float radius, int hSlices, int vSlices, std::vector<glm::vec3>& vertices, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<unsigned>& indices)
{
	float thetaStep = 2.0f * glm::pi<float>() / (hSlices - 1.0f);
	float phiStep = glm::pi<float>() / (vSlices - 1.0f);
	float theta = 0.0f;
	for (auto i = 0; i < hSlices; i++, theta += thetaStep)
	{
		float cosTheta = std::cos(theta);
		float sinTheta = std::sin(theta);
		float phi = 0.0f;
		for (auto j = 0; j < vSlices; j++, phi += phiStep)
		{
			float nx = -std::sin(phi) * cosTheta;
			float ny = -std::cos(phi);
			float nz = -std::sin(phi) * sinTheta;
			float n = std::sqrt(nx * nx + ny * ny + nz * nz);
			if (n < 0.99f || n > 1.01f)
			{
				nx = nx / n;
				ny = ny / n;
				nz = nz / n;
			}
			// NOTE: texture repeats twice horizontally
			float tx = theta / glm::pi<float>();
			float ty = phi / glm::pi<float>();

			vertices.emplace_back(nx * radius, ny * radius, nz * radius);
			normals.emplace_back(nx, ny, nz);
			uvs.emplace_back(tx, ty);
		}
	}
	for (auto i = 0; i < hSlices - 1; i++)
	{
		for (auto j = 0; j < vSlices - 1; j++)
		{
			unsigned int baseIndex = (i * vSlices + j);
			indices.emplace_back(baseIndex);
			indices.emplace_back(baseIndex + hSlices + 1);
			indices.emplace_back(baseIndex + hSlices);
			indices.emplace_back(baseIndex);
			indices.emplace_back(baseIndex + 1);
			indices.emplace_back(baseIndex + hSlices + 1);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void inspectUniforms(GLuint program, std::map<std::string, Uniform>& uniformsMap)
{
	GLint N = 0;
	glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &N);

	const GLenum properties[4] = { GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH, GL_LOCATION };

	for (int i = 0; i < N; i++)
	{
		GLint values[4];
		glGetProgramResourceiv(program, GL_UNIFORM, i, 4, properties, 4, NULL, values);

		// skip any uniforms that are in a block.
		if (values[0] != -1)
			continue;

		// get the name. Must use a std::vector rather than a std::string for C++03 standards issues.
		// C++11 would let you use a std::string directly.
		std::vector<char> nameData(values[2]);
		glGetProgramResourceName(program, GL_UNIFORM, i, (GLsizei)nameData.size(), NULL, &nameData[0]);
		std::string name(nameData.begin(), nameData.end() - 1);

		Uniform newUniform(program, name, values[1], values[3]);
		uniformsMap.emplace(name, newUniform);
	}
}

bool isBuiltIn(Uniform& uniform)
{
	return uniform.name == "model" ||
		uniform.name == "view" ||
		uniform.name == "projection" ||
		uniform.name == "eyePosition" ||
		uniform.name.substr(0, 5) == "light";
}

template <typename T>
void TW_CALL setUniformValue_callback(const void* value, void* clientData)
{
	static_cast<Uniform*>(clientData)->setValue(*static_cast<const T*>(value), true);
}

template <typename T>
void TW_CALL getUniformValue_callback(void* value, void* clientData)
{
	T* ptr = static_cast<Uniform*>(clientData)->getValuePtr<T>();
	if (ptr == nullptr)
		value = nullptr;
	else
		*reinterpret_cast<T*>(value) = *ptr;
}

void defineAnttweakbarStructs()
{
	TwStructMember structMembers1[] = {
		{ "x", TW_TYPE_FLOAT, offsetof(glm::vec2, x), "step=0.1" },
		{ "y", TW_TYPE_FLOAT, offsetof(glm::vec2, y), "step=0.1" }
	};
	g_vec2Type = TwDefineStruct(
		"Vec2Type",
		structMembers1,
		2,
		sizeof(glm::vec2),
		NULL,
		NULL);
	TwStructMember structMembers2[] = {
		{ "x", TW_TYPE_FLOAT, offsetof(glm::vec3, x), "step=0.1" },
		{ "y", TW_TYPE_FLOAT, offsetof(glm::vec3, y), "step=0.1" },
		{ "z", TW_TYPE_FLOAT, offsetof(glm::vec3, z), "step=0.1" }
	};
	g_vec3Type = TwDefineStruct(
		"Vec3Type",
		structMembers2,
		3,
		sizeof(glm::vec3),
		NULL,
		NULL);
	TwStructMember structMembers3[] = {
		{ "x", TW_TYPE_FLOAT, offsetof(glm::vec4, x), "step=0.1" },
		{ "y", TW_TYPE_FLOAT, offsetof(glm::vec4, y), "step=0.1" },
		{ "z", TW_TYPE_FLOAT, offsetof(glm::vec4, z), "step=0.1" },
		{ "w", TW_TYPE_FLOAT, offsetof(glm::vec4, w), "step=0.1" }
	};
	g_vec4Type = TwDefineStruct(
		"Vec4Type",
		structMembers3,
		4,
		sizeof(glm::vec4),
		NULL,
		NULL);
	TwStructMember structMembers4[] = {
		{ "filename", TW_TYPE_CSSTRING(256), offsetof(Texture, filename), "" },
		{ "unit", TW_TYPE_UINT32, offsetof(Texture, unit), "" }
	};
	g_textureType = TwDefineStruct(
		"TextureType",
		structMembers4,
		2,
		sizeof(Texture),
		NULL,
		NULL);
}

void initializeUniformsInspector(TwBar* bar, std::map<std::string, Uniform>& uniformsMap)
{
	for (auto& entry : uniformsMap)
	{
		auto& uniform = entry.second;
		if (isBuiltIn(uniform))
			continue;

		switch (uniform.type)
		{
		case GL_BOOL:
			TwAddVarCB(bar, uniform.name.c_str(), TW_TYPE_BOOLCPP, setUniformValue_callback<bool>, getUniformValue_callback<bool>, &uniform, "");
			break;
		case GL_FLOAT:
			TwAddVarCB(bar, uniform.name.c_str(), TW_TYPE_FLOAT, setUniformValue_callback<float>, getUniformValue_callback<float>, &uniform, "step=0.1");
			break;
		case GL_FLOAT_VEC2:
			TwAddVarCB(bar, uniform.name.c_str(), g_vec2Type, setUniformValue_callback<glm::vec2>, getUniformValue_callback<glm::vec2>, &uniform, "");
			break;
		case GL_FLOAT_VEC3:
			TwAddVarCB(bar, uniform.name.c_str(), g_vec3Type, setUniformValue_callback<glm::vec3>, getUniformValue_callback<glm::vec3>, &uniform, "");
			break;
		case GL_FLOAT_VEC4:
			TwAddVarCB(bar, uniform.name.c_str(), g_vec4Type, setUniformValue_callback<glm::vec4>, getUniformValue_callback<glm::vec4>, &uniform, "");
			break;
		case GL_SAMPLER_2D:
			TwAddVarCB(bar, uniform.name.c_str(), g_textureType, setUniformValue_callback<Texture>, getUniformValue_callback<Texture>, &uniform, "");
			break;
			/*case GL_FLOAT_MAT3:
			case GL_FLOAT_MAT4:
			case GL_INT_VEC2:
			case GL_INT_VEC3:
			case GL_INT_VEC4:
			case GL_BOOL_VEC2:
			case GL_BOOL_VEC3:
			case GL_BOOL_VEC4:
			case GL_FLOAT_MAT2:
			case GL_SAMPLER_1D:
			case GL_SAMPLER_3D:
			case GL_SAMPLER_CUBE:
			case GL_SAMPLER_1D_SHADOW:
			case GL_SAMPLER_2D_SHADOW:*/
		default:
			std::cout << "unsupported uniform type (" << uniform.type << ")" << std::endl;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template <size_t UniqueIndex>
void TwAddStringVarRO(TwBar* bar, const char* name, const std::string& value)
{
	static char cString[1024];
	auto count = std::min((size_t)1023, value.size());
	strncpy_s(cString, value.c_str(), count);
	cString[count + 1] = (char)'\0';
	TwAddVarRO(bar, name, TW_TYPE_CSSTRING(1024), cString, "");
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	std::string obj = argc >= 2 ? argv[1] : "bunny.obj";
	std::string vertexShader = argc >= 3 ? argv[2] : "common.vs.glsl";
	std::string fragmentShader = argc >= 4 ? argv[3] : "cook_torrance_colored.fs.glsl";

	auto t_start = std::chrono::high_resolution_clock::now();

	//////////////////////////////////////////////////////////////////////////
	// Initialize GLFW and create window

	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "application", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwSetErrorCallback(glfw_error_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, (GLFWcharfun)char_callback);
	glfwSetMouseButtonCallback(window, mousebuttons_callback);
	glfwSetCursorPosCallback(window, mousemove_callback);
	glfwSetWindowSizeCallback(window, windowsize_callback);

	//////////////////////////////////////////////////////////////////////////
	// Initialize GLEW

	glewExperimental = GL_TRUE;
	glewInit();

	//////////////////////////////////////////////////////////////////////////
	// Initialize AntTweakBar
	TwInit(TW_OPENGL_CORE, 0);

	defineAnttweakbarStructs();

	auto bar0 = TwNewBar("Scene");
	auto bar1 = TwNewBar("Uniforms");
	TwDefine(" Scene position='10 10' ");
	TwDefine(" Uniforms position='590 10' ");
	TwWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);

	TwAddStringVarRO<0>(bar0, "OBJ", obj);
	TwAddStringVarRO<1>(bar0, "Vertex Shader", vertexShader);
	TwAddStringVarRO<2>(bar0, "Fragment Shader", fragmentShader);
	TwAddVarRW(bar0, "Show Light", TW_TYPE_BOOLCPP, &g_ShowLight, "");
	TwAddVarRW(bar0, "Light Color", g_vec3Type, &g_LightColor, "");
	TwAddVarRW(bar0, "Light Position", g_vec3Type, &g_LightPosition, "");
	TwAddVarRW(bar0, "Light Transparency", TW_TYPE_FLOAT, &g_LightTransparency, "min=0 max=1 step=0.1");

	//////////////////////////////////////////////////////////////////////////
	// Setup basic GL states

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	// NOTE: opening scope so that objects created inside of it can be destroyed before the program ends
	{
		// Reading shader source from files
		Shader shader0(SHADERS_DIR + vertexShader, SHADERS_DIR + fragmentShader);
		Shader shader1(SHADERS_DIR + "common.vs.glsl", SHADERS_DIR + "light_source.fs.glsl");

		std::map<std::string, Uniform> uniformsMap;
		inspectUniforms(shader0, uniformsMap);
		initializeUniformsInspector(bar1, uniformsMap);

		//////////////////////////////////////////////////////////////////////////
		// Load OBJ file

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;
		if (!loadOBJData((MEDIA_DIR + obj).c_str(), vertices, uvs, normals))
		{
			std::cout << "error loading OBJ" << std::endl;
			exit(EXIT_FAILURE);
		}
		Mesh objMesh(vertices, uvs, normals);

		vertices.clear();
		uvs.clear();
		normals.clear();

		//////////////////////////////////////////////////////////////////////////
		// Create sphere mesh

		std::vector<unsigned> indices;
		createSphereData(0.25f, 30, 30, vertices, uvs, normals, indices);
		Mesh sphereMesh(vertices, uvs, normals, indices);

		// Setting VAO pointers to the allocated VBOs, which is only necessary ONCE since we're always rendering a mesh with the same shader
		objMesh.setup(shader0);
		sphereMesh.setup(shader1);

		glUseProgram(shader0);
		// Loading textures according to sequential 'tex' uniforms in shader0
		unsigned c = 0;
		for (auto& entry : uniformsMap)
		{
			auto& uniform = entry.second;
			if (uniform.name.substr(0, 3) != "tex")
				continue;
			auto i = atoi(uniform.name.substr(3).c_str());
			uniform.setValue(Texture(uniform.name + ".png", c++));
		}

		GLuint uModel = glGetUniformLocation(shader1, "model");
		GLuint uView = glGetUniformLocation(shader1, "view");
		GLuint uProjection = glGetUniformLocation(shader1, "projection");
		GLuint uEyePosition = glGetUniformLocation(shader1, "eyePosition");
		GLuint uColor = glGetUniformLocation(shader1, "color");
		GLuint uTransparency = glGetUniformLocation(shader1, "transparency");

		while (!glfwWindowShouldClose(window))
		{
			auto start = glfwGetTime();

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto eyePosition = g_Navigator.getPosition();
			auto view = g_Navigator.getLocalToWorldTransform();
			auto projection = g_Camera.getProjection();

			//////////////////////////////////////////////////////////////////////////
			// Draw OBJ

			glUseProgram(shader0);
			auto it = uniformsMap.find("lightPosition");
			if (it != uniformsMap.end())
				it->second.setValue(g_LightPosition);
			it = uniformsMap.find("lightColor");
			if (it != uniformsMap.end())
				it->second.setValue(g_LightColor);
			it = uniformsMap.find("eyePosition");
			if (it != uniformsMap.end())
				it->second.setValue(eyePosition);
			it = uniformsMap.find("view");
			if (it != uniformsMap.end())
				it->second.setValue(view);
			it = uniformsMap.find("projection");
			if (it != uniformsMap.end())
				it->second.setValue(projection);
			it = uniformsMap.find("model");
			if (it != uniformsMap.end())
				it->second.setValue(glm::mat4(1));

			objMesh.draw();

			checkOpenGLError();

			if (g_ShowLight)
			{
				//////////////////////////////////////////////////////////////////////////
				// Draw light

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glUseProgram(shader1);
				glUniform3fv(uEyePosition, 1, glm::value_ptr(eyePosition));
				glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1), g_LightPosition)));
				glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
				glUniform3fv(uColor, 1, glm::value_ptr(g_LightColor.xyz()));
				glUniform1f(uTransparency, g_LightTransparency);
				sphereMesh.draw();
				glDisable(GL_BLEND);
			}

			TwDraw();

			glfwSwapBuffers(window);

			checkOpenGLError();

			auto diff = (float)(start - glfwGetTime());

			g_Navigator.update(diff);

			glfwPollEvents();
		}
	}

	TwTerminate();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}