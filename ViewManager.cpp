///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000; // Define window width
	const int WINDOW_HEIGHT = 800; // Define window height
	const char* g_ViewName = "view"; // Uniform name for view matrix
	const char* g_ProjectionName = "projection"; // Uniform name for projection matrix

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr; // Pointer to the camera object

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f; // Last mouse X position
	float gLastY = WINDOW_HEIGHT / 2.0f; // Last mouse Y position
	bool gFirstMouse = true; // Flag for the first mouse movement

	// time between current frame and last frame
	float gDeltaTime = 0.0f; // Time difference between frames
	float gLastFrame = 0.0f; // Time of the last frame

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false; // Flag for projection mode
}

/***********************************************************
 * ViewManager()
 *
 * The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL; // Initialize window pointer
	g_pCamera = new Camera(); // Create a new camera object
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f); // Initial camera position
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f); // Initial camera front direction
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f); // Initial camera up direction
	g_pCamera->Zoom = 80; // Initial camera zoom (FOV)
	g_pCamera->MovementSpeed = 20; // Initial camera movement speed
}

/***********************************************************
 * ~ViewManager()
 *
 * The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera; // Delete the camera object
		g_pCamera = NULL;
	}
}

/***********************************************************
 * CreateDisplayWindow()
 *
 * This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// this callback is used to receive mouse scroll events
	glfwSetScrollCallback(window, &ViewManager::Mouse_Wheel_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window; // Store the window pointer

	return(window);
}

/***********************************************************
 * Mouse_Position_Callback()
 *
 * This method is automatically called from GLFW whenever
 * the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 * Mouse_Wheel_Callback()
 *
 * This method is automatically called from GLFW whenever
 * the mouse wheel is scrolled.
 ***********************************************************/
void ViewManager::Mouse_Wheel_Callback(GLFWwindow* window, double x, double yScrollDistance)
{
	//call the camera class to process the mouse wheel scrolling
	g_pCamera->ProcessMouseScroll(yScrollDistance);

}

/***********************************************************
 * ProcessKeyboardEvents()
 *
 * This method is called to process any keyboard events
 * that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out (Forward and Backward movement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right (Strafe movement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// process camera up and down motion (Vertical movement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// switch between perspective and orthographic projections
	// Use glfwGetKey with a check for GLFW_RELEASE to trigger only once per key press
	static bool p_pressed = false;
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS && !p_pressed)
	{
		bOrthographicProjection = false; // Switch to Perspective
		p_pressed = true;
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE)
	{
		p_pressed = false;
	}

	static bool o_pressed = false;
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS && !o_pressed)
	{
		bOrthographicProjection = true; // Switch to Orthographic
		o_pressed = true;
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE)
	{
		o_pressed = false;
	}
}

/***********************************************************
 * PrepareSceneView()
 *
 * This method is used for preparing the 3D scene by loading
 * the shapes, textures in memory to support the 3D scene
 * rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	if (bOrthographicProjection)
	{
		float scale = 6.0f; // Scale for orthographic projection
		// Adjust aspect ratio for orthographic projection
		float aspectRatio = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
		projection = glm::ortho(-scale * aspectRatio, scale * aspectRatio, -scale, scale, 0.1f, 100.0f);
	}
	else
	{
		// Perspective projection
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
