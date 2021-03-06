#include <Windows.h>
#include <Ole2.h>

#include <gl/GL.h>
#include <gl/GLU.h>
#include <GL/glut.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>

// input to KINECT SDK calls to change the resolution 
// please change the preprocessor definitions here
#define WIDTH 640
#define HEIGHT 480

// OpenGL Variables
GLuint textureId;             // ID that contains the texture to contain RGB data
GLubyte data[WIDTH*HEIGHT*4]; // BGRA array that contains the texture data
int screen_width, screen_height;

//Kinect variables
HANDLE rgbStream;             // Identifier of Kinect's RGB Camera
INuiSensor *sensor;           // Kinect Sensor

// Kinect initialization

HRESULT initKinect()
{
  // Getting a working Kinect sesor
	int numSensors;
	HRESULT hr = NuiGetSensorCount(&numSensors);
	if(FAILED(hr))		return hr;

	// identifying the sensor (We are assuming that there is only one sensor)
	hr = NuiCreateSensorByIndex(0,&sensor);
	if(FAILED(hr))		return hr;

	// Initialize sensor
	hr = sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
	if(FAILED(hr))		return hr;

	// Specify the color stream properties
	hr = sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,
		NUI_IMAGE_RESOLUTION_640x480,
		0,
		2,
		NULL,
		&rgbStream);
	if(FAILED(hr))		return hr;

	return S_OK;
}

// Getting RGB frame from Kinect

void getKinectData(GLubyte* dest)
{
	NUI_IMAGE_FRAME imageFrame; // structure containing all the metadata about the frame
	NUI_LOCKED_RECT LockedRect; // contains the pointer to the actual data

	if(FAILED(sensor->NuiImageStreamGetNextFrame(rgbStream,0,&imageFrame)))
		return;

	INuiFrameTexture *texture = imageFrame.pFrameTexture;
	texture->LockRect(0,&LockedRect,NULL,0);

	// Now copy the data to our own memory location
	if(LockedRect.Pitch != 0)
	{
		const BYTE* curr = (const BYTE*) LockedRect.pBits;

		// copy the texture contents from current to destination
		memcpy( dest, curr, sizeof(BYTE)*(WIDTH*HEIGHT*4) );
	}

	texture->UnlockRect(0);
	if(FAILED(sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame)))
		return;
}

void display()
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	getKinectData(data);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,WIDTH,HEIGHT,GL_BGRA_EXT,GL_UNSIGNED_BYTE,(void*)data);
	
	// mapping the texture on the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);		glVertex3f(0.0,0.0,0.0);
		glTexCoord2f(1.0f, 0.0f);       glVertex3f(WIDTH,0.0,0.0);
		glTexCoord2f(1.0f, 1.0f);       glVertex3f(WIDTH,HEIGHT,0.0);
		glTexCoord2f(0.0f, 1.0f);       glVertex3f(0.0,HEIGHT,0.0);
	glEnd();

	glutSwapBuffers();
}

void myInit()
{
	glClearColor(0.0,0.0,0.0,0.0);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST); // enable depth test to avoid z-buffer fighting

	// Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)data);
	glBindTexture(GL_TEXTURE_2D,0);

	// Enable the texture map
	glEnable(GL_TEXTURE_2D);

	// Camera setup
	glViewport(0.0,0.0,WIDTH,HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WIDTH, HEIGHT, 0, 1, -1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void doIdle()
{
	glutPostRedisplay(); // calls the display function
}

int main(int argc, char* argv[])
{
	// Check for Kinect
	HRESULT hr;
    if ((hr = initKinect()) != S_OK) return 1;

	glutInit(&argc,(char**)argv);
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );
	glutInitWindowSize(WIDTH,HEIGHT);
	glutInitWindowPosition(100,100);
	glutCreateWindow("ColorStream-OpenGL");

	// tells glut to use a particular display function to redraw
	glutDisplayFunc(display);

	// Needs an idle function to reinitiate the draw function
	glutIdleFunc(doIdle);

	// initialize the window
	myInit();

	glutMainLoop();
	return 0;
}
