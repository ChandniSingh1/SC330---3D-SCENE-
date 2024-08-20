///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declare the global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
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
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
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
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
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
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
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
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
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
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
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
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
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
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
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
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("textures/floor.jpg", "floor");
	CreateGLTexture("textures/background.jpg", "background");
	CreateGLTexture("textures/orange.jpg", "orange");
	CreateGLTexture("textures/stem.jpg", "stem");
	CreateGLTexture("textures/leaf.jpg", "leaf");
	CreateGLTexture("textures/orangesticker.jpg", "orangesticker");
	CreateGLTexture("textures/lighter.jpg", "lighter");
	CreateGLTexture("textures/cup.jpg", "cup");
	CreateGLTexture("textures/waterbottle.jpg", "waterbottle");
	CreateGLTexture("textures/white.jpg", "white");
	CreateGLTexture("textures/waterbottlecap.jpg", "thecap");
	CreateGLTexture("textures/waterbottlelabel.jpg", "thelabel");
	CreateGLTexture("textures/cuplabel.jpg", "cuplabel");
	CreateGLTexture("textures/lightertop.jpg", "lightertop");
	BindGLTextures();
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Wooden Floor
	OBJECT_MATERIAL floors;
	floors.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f); // Wood color
	floors.specularColor = glm::vec3(0.2f, 0.1f, 0.0f); // Slight specular reflection
	floors.shininess = 20.0; // Semi-glossy
	floors.tag = "floors";
	m_objectMaterials.push_back(floors);

	// wall
	OBJECT_MATERIAL whiteWallMaterial;
	whiteWallMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);      // Pure white ambient color
	whiteWallMaterial.ambientStrength = 0.5f;                          // Increased ambient strength for a brighter look
	whiteWallMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);      // Pure white diffuse color
	whiteWallMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);     // High specular reflection for a bright, reflective surface
	whiteWallMaterial.shininess = 64.0f;                                // Increased shininess for a more polished look
	whiteWallMaterial.tag = "wall";                                // Tag to identify the material
	m_objectMaterials.push_back(whiteWallMaterial);                     // Add to materials list                    // Add to materials list


	// Orange
	OBJECT_MATERIAL oranges;
	oranges.diffuseColor = glm::vec4(1.0f, 0.65f, 0.0f, 0.8f); // Bright orange with 20% transparency
	oranges.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // Matte finish
	oranges.shininess = 0.0; // Matte finish
	oranges.tag = "oranges2";
	m_objectMaterials.push_back(oranges);

	// Leaf on Orange
	OBJECT_MATERIAL leafs;
	leafs.diffuseColor = glm::vec4(0.0f, 0.6f, 0.0f, 0.9f); // Leaf green with 10% transparency
	leafs.specularColor = glm::vec3(0.0f, 0.2f, 0.0f); // Slight specular reflection
	leafs.shininess = 10.0; // Slightly glossy
	leafs.tag = "leafs";
	m_objectMaterials.push_back(leafs);

	// Stem on Orange
	OBJECT_MATERIAL stems;
	stems.diffuseColor = glm::vec4(0.3f, 0.2f, 0.1f, 1.0f); // Stem brown, opaque
	stems.specularColor = glm::vec3(0.1f, 0.1f, 0.0f); // Slight specular reflection
	stems.shininess = 10.0; // Slightly glossy
	stems.tag = "stems";
	m_objectMaterials.push_back(stems);

	// Sticker on Orange
	OBJECT_MATERIAL stickers;
	stickers.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f); // White sticker with 40% transparency
	stickers.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // Matte finish
	stickers.shininess = 0.0; // Matte finish
	stickers.tag = "stickers";
	m_objectMaterials.push_back(stickers);

	// Neon Green Lighter Plastic
	OBJECT_MATERIAL lighters;
	lighters.diffuseColor = glm::vec4(0.0f, 1.0f, 0.0f, 0.8f); // Neon green with 20% transparency
	lighters.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // Shiny
	lighters.shininess = 50.0; // Glossy
	lighters.tag = "lighters";
	m_objectMaterials.push_back(lighters);

	// Ceramic Green Cup
	OBJECT_MATERIAL cups;
	cups.diffuseColor = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f); // Green ceramic, opaque
	cups.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Slightly shiny
	cups.shininess = 30.0; // Glossy
	cups.tag = "cups";
	m_objectMaterials.push_back(cups);

	// Clear Plastic on Water Bottle
	OBJECT_MATERIAL plastic;
	plastic.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f); // Clear plastic with 10% transparency
	plastic.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Very shiny
	plastic.shininess = 200.0f; // Very shiny surface
	plastic.tag = "plastic";
	m_objectMaterials.push_back(plastic);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL redMaterial;
	redMaterial.ambientColor = glm::vec3(1.0f, 0.0f, 0.0f); // Bright red ambient color
	redMaterial.ambientStrength = 0.8f; // Strong ambient light
	redMaterial.diffuseColor = glm::vec3(1.0f, 0.1f, 0.1f); // Very bright red diffuse color
	redMaterial.specularColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red specular highlights
	redMaterial.shininess = 0.05f; // Low shininess for a rough look
	redMaterial.tag = "red";
	m_objectMaterials.push_back(redMaterial);

	


}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light to emulate sunlight coming from the right side
	m_pShaderManager->setVec3Value("directionalLight.direction", -1.0f, -0.2f, 0.0f); // Adjust direction to come from the right
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.2f, 0.2f, 0.2f); // Brighter ambient light
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 1.0f, 1.0f); // Bright diffuse light
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.5f, 0.5f, 0.5f); // Increased specular reflection
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f); // Position on the left
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.1f); // Slightly brighter ambient light
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.6f); // Brighter diffuse light
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.3f, 0.3f, 0.3f); // Increased specular reflection
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f); // Position on the right
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.2f, 0.2f, 0.2f); // Brighter ambient light on the right side
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 1.0f, 1.0f, 1.0f); // Bright diffuse light
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.5f, 0.5f); // Increased specular reflection
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

}



