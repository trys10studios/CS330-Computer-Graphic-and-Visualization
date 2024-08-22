///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
	// Textures needed to map to 3D objects RW
	bool bReturn = false;
	bReturn = CreateGLTexture("Textures/BurgerTexture.jpg", "BurgerTexture");
	bReturn = CreateGLTexture("Textures/BurgerBottomBun.jpg", "BurgerBottomBun");
	bReturn = CreateGLTexture("Textures/SesameSeedTexture.jpg", "SesameSeedTexture");
	bReturn = CreateGLTexture("Textures/CupTexture.jpg", "CupTexture");
	bReturn = CreateGLTexture("Textures/TableTexture.jpg", "TableTexture");
	bReturn = CreateGLTexture("Textures/StrawTexture.jpg", "StrawTexture");
	bReturn = CreateGLTexture("Textures/LidTexture.jpg", "LidTexture");
	bReturn = CreateGLTexture("Textures/FryTexture.jpg", "FryTexture");
	bReturn = CreateGLTexture("Textures/CakeTexture.jpg", "CakeTexture");
	bReturn = CreateGLTexture("Textures/CakeTopTexture.jpg", "CakeTopTexture");
	bReturn = CreateGLTexture("Textures/StrawCapTexture.jpg", "StrawCapTexture");
	bReturn = CreateGLTexture("Textures/FloorTexture.jpg", "FloorTexture");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
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
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL bunMaterial;
	bunMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	bunMaterial.ambientStrength = 0.1f;
	bunMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	bunMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	bunMaterial.shininess = 0.1f;
	bunMaterial.tag = "bun";

	m_objectMaterials.push_back(bunMaterial);

	OBJECT_MATERIAL cheeseburgerMaterial;
	cheeseburgerMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
	cheeseburgerMaterial.ambientStrength = 0.05f;
	cheeseburgerMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	cheeseburgerMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseburgerMaterial.shininess = 0.1f;
	cheeseburgerMaterial.tag = "cheeseburger";

	m_objectMaterials.push_back(cheeseburgerMaterial);

	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	tableMaterial.ambientStrength = 0.5f;
	tableMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	tableMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	tableMaterial.shininess = 50.0;
	tableMaterial.tag = "table";

	m_objectMaterials.push_back(tableMaterial);

	OBJECT_MATERIAL strawMaterial;
	strawMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	strawMaterial.ambientStrength = 0.5f;
	strawMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	strawMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	strawMaterial.shininess = 50.0;
	strawMaterial.tag = "straw";

	m_objectMaterials.push_back(strawMaterial);

	OBJECT_MATERIAL cupMaterial;
	cupMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	cupMaterial.ambientStrength = 0.5f;
	cupMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	cupMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	cupMaterial.shininess = 50.0;
	cupMaterial.tag = "cup";

	m_objectMaterials.push_back(cupMaterial);

	OBJECT_MATERIAL lidMaterial;
	lidMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	lidMaterial.ambientStrength = 0.5f;
	lidMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	lidMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	lidMaterial.shininess = 50.0;
	lidMaterial.tag = "lid";

	m_objectMaterials.push_back(lidMaterial);

	OBJECT_MATERIAL fryMaterial;
	fryMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	fryMaterial.ambientStrength = 0.5f;
	fryMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	fryMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	fryMaterial.shininess = 50.0;
	fryMaterial.tag = "fry";

	m_objectMaterials.push_back(fryMaterial);

	OBJECT_MATERIAL fryBoxMaterial;
	fryBoxMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	fryBoxMaterial.ambientStrength = 0.5f;
	fryBoxMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	fryBoxMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	fryBoxMaterial.shininess = 50.0;
	fryBoxMaterial.tag = "fryBox";

	m_objectMaterials.push_back(fryBoxMaterial);

	OBJECT_MATERIAL cakeMaterial;
	cakeMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	cakeMaterial.ambientStrength = 0.5f;
	cakeMaterial.diffuseColor = glm::vec3(0.96f, 0.70f, 0.50f);
	cakeMaterial.specularColor = glm::vec3(0.1f, 0.5f, 0.1f);
	cakeMaterial.shininess = 50.0;
	cakeMaterial.tag = "cake";

	m_objectMaterials.push_back(cakeMaterial);

	OBJECT_MATERIAL strawCapMaterial;
	strawCapMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	strawCapMaterial.ambientStrength = 0.1f;
	strawCapMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	strawCapMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	strawCapMaterial.shininess = 0.1f;
	strawCapMaterial.tag = "strawCap";

	m_objectMaterials.push_back(strawCapMaterial);

	OBJECT_MATERIAL tableSupportMaterial;
	tableSupportMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	tableSupportMaterial.ambientStrength = 0.1f;
	tableSupportMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	tableSupportMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	tableSupportMaterial.shininess = 50.0f;
	tableSupportMaterial.tag = "tableSupport";

	m_objectMaterials.push_back(tableSupportMaterial);

	OBJECT_MATERIAL floorMaterial;
	floorMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	floorMaterial.ambientStrength = 0.1f;
	floorMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	floorMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	floorMaterial.shininess = 50.0f;
	floorMaterial.tag = "floor";

	m_objectMaterials.push_back(floorMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/



	m_pShaderManager->setVec3Value("lightSources[2].direction", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.1f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
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
	// load the textures for the 3D scene RW
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***********************************************************
	
	// FLOOR

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -20.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale to tile the texture RW
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderTexture("FloorTexture"); 
	SetShaderMaterial("floor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // Plane (Floor)

	/****************************************************************/

	/***********************************************************

	// TABLE

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(15.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("TableTexture");
	SetShaderMaterial("table");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // Box (Table Top)

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 20.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -20.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("LidTexture"); // Table Support shares texture with lid
	SetShaderMaterial("tableSupport");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // Cylinder (Table Support)

	/****************************************************************/

	// CHEESEBURGER

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 1.75f, 2.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 270.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("SesameSeedTexture");
	SetShaderMaterial("bun");

	// Set the UV scale to tile the texture RW
	SetTextureUVScale(5.0f, 5.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();  // Sphere (Burger top)

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 1.25f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 270.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("BurgerTexture"); // RW
	SetShaderMaterial("cheeseburger");
	// Set the UV scale of the texture RW
	SetTextureUVScale(2.0, 0.9);

	m_basicMeshes->DrawSphereMesh();  // Sphere (Burger Patty)

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.25f, 2.2f, 2.52f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 270.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("BurgerBottomBun");
	SetShaderMaterial("bun");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();  // Tapered Cylinder (Burger Bottom)

	/****************************************************************/

	// SODA CUP

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 5.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // RW
	SetShaderMaterial("cup");
	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();  // Cylinder (Cup)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 0.5f, 1.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 5.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("LidTexture"); // RW
	SetShaderMaterial("lid");
	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();  // Cylinder (Lid)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 00.0f;
	YrotationDegrees = 225.0f;
	ZrotationDegrees = 10.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.5f, 2.1f, -0.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("StrawTexture"); // RW
	SetShaderMaterial("straw");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawTaperedCylinderMesh();  // Cylinder (Straw Bottom)

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 1.25f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 170.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.73f, 7.0f, -0.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("StrawTexture");
	SetShaderMaterial("straw");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();  // Cylinder (Straw Top)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.1f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.725f, 7.0f, -0.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("StrawCapTexture"); // RW
	SetShaderMaterial("strawCap");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();  // Cylinder (Straw End Cap)

	/****************************************************************/

	// FRIES

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	float scalar = 1.25f; // used as a scalar for the fry box RW

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 0.1f) * scalar;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.75f, 1.25f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("cup");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry Box 1, Front)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 0.1f) * scalar;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.02f, 1.25f, -2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("cup");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry Box 2, Back)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 2.0f, 0.75f) * scalar;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.035f, 1.25f, -1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("cup");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry Box 3, Right Side)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 2.0f, 0.75f) * scalar;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.735f, 1.25f, -2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("cup");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry Box 4, Left Side)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 0.1f, 0.8f) * scalar;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.8f, 0.25f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CupTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("cup");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry Box 5, Bottom Side)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 3.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 5.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 10.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.1f, 2.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("FryTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("fry");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry 1)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 3.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 5.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 5.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 2.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("FryTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("fry");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry 2)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 3.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 5.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 5.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 2.75f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("FryTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("fry");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry 3)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 2.75f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 3.0f, -1.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("FryTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("fry");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry 4)

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 4.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 5.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 2.5f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("FryTexture"); // Frybox shares texture with the cup RW
	SetShaderMaterial("fry");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // Box (Fry 5)

	/******************************************************************/

	// PIECE OF CAKE

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 170.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 1.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CakeTexture");
	SetShaderMaterial("cake");

	// Set the UV scale of the texture RW
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();  // Prism (Piece of Cake)

	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.01f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 170.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 2.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("CakeTopTexture");
	SetShaderMaterial("cake");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();  // Prism (Icing for Piece of Cake)
}


