///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
// AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 * SceneManager()
 *
 * The constructor for the class. Initializes the shader manager
 * and the basic meshes object.
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes(); // Assuming ShapeMeshes handles vertex data for basic shapes
	m_loadedTextures = 0; // Initialize the loaded textures count
}

/***********************************************************
 * ~SceneManager()
 *
 * The destructor for the class. Cleans up allocated memory.
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// Note: Textures should ideally be destroyed here as well
	DestroyGLTextures(); // Call the texture destruction method
}

/***********************************************************
 * CreateGLTexture()
 *
 * This method is used for loading textures from image files,
 * configuring the texture mapping parameters in OpenGL,
 * generating the mipmaps, and loading the read texture into
 * the next available texture slot in memory.
 *
 * @param filename: Path to the image file.
 * @param tag: A string tag to identify the texture.
 * @return bool: True if texture loading was successful, false otherwise.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			// Free image data even if not handled
			stbi_image_free(image);
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// Check if we have space for more textures
		if (m_loadedTextures < MAX_TEXTURES) // MAX_TEXTURES is defined in SceneManager.h
		{
			// register the loaded texture and associate it with the special tag string
			m_textureIDs[m_loadedTextures].ID = textureID;
			m_textureIDs[m_loadedTextures].tag = tag;
			m_loadedTextures++;
		}
		else
		{
			std::cout << "Maximum number of textures loaded. Could not load:" << filename << std::endl;
			// Delete the generated texture if it couldn't be stored
			glDeleteTextures(1, &textureID);
			return false;
		}


		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 * BindGLTextures()
 *
 * This method is used for binding the loaded textures to
 * OpenGL texture memory slots. There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 * DestroyGLTextures()
 *
 * This method is used for freeing the memory in all the
 * used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
	m_loadedTextures = 0; // Reset the count of loaded textures
}

/***********************************************************
 * FindTextureID()
 *
 * This method is used for getting an ID for the previously
 * loaded texture bitmap associated with the passed in tag.
 *
 * @param tag: The string tag associated with the texture.
 * @return int: The texture ID if found, -1 otherwise.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 * FindTextureSlot()
 *
 * This method is used for getting a slot index for the previously
 * loaded texture bitmap associated with the passed in tag.
 *
 * @param tag: The string tag associated with the texture.
 * @return int: The texture slot index if found, -1 otherwise.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 * FindMaterial()
 *
 * This method is used for getting a material from the previously
 * defined materials list that is associated with the passed in tag.
 *
 * @param tag: The string tag associated with the material.
 * @param material: Output parameter to store the found material properties.
 * @return bool: True if the material was found, false otherwise.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 * SetTransformations()
 *
 * This method is used for setting the transform buffer
 * using the passed in transformation values. It calculates
 * the model matrix from scale, rotation, and translation.
 *
 * @param scaleXYZ: Scaling factors along X, Y, and Z axes.
 * @param XrotationDegrees: Rotation around the X-axis in degrees.
 * @param YrotationDegrees: Rotation around the Y-axis in degrees.
 * @param ZrotationDegrees: Rotation around the Z-axis in degrees.
 * @param positionXYZ: Translation vector.
 * @param offset: Additional offset vector (optional).
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ + offset);

	// Combine transformations: Scale -> Rotate -> Translate
	modelView = translation * rotationZ * rotationY * rotationX * scale;

	// Pass the model matrix to the shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 * SetShaderColor()
 *
 * This method is used for setting the passed in color
 * into the shader for the next draw command. Disables texturing.
 *
 * @param redColorValue: Red component of the color (0.0 to 1.0).
 * @param greenColorValue: Green component of the color (0.0 to 1.0).
 * @param blueColorValue: Blue component of the color (0.0 to 1.0).
 * @param alphaValue: Alpha component of the color (0.0 to 1.0).
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false); // Disable texturing
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 * SetShaderTexture()
 *
 * This method is used for setting the texture data
 * associated with the passed in ID into the shader. Enables texturing.
 *
 * @param textureTag: The string tag of the texture to use.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true); // Enable texturing

		int textureID = -1;
		textureID = FindTextureSlot(textureTag); // Get the texture slot (index)
		if (textureID != -1)
		{
			m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID); // Pass the texture slot to the shader
		}
		else
		{
			std::cout << "Warning: Texture with tag '" << textureTag << "' not found." << std::endl;
			m_pShaderManager->setIntValue(g_UseTextureName, false); // Disable texturing if texture not found
		}
	}
}

/***********************************************************
 * SetTextureUVScale()
 *
 * This method is used for setting the texture UV scale
 * values into the shader. This controls how textures tile on a surface.
 *
 * @param u: Scaling factor for the U texture coordinate.
 * @param v: Scaling factor for the V texture coordinate.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 * SetShaderMaterial()
 *
 * This method is used for passing the material values
 * into the shader. These values are used for lighting calculations (Phong model).
 *
 * @param materialTag: The string tag of the material to use.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
		else
		{
			std::cout << "Warning: Material with tag '" << materialTag << "' not found." << std::endl;
			// Optionally set a default material or handle the error
		}
	}
}
/***********************************************************
 * DefineObjectMaterials()
 *
 * This method is used for configuring the various material
 * settings for all of the objects within the 3D scene.
 * Each material includes diffuse color, specular color, and shininess.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Clear any existing materials
	m_objectMaterials.clear();

	// Define various materials for objects in the scene

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.8f);
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.shininess = 1.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.2f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.8f);
	metalMaterial.shininess = 8.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.8f);
	glassMaterial.shininess = 10.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tileMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	tileMaterial.shininess = 6.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	stoneMaterial.specularColor = glm::vec3(0.73f, 0.3f, 0.3f);
	stoneMaterial.shininess = 6.0;
	stoneMaterial.tag = "stone";
	m_objectMaterials.push_back(stoneMaterial);

	// Material for lamp shade 
	OBJECT_MATERIAL shadeMaterial;
	shadeMaterial.diffuseColor = glm::vec3(1.0f, 0.98f, 0.88f); 
	shadeMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); 
	shadeMaterial.shininess = 0.5; 
	shadeMaterial.tag = "lampshade";
	m_objectMaterials.push_back(shadeMaterial);

	// Material for lamp base 
	OBJECT_MATERIAL lampBaseMaterial;
	lampBaseMaterial.diffuseColor = glm::vec3(0.25f, 0.15f, 0.05f); 
	lampBaseMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.1f);
	lampBaseMaterial.shininess = 3.0;
	lampBaseMaterial.tag = "lampbase";
	m_objectMaterials.push_back(lampBaseMaterial);

	// Material for book covers 
	OBJECT_MATERIAL bookCoverMaterial;
	bookCoverMaterial.diffuseColor = glm::vec3(0.4f, 0.05f, 0.05f);
	bookCoverMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	bookCoverMaterial.shininess = 0.8;
	bookCoverMaterial.tag = "bookcover";
	m_objectMaterials.push_back(bookCoverMaterial);

	// Material for jar (ceramic) 
	OBJECT_MATERIAL jarMaterial;
	jarMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.9f);
	jarMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.4f);
	jarMaterial.shininess = 3.0;
	jarMaterial.tag = "jar";
	m_objectMaterials.push_back(jarMaterial);

	// Material for reflective table surface
	OBJECT_MATERIAL tableSurfaceMaterial;
	tableSurfaceMaterial.diffuseColor = glm::vec3(0.4f, 0.3f, 0.2f); // Base color
	tableSurfaceMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // High specularity for reflection
	tableSurfaceMaterial.shininess = 30.0; // High shininess for sharp reflection
	tableSurfaceMaterial.tag = "tableSurface";
	m_objectMaterials.push_back(tableSurfaceMaterial);

	// Material for window frame (example: white painted wood)
	OBJECT_MATERIAL windowFrameMaterial;
	windowFrameMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f); // Off-white
	windowFrameMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	windowFrameMaterial.shininess = 1.0;
	windowFrameMaterial.tag = "windowFrame";
	m_objectMaterials.push_back(windowFrameMaterial);

}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 * LoadSceneTextures()
 *
 * This method is used for loading all the textures required
 * for the 3D scene. Ensures textures are bound after loading.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// Note: Ensure these texture files exist in the correct directory
	// Note: Ensure textures are royalty-free and at least 1024x1024 resolution

	CreateGLTexture("textures/wooden.jpg", "wooden"); // Could be used for some lamp parts
	CreateGLTexture("textures/vase.jpg", "vase");     // Could be used for the jar pattern
	CreateGLTexture("textures/table.jpg", "table");   // For the table surface
	CreateGLTexture("textures/stand.jpg", "stand");   // lamp base detail
	CreateGLTexture("textures/neck.jpg", "neck");     // lamp neck detail
	CreateGLTexture("textures/book_cover.jpg", "bookcover_tex"); //book cover texture exists
	CreateGLTexture("textures/window_frame_tex.jpg", "window_frame_tex");

	


	BindGLTextures();

}


/***********************************************************
 * PrepareScene()
 *
 * This method is used for preparing the 3D scene by loading
 * the shapes, textures, and setting up lights and materials
 * in memory to support the 3D scene rendering. This should
 * be called once before the rendering loop.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures, define materials, and set up lights
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// Load basic meshes into memory
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();


}

/***********************************************************
 * SetupSceneLights()
 *
 * This method is called to add and configure the light
 * sources for the 3D scene. Includes directional, spotlight,
 * and point lights with ambient, diffuse, and specular components.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light: Simulating sunlight from upper right
	// This light affects all objects equally regardless of distance.
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.8f, -0.6f, -0.4f); // Direction of the light
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1f, 0.1f, 0.1f); // Ambient color
	// Increased diffuse and specular for a stronger sunlight effect
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.9f, 0.9f, 0.8f); // Brighter diffuse color
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f); // Stronger specular highlight
	m_pShaderManager->setBoolValue("directionalLight.bActive", true); // Activate directional light

	//lamp light
	// This light is positioned at a point and shines in a specific direction within a cone.
	
	
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -1.0f, -0.2f); // Pointing slightly forward
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.5f, 0.5f, 0.5f); // Increased ambient
	m_pShaderManager->setVec3Value("spotLight.diffuse", 0.9f, 0.9f, 0.9f); // Diffuse color
	m_pShaderManager->setVec3Value("spotLight.specular", 0.6f, 0.6f, 0.6f); // Specular color
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f); // Light falloff factor
	m_pShaderManager->setFloatValue("spotLight.linear", 0.07f); // Light falloff factor
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.017f); // Light falloff factor
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(12.0f))); // Inner cone angle
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(15.0f))); // Outer cone angle
	m_pShaderManager->setBoolValue("spotLight.bActive", true); // Activate spotlight


	// Point light: Example point light (can be used for general illumination)
	// This light is positioned at a point and emits light in all directions.
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 1.5f, 2.5f); // Position of the point light
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f); // Ambient color
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.6f); // Diffuse color
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.8f, 0.8f, 0.8f); // Specular color
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f); // Light falloff factor
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f); // Light falloff factor
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f); // Light falloff factor
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true); // Activate point light

	// Example of a colored point light
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 1.0f, -2.0f); // Position of the colored point light
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.02f, 0.01f, 0.01f); // Low ambient
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.5f, 0.2f, 0.2f); // Reddish diffuse color
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.6f, 0.3f, 0.3f); // Reddish specular color
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.1f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.05f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true); // Activate colored point light


}

/***********************************************************
* RenderScene()
*
* This method is used for rendering the 3D scene by
* transforming and drawing the basic 3D shapes. It places
* and styles each object according to the scene design.
***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees, YrotationDegrees, ZrotationDegrees;
	glm::vec3 positionXYZ;

	// --- Draw Table ---
	/********* Draw Floor Plane (Table Surface) *********/
	// Adjusted size to better fit objects and scene scale
	scaleXYZ = glm::vec3(10.0f, 0.1f, 5.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("table"); 
	SetShaderMaterial("tableSurface"); // Using the table surface material for reflection
	SetTextureUVScale(2.0f, 1.0f); 
	m_basicMeshes->DrawPlaneMesh(); // Draw the table surface using a plane mesh

	// --- Draw Lamp 1 ---
	{
		glm::vec3 basePos = glm::vec3(-3.0f, 0.05f, 0.0f); // Base position for the first lamp

		/********* Draw Base Bottom (Box) *********/
		// Adjusted size and position to better approximate the lamp base
		scaleXYZ = glm::vec3(1.8f, 0.3f, 1.8f);
		SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, basePos);
		SetShaderTexture("stand");
		SetShaderMaterial("lampbase"); // Using lamp base material
		m_basicMeshes->DrawBoxMesh();

		/********* Draw Base Mid (Cylinder) *********/
		// Adjusted size and position
		scaleXYZ = glm::vec3(1.3f, 0.4f, 1.3f);
		positionXYZ = basePos + glm::vec3(0.0f, 0.3f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		/********* Draw Base Top (Cone - inverted) *********/
		// Adjusted size and position
		scaleXYZ = glm::vec3(1.5f, 0.5f, 1.5f);
		positionXYZ = basePos + glm::vec3(0.0f, 0.7f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 180.0f, positionXYZ); // Inverted cone to form the top of the base
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawConeMesh();

		/********* Draw Lower Body (Combination of Cylinders and Spheres for curves) *********/
		// This section uses multiple basic shapes to approximate the curved and detailed lamp body
		
		glm::vec3 lowerBodyPos = basePos + glm::vec3(0.0f, 1.3f, 0.0f);
		scaleXYZ = glm::vec3(1.1f, 1.0f, 1.1f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, lowerBodyPos);
		SetShaderTexture("neck"); 
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		glm::vec3 curvePos1 = basePos + glm::vec3(0.0f, 2.3f, 0.0f);
		scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, curvePos1);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawSphereMesh(); // Using a sphere to suggest a curve

		scaleXYZ = glm::vec3(0.9f, 1.2f, 0.9f);
		lowerBodyPos = basePos + glm::vec3(0.0f, 3.0f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, lowerBodyPos);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		glm::vec3 curvePos2 = basePos + glm::vec3(0.0f, 4.2f, 0.0f);
		scaleXYZ = glm::vec3(0.8f, 0.4f, 0.8f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, curvePos2);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawSphereMesh(); // Using a sphere to suggest a curve


		/********* Draw Upper Body (Thinner Cylinder) *********/
		
		glm::vec3 upperBodyPos = basePos + glm::vec3(0.0f, 5.0f, 0.0f);
		scaleXYZ = glm::vec3(0.4f, 3.0f, 0.4f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, upperBodyPos);
		SetShaderTexture("wooden"); 
		SetShaderMaterial("wood");
		m_basicMeshes->DrawCylinderMesh();

		/********* Draw Shade (Single Cone - wider part down) *********/
		
		
		glm::vec3 shadePos = basePos + glm::vec3(0.0f, 8.0f, 0.0f); 
		scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f); 
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, shadePos);
		SetShaderMaterial("lampshade"); 
		m_basicMeshes->DrawConeMesh();

		/********* Draw Finial (Small Sphere) *********/
		// Adjusted position to sit on top of the shade
		glm::vec3 finialPos = basePos + glm::vec3(0.0f, 10.5f, 0.0f);
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f); 
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, finialPos);
		SetShaderMaterial("metal"); // Using metal material for finial
		m_basicMeshes->DrawSphereMesh(); 
	}

	// --- Draw Lamp 2 (Mirror of Lamp 1) ---
	// This lamp is a duplicate of Lamp 1, mirrored across the center
	{
		glm::vec3 basePos = glm::vec3(3.0f, 0.05f, 0.0f); // Base position for the second lamp

		/********* Draw Base Bottom (Box) *********/
		// Adjusted size and position to better approximate the lamp base
		scaleXYZ = glm::vec3(1.8f, 0.3f, 1.8f);
		SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, basePos);
		SetShaderTexture("stand"); 
		SetShaderMaterial("lampbase"); 
		m_basicMeshes->DrawBoxMesh();

		/********* Draw Base Mid (Cylinder) *********/
		// Adjusted size and position
		scaleXYZ = glm::vec3(1.3f, 0.4f, 1.3f);
		positionXYZ = basePos + glm::vec3(0.0f, 0.3f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		/********* Draw Base Top (Cone - inverted) *********/
		// Adjusted size and position
		scaleXYZ = glm::vec3(1.5f, 0.5f, 1.5f);
		positionXYZ = basePos + glm::vec3(0.0f, 0.7f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 180.0f, positionXYZ); // Inverted cone to form the top of the base
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawConeMesh();

		/********* Draw Lower Body (Combination of Cylinders and Spheres for curves) *********/
		// This section uses multiple basic shapes to approximate the curved and detailed lamp body
		// Adjusted sizes and positions for a slightly more curved look
		glm::vec3 lowerBodyPos = basePos + glm::vec3(0.0f, 1.3f, 0.0f);
		scaleXYZ = glm::vec3(1.1f, 1.0f, 1.1f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, lowerBodyPos);
		SetShaderTexture("neck"); // Assuming 'neck' texture is suitable for this part
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		glm::vec3 curvePos1 = basePos + glm::vec3(0.0f, 2.3f, 0.0f);
		scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, curvePos1);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawSphereMesh(); // Using a sphere to suggest a curve

		scaleXYZ = glm::vec3(0.9f, 1.2f, 0.9f);
		lowerBodyPos = basePos + glm::vec3(0.0f, 3.0f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, lowerBodyPos);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawCylinderMesh();

		glm::vec3 curvePos2 = basePos + glm::vec3(0.0f, 4.2f, 0.0f);
		scaleXYZ = glm::vec3(0.8f, 0.4f, 0.8f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, curvePos2);
		SetShaderMaterial("lampbase");
		m_basicMeshes->DrawSphereMesh(); // Using a sphere to suggest a curve


		/********* Draw Upper Body (Thinner Cylinder) *********/
		// Adjusted height and position
		glm::vec3 upperBodyPos = basePos + glm::vec3(0.0f, 5.0f, 0.0f);
		scaleXYZ = glm::vec3(0.4f, 3.0f, 0.4f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, upperBodyPos);
		SetShaderTexture("wooden"); // Assuming 'wooden' texture is suitable
		SetShaderMaterial("wood");
		m_basicMeshes->DrawCylinderMesh();

		/********* Draw Shade (Single Cone - wider part down) *********/
		// Using a single cone for a simplified shade shape, oriented upright
		// Adjusted position and scale to fit the lamp body better
		glm::vec3 shadePos = basePos + glm::vec3(0.0f, 8.0f, 0.0f); // Position of the shade
		scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f); // Adjusted scale for a single cone
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, shadePos); 
		SetShaderMaterial("lampshade"); // Using the lampshade material with cream color
		m_basicMeshes->DrawConeMesh();

		/********* Draw Finial (Small Sphere) *********/
		// Adjusted position to sit on top of the shade
		glm::vec3 finialPos = basePos + glm::vec3(0.0f, 10.5f, 0.0f);
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f); 
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, finialPos);
		SetShaderMaterial("metal"); 
		m_basicMeshes->DrawSphereMesh(); 
	}

	// --- Draw Books ---
	/********* Draw Book 1 (Bottom Book) *********/
	// Adjusted position and scale
	glm::vec3 book1Pos = glm::vec3(0.0f, 0.05f, 0.0f);
	scaleXYZ = glm::vec3(2.8f, 0.15f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, book1Pos);
	SetShaderTexture("bookcover_tex"); // Using book cover texture
	SetShaderMaterial("bookcover"); // Using book cover material
	SetTextureUVScale(1.0f, 1.0f); // Adjust UV scale as needed for texture mapping
	m_basicMeshes->DrawBoxMesh();

	/********* Draw Book 2 (Stacked on Book 1) *********/
	// Adjusted position and scale, keeping slight rotation
	glm::vec3 book2Pos = glm::vec3(0.0f, 0.21f, 0.0f);
	scaleXYZ = glm::vec3(2.6f, 0.12f, 1.9f);
	SetTransformations(scaleXYZ, 0.0f, 5.0f, 0.0f, book2Pos); // Keep slight rotation
	SetShaderTexture("bookcover_tex"); // Using book cover texture
	SetShaderMaterial("bookcover"); // Using book cover material
	SetTextureUVScale(1.0f, 1.0f); // Adjust UV scale as needed
	m_basicMeshes->DrawBoxMesh();

	// --- Draw Jar ---
	/********* Draw Jar (Combination of Cylinders and Spheres for jar shape) *********/

	// Adjusted position and scales to better match the jar shape
	glm::vec3 jarPos = glm::vec3(0.0f, 0.36f, 0.0f); // Position on top of books
	// Jar base
	scaleXYZ = glm::vec3(0.8f, 0.6f, 0.8f); // Wider base cylinder
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, jarPos);
	SetShaderTexture("vase"); // Using 'vase' texture for pattern
	SetShaderMaterial("jar"); // Using jar material
	SetTextureUVScale(1.0f, 1.0f); // Adjust UV scale as needed
	m_basicMeshes->DrawCylinderMesh();

	// Jar body (larger sphere)
	glm::vec3 jarBodyPos = jarPos + glm::vec3(0.0f, 0.6f, 0.0f);
	scaleXYZ = glm::vec3(0.9f, 0.9f, 0.9f); // Larger sphere for the main body
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, jarBodyPos);
	SetShaderTexture("vase"); // Using 'vase' texture for pattern
	SetShaderMaterial("jar");
	SetTextureUVScale(1.0f, 1.0f); // Adjust UV scale as needed
	m_basicMeshes->DrawSphereMesh();

	// Jar neck (thin cylinder)
	glm::vec3 jarNeckPos = jarPos + glm::vec3(0.0f, 1.5f, 0.0f);
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, jarNeckPos);
	SetShaderMaterial("jar"); // Using jar material
	m_basicMeshes->DrawCylinderMesh();

	// Jar lid (flattened sphere or cylinder)
	glm::vec3 lidPos = jarPos + glm::vec3(0.0f, 1.9f, 0.0f);
	scaleXYZ = glm::vec3(0.7f, 0.3f, 0.7f); // Flattened sphere for lid
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, lidPos);
	SetShaderMaterial("jar"); // Using jar material for lid
	m_basicMeshes->DrawSphereMesh();

	// Small sphere on top of lid (handle)
	glm::vec3 handlePos = jarPos + glm::vec3(0.0f, 2.1f, 0.0f);
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, handlePos);
	SetShaderMaterial("jar"); // Using jar material for handle
	m_basicMeshes->DrawSphereMesh();

	// --- Draw Window and Outside Detail ---
	/********* Draw Outside View (Large Plane) *********/
	// Position this behind the table and objects
	glm::vec3 outsidePos = glm::vec3(0.0f, 5.0f, -5.0f); // Position behind the table, elevated
	scaleXYZ = glm::vec3(15.0f, 10.0f, 0.1f); // Large plane for the background
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, outsidePos);
	
	
	SetShaderMaterial("tile");
	SetTextureUVScale(1.0f, 1.0f); 
	m_basicMeshes->DrawPlaneMesh();

	/********* Draw Window Frame (Boxes) *********/
	
	glm::vec3 windowFramePos = glm::vec3(0.0f, 5.0f, -4.9f); // Slightly in front of the outside view
	SetShaderMaterial("windowFrame"); // Use the window frame material

	// Top horizontal frame
	scaleXYZ = glm::vec3(7.5f, 0.3f, 0.1f); // Adjusted size
	positionXYZ = windowFramePos + glm::vec3(0.0f, 4.15f, 0.0f); // Adjusted position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("window_frame_tex"); // Apply window frame texture
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Bottom horizontal frame
	scaleXYZ = glm::vec3(7.5f, 0.3f, 0.1f); // Adjusted size
	positionXYZ = windowFramePos + glm::vec3(0.0f, -4.15f, 0.0f); // Adjusted position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("window_frame_tex");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Left vertical frame
	scaleXYZ = glm::vec3(0.3f, 8.5f, 0.1f); // Adjusted size
	positionXYZ = windowFramePos + glm::vec3(-3.6f, 0.0f, 0.0f); // Adjusted position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("window_frame_tex");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Right vertical frame
	scaleXYZ = glm::vec3(0.3f, 8.5f, 0.1f); // Adjusted size
	positionXYZ = windowFramePos + glm::vec3(3.6f, 0.0f, 0.0f); // Adjusted position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Center vertical divider
	scaleXYZ = glm::vec3(0.15f, 8.3f, 0.1f); // Adjusted size
	positionXYZ = windowFramePos + glm::vec3(0.0f, 0.0f, 0.0f); // Adjusted position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/********* Draw Window Panes (Planes) *********/
	// Approximate the glass panes
	glm::vec3 windowPanePos = glm::vec3(0.0f, 5.0f, -4.8f); // Slightly in front of the frame

	
	
	SetTextureUVScale(1.0f, 1.0f); 

	// Left pane
	scaleXYZ = glm::vec3(100.4f, 100.2f, 0.05f); 
	positionXYZ = windowPanePos + glm::vec3(-1.8f, 0.0f, 0.0f); 
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ); 
	SetTextureUVScale(1.0f, 1.0f); 
	m_basicMeshes->DrawPlaneMesh();

	// Right pane
	scaleXYZ = glm::vec3(100.4f, 100.2f, 0.05f); 
	positionXYZ = windowPanePos + glm::vec3(1.8f, 0.0f, 0.0f); 
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetTextureUVScale(1.0f, 1.0f); 
	m_basicMeshes->DrawPlaneMesh();


	
}