/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;          // Vector to store scale factors along X, Y, Z axes
	float XrotationDegrees = 0.0f; // Rotation angle around the X axis in degrees
	float YrotationDegrees = 0.0f; // Rotation angle around the Y axis in degrees
	float ZrotationDegrees = 0.0f; // Rotation angle around the Z axis in degrees
	glm::vec3 positionXYZ;       // Vector to store position coordinates in X, Y, Z axes

	// Render the floor
		glm::vec3 floorScale = glm::vec3(20.0f, 1.0f, 10.0f);   // Scale factors for X, Y, Z
	glm::vec3 floorPosition = glm::vec3(0.0f, 0.0f, 0.0f);   // Position at the origin
	SetTransformations(floorScale, 0.0f, 0.0f, 0.0f, floorPosition);
	SetShaderMaterial("wood");
	SetShaderTexture("floor");
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                       // Shininess
	m_basicMeshes->DrawPlaneMesh();

	// Render the Background
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);   // Set scale factors for X, Y, Z
	XrotationDegrees = 90.0f;                   // Rotate 90 degrees around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f); // Position in the scene
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("background"); 
	SetShaderMaterial("wood");// Set texture for the background
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                       // Shininess
	m_basicMeshes->DrawPlaneMesh();             // Draw the vertical plane mesh                

	// Orange
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);     // Set scale factors for X, Y, Z (two times smaller)
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(5.0f, 1.75f, -3.0f); // Position in the scene
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("orange");
	SetShaderMaterial("oranges2");// Set texture for the orange
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.9f, 0.4f, 0.0f));   // Set orange ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.9f, 0.4f, 0.0f));   // Set orange diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.2f, 0.2f, 0.2f));  // Set low specular for matte look
	m_pShaderManager->setFloatValue("material.shininess", 16.0f);                      // Set orange shininess
	m_basicMeshes->DrawSphereMesh();            // Draw the orange (sphere mesh)

	// Leaf on orange
	scaleXYZ = glm::vec3(.2f, 0.2f, 0.6f);     // Set scale factors for X, Y, Z (make leaf more prominent)
	XrotationDegrees = 45.0f;                   // Rotate 45 degrees around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(5.0f, 4.25f, -3.0f); // Position on top of the orange
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf"); 
	SetShaderMaterial("leafs");// Set texture for the leaf
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.0f, 0.5f, 0.0f));   // Set leaf ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.0f, 0.8f, 0.0f));   // Set leaf diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));  // Set low specular for leaf
	m_pShaderManager->setFloatValue("material.shininess", 16.0f);                      // Set leaf shininess
	m_basicMeshes->DrawBoxMesh();               // Draw the leaf (box mesh)

	// Stem on orange
	scaleXYZ = glm::vec3(0.1f, 0.5f, 0.2f);     // Set scale factors for X, Y, Z (adjusted for smaller and more realistic stem)
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(5.0f, 3.75f, -3.0f); // Position on top of the orange
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stem");
	SetShaderMaterial("stems"); // Set texture for the stem
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.3f, 0.2f, 0.1f));   // Set stem ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.4f, 0.3f, 0.2f));   // Set stem diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));  // Set low specular for stem
	m_pShaderManager->setFloatValue("material.shininess", 16.0f);                      // Set stem shininess
	m_basicMeshes->DrawCylinderMesh();          // Draw the stem (cylinder mesh)

	// Sticker on orange
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.01f);    // Set scale factors for X, Y, Z (make it small and thin)
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(5.0f, 2.01f, -3.0f); // Position on top of the orange
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("sticker");
	SetShaderMaterial("stickers"); // Set texture for the sticker
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // Set sticker ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // Set sticker diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));  // Set low specular for sticker
	m_pShaderManager->setFloatValue("material.shininess", 16.0f);                      // Set sticker shininess
	m_basicMeshes->DrawCylinderMesh();          // Draw the sticker (cylinder mesh)

	// Lime green lighter 
	scaleXYZ = glm::vec3(0.5f, 0.50f, 1.50f);     // Set scale factors for X, Y, Z (adjusted for cylindrical shape)
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 90.0f;                   // Rotate 90 degrees around Z axis to lay flat
	positionXYZ = glm::vec3(10.0f, 0.28f, -3.0f); // New position of the lighter (swapped with water bottle)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("lighter");
	SetShaderMaterial("lighters"); // Set texture for the lighter
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.7f, 1.0f, 0.7f));   // Set lighter ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.8f, 1.0f, 0.8f));   // Set lighter diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.9f, 0.9f, 0.9f));  // Set high specular for shininess
	m_pShaderManager->setFloatValue("material.shininess", 128.0f);                     // Set lighter shininess
	m_basicMeshes->DrawCylinderMesh();          // Draw the lighter (cylinder mesh)
	
	// Create a white bottom piece for the lighter
	glm::vec3 whiteBottomScale = glm::vec3(0.5f, 0.5f, 0.4f); // Thinner and shorter cylinder for the bottom
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 90.0f; 
	glm::vec3 whiteBottomPosition = glm::vec3(10.0f, 0.20f, -2.0f);
	SetTransformations(whiteBottomScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, whiteBottomPosition);                 
	SetShaderTexture("white"); // Ensure this texture is specifically for the white bottom
	m_basicMeshes->DrawCylinderMesh();             // Set shininess
 
	// Render a small red box
	glm::vec3 redBoxScale = glm::vec3(0.40f, 0.5f, 0.70f); // Thicker box // Thicker and shorter box dimensions
	XrotationDegrees = 0.0f;                         // No rotation around X axis
	YrotationDegrees = 0.0f;                         // No rotation around Y axis
	ZrotationDegrees = 0.0f;                         // No rotation around Z axis
	glm::vec3 redBoxPosition = glm::vec3(9.65f, 0.38f, -3.90f); // Position in the scene
	SetTransformations(redBoxScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, redBoxPosition);
	SetShaderTexture("lightertop");
	SetShaderMaterial("red"); // Set texture for the lighter
	m_pShaderManager->setVec4Value("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Bright red color
	m_pShaderManager->setVec3Value("specularColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Shiny specular color
	m_basicMeshes->DrawBoxMesh(); // Draw the box

	// Green ceramic cup
	scaleXYZ = glm::vec3(2.0f, 3.75f, 1.0f);     // Set scale factors for X, Y, Z
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(-9.0f, -1.0f, -3.0f); // Position of the cup
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("cup");
	SetShaderMaterial("cups");                  // Set texture for the cup
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.0f, 0.5f, 0.0f));   // Set cup ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.1f, 0.8f, 0.1f));   // Set cup diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.6f, 0.6f, 0.6f));  // Set cup specular for glossiness
	m_pShaderManager->setFloatValue("material.shininess", 128.0f);                     // Set cup shininess
	m_basicMeshes->DrawCylinderMesh();          // Draw the cup (cylinder mesh)

	// Cup handle
	glm::vec3 handleScale = glm::vec3(0.80f, 1.20f, 0.2f); // Adjust to match the cup size and desired handle thickness
    glm::vec3 handlePosition = glm::vec3(-10.50f, 1.50f, -2.25f); // Adjust X to move it beside the cup
	SetTransformations(handleScale, 0.0f, 0.0f, 90.0f, handlePosition); // Rotate to align with the cup
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(0.3f, 0.6f, 0.3f));   // Green ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(0.4f, 0.8f, 0.4f));   // Green diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(0.6f, 0.9f, 0.6f));  // Green specular color
	m_pShaderManager->setFloatValue("material.shininess", 64.0f);                     // Adjust shininess
	m_basicMeshes->DrawTorusMesh(); // Draw the handle

	// label on cup
	glm::vec3 labelScale = glm::vec3(0.80f, 0.50f, 01.0f); // Adjust X and Z to match desired size, Y for flatness
	glm::vec3 labelPosition = glm::vec3(-8.25f, 1.25f, -1.5f); // Position to align with the side of the cup
	SetTransformations(labelScale, 90.0f, 0.0f, 0.0f, labelPosition); // Rotate around Y-axis to align with the cup's side
	SetShaderTexture("cuplabel");
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                     // Adjust shininess
	m_basicMeshes->DrawTaperedCylinderMesh(); // Draw the flat circular label

	// Water bottle
	scaleXYZ = glm::vec3(1.0f, 6.0f, 0.50f);     // Set scale factors for X, Y, Z (adjusted for bottle shape)
	// Adjust width to match sphere's diameter
	XrotationDegrees = 0.0f;                    // No rotation around X axis
	YrotationDegrees = 0.0f;                    // No rotation around Y axis
	ZrotationDegrees = 0.0f;                    // No rotation around Z axis
	positionXYZ = glm::vec3(-2.0f, -1.25f, -3.0f); // Position of the water bottle
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("waterbottle");
	SetShaderMaterial("plastic"); // Set texture for the water bottle
	m_basicMeshes->DrawCylinderMesh();          // Draw the water bottle (cylinder mesh)

	// Round top on waterbottle
	glm::vec3 domeScale = glm::vec3(1.0f, 0.75f, 0.5f); // Scale Z-axis to create a dome effect
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	glm::vec3 domePosition = glm::vec3(-2.0f, -1.25f + 6.0f, -3.0f); // Position to align with the water bottle
	SetTransformations(domeScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, domePosition);
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                     // Adjust shininess
	m_basicMeshes->DrawSphereMesh();

	//water bottle cap
	glm::vec3 capScale = glm::vec3(0.4f, 0.4f, 0.05f); // Small, thin cylinder for the cap
	XrotationDegrees = 0.0f;  // No rotation around X axis
	YrotationDegrees = 0.0f;  // No rotation around Y axis
	ZrotationDegrees = 0.0f;  // No rotation around Z axis
	glm::vec3 capPosition = glm::vec3(-2.0f, -1.25f + 6.0f + 0.75f, -3.0f); // Position to sit on top of the water bottle and move up
	SetTransformations(capScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, capPosition);
	SetShaderTexture("thecap"); // Set the texture for the cap (assuming it's white)
	SetShaderMaterial("plastic"); // Ensure the material is set if needed
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                     // Adjust shininess
	m_basicMeshes->DrawCylinderMesh();  // Draw the cap using the cylinder mesh

	//waterbottle label
	glm::vec3 boxScale = glm::vec3 (2.0f, 1.60f, 0.1f); // Width matches the bottle, height is appropriate for a label
	glm::vec3 boxPosition = glm::vec3(-2.0f + 0.20f, -2.25f + 3.0f + 2.0f, -2.50f); // Adjust X to move right
	SetTransformations(boxScale, 0.0f, 0.0f, 0.0f, boxPosition);
	SetShaderTexture("thelabel"); // Ensure "thelabel" texture is loaded and applied
	m_pShaderManager->setVec3Value("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));   // White ambient color
	m_pShaderManager->setVec3Value("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));   // White diffuse color
	m_pShaderManager->setVec3Value("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // White specular color
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);                     // Adjust shininess
	m_basicMeshes->DrawBoxMesh();

}

