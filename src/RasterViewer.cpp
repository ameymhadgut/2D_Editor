#include "SDLViewer.h"

#include <Eigen/Core>
#include <Eigen/Geometry> 

#include <functional>
#include <iostream>
#include <dos.h> 
#include <windows.h>
#include <math.h>
#include <string>

#include "raster.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"
using namespace Eigen;

/* Wrapper class to store types of modes and keys */
class EditorMode {
public:
    const static char INSERTION_MODE_KEY = 'i', TRANSLATION_MODE_KEY = 'o', DELETE_MODE_KEY = 'p', COLOR_MODE_KEY = 'c', ANIMATION_MODE_KEY = 'm';
    const static char SCALE_UP = 'k', SCALE_DOWN = 'l', ROTATE_CLOCKWISE = 'h', ROTATE_COUNTERCLOCKWISE = 'j';
    const static char PAN_DOWN_KEY = 'w', PAN_UP_KEY = 's', PAN_LEFT_KEY = 'd', PAN_RIGHT_KEY = 'a', ZOOM_IN_KEY = 'W', ZOOM_OUT_KEY = 'V';
};

/* Color Constants */
const static Vector4f RED = Vector4f(1, 0, 0, 1);
const static Vector4f GREEN = Vector4f(0, 1, 0, 1);
const static Vector4f BLUE = Vector4f(0, 0, 1, 1);
const static Vector4f HIGHLIGHT = Vector4f(0.5, 0.8, 0, 1);

/*String Constants */
const static std::string WELCOME_MSG = "Welcome to 2D Editor. \nThe mode & respective keys are:\n 1. Insertion Mode = i \n 2. Translation Mode = o \n 3. Delete Mode = p \n 4. Color Mode = c  \n 5. Animation Mode = m\n******************************\nYou are currently in insertion mode. Please select vertices to draw a triangle.\n******************************\n";
const static std::string INSERTION_MODE_MSG = "You are in Insertion Mode.\n";
const static std::string TRANSLATION_MODE_MSG = "\nYou are in Translation Mode.\n You can: \n 1. Use cursor to move triangles\n 2. Scale up = k\n 3. Scale down = l\n 4. Rotate clockwise = h\n 5. Rotate anti-clockwise\n";
const static std::string DELETION_MODE_MSG = "\nYou are in Deletion Mode. Click on a triangle to delete. \n";
const static std::string COLOR_MODE_MSG = "\nYou are in Color Mode. Click inside a triangle to color the closest vertex. \n";
const static std::string ANIMATION_MODE_MSG = "\nYou are in Animation Mode.\n Now, you should move the triangle to whichever position you like. \n Use key 'n' for linear interpolation animation\n Use key 'b' for Beizer Curve Interpolation animation\n";
const static std::string ZOOM_OUT_MSG = "\nZooming Out";
const static std::string ZOOM_IN_MSG = "\nZooming In";
const static std::string PAN_DOWN_MSG = "\nPanning down";
const static std::string PAN_UP_MSG = "\nPanning up";
const static std::string PAN_RIGHT_MSG = "\nPanning right";
const static std::string PAN_LEFT_MSG = "\nPanning left";

/* Identity Matrix constant */
const static Matrix4f identity = Matrix4f::Identity();

/* Enum to store Editor Mode*/
enum Mode { INSERTION_MODE, TRANSLATION_MODE, DELETION_MODE, COLOR_MODE };

/* Method to print string message */
void printMessage(std::string message) {
    std::cout << message;
}

/* Method to print vector while debugging */
void printVector(Vector4f& v) {
    std::cout << "\n[" << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << "]";
}

/* Method to set color */
void setColor(VertexAttributes& v1, VertexAttributes& v2, VertexAttributes& v3, Vector4f color) {
    v1.color = color;
    v2.color = color;
    v3.color = color;
}

/* Method to get 4f to 3d Vector */
Vector3d get3DPositionVector(Vector4f& temp) {
    return Vector3d(temp.x(), temp.y(), temp.z());
}

/* Method to get index of selected triangle based on clicked position */
int getSelectedTriangleIndex(std::vector<VertexAttributes>& triangles, UniformAttributes& uniform, double x_pos, double y_pos) {
    for (unsigned i = 0; i + 2 < triangles.size(); i += 3) {
        Vector4f v1_pos = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[i].position;
        Vector4f v2_pos = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[i + 1].position;
        Vector4f v3_pos = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[i + 2].position;

        Vector3d v1 = get3DPositionVector(v1_pos);
        Vector3d v2 = get3DPositionVector(v2_pos);
        Vector3d v3 = get3DPositionVector(v3_pos);

        AlignedBox3d box;
        box = box.extend(v1);
        box = box.extend(v2);
        box = box.extend(v3);
        if (box.contains(Vector3d(x_pos, y_pos, 0))) {
            return i;
        }
    }
    return -1;
}

/* Method to delete triangle based on click position */
void deleteTriangle(std::vector<VertexAttributes>& triangles, UniformAttributes& uniform, double x_pos, double y_pos, SDLViewer& viewer) {
    int index = getSelectedTriangleIndex(triangles, uniform, x_pos, y_pos);
    if (index >= 0) {
        triangles.erase(triangles.begin() + index, triangles.begin() + index + 3);
        uniform.view << identity;
        uniform.translate << identity;
        uniform.rotate << identity;
        uniform.scale << identity;
        uniform.scale_factor = 1.00;
        uniform.rotate_radians = 0.0;
        uniform.translate_delta = Vector4f(0.0, 0.0, 0.0, 0.0);
        viewer.redraw_next = true;
    }
}

/* Method to set current editor mode */
void setCurrentMode(char key, Mode& currentMode) {
    if (key == EditorMode::INSERTION_MODE_KEY) { //Insertion Mode
        printMessage(INSERTION_MODE_MSG);
        currentMode = INSERTION_MODE;
    }
    else if (key == EditorMode::TRANSLATION_MODE_KEY) { //Translation Mode
        printMessage(TRANSLATION_MODE_MSG);
        currentMode = TRANSLATION_MODE;
    }
    else if (key == EditorMode::DELETE_MODE_KEY) { //Deletion Mode
        printMessage(DELETION_MODE_MSG);
        currentMode = DELETION_MODE;
    }
    else if (key == EditorMode::COLOR_MODE_KEY) { //Color Mode
        printMessage(COLOR_MODE_MSG);
        currentMode = COLOR_MODE;
    }
    else if (key == EditorMode::ANIMATION_MODE_KEY) { //Animation Mode
        printMessage(ANIMATION_MODE_MSG);
        currentMode = TRANSLATION_MODE;
    }
}

/* Method to translate selected triangle */
void translateTriangle(VertexAttributes& v1, VertexAttributes& v2, VertexAttributes& v3, UniformAttributes& uniform) {
    v1.selected = true;
    v2.selected = true;
    v3.selected = true;
    Matrix4f currentTransform;
    currentTransform <<     1, 0, 0, uniform.translate_delta.x(),
                            0, 1, 0, uniform.translate_delta.y(),
                            0, 0, 1, 0,
                            0, 0, 0, 1;
    uniform.translate = currentTransform * uniform.translate;
}

/* Method to scale selected triangle */
void scaleTriangle(VertexAttributes& v1, VertexAttributes& v2, VertexAttributes& v3, UniformAttributes& uniform) {
    v1.selected = true;
    v2.selected = true;
    v3.selected = true;

    Matrix4f currentTransform;
    currentTransform << uniform.scale_factor, 0, 0, 0,
                        0, uniform.scale_factor, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1;
    uniform.scale = currentTransform * uniform.scale;
}

/* Method to rotate selected triangle */
void rotateTriangle(VertexAttributes& v1, VertexAttributes& v2, VertexAttributes& v3, UniformAttributes& uniform) {
    v1.selected = true;
    v2.selected = true;
    v3.selected = true;
    double cos_rotate_degree = cos(uniform.rotate_radians), sin_rotate_degree = sin(uniform.rotate_radians);
    Matrix4f currentTransform;
    currentTransform <<     cos_rotate_degree, -sin_rotate_degree, 0, 0,
                            sin_rotate_degree, cos_rotate_degree, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 1;
    uniform.rotate = currentTransform * uniform.rotate;
}

/* Method to perform translations triangle */
void performTranslationAction(char key, std::vector<VertexAttributes>& triangles, UniformAttributes& uniform, int triangleToTranslate) {
    switch (key) {
    case EditorMode::SCALE_UP:
        if (uniform.scale_factor < 1)
            uniform.scale_factor = 1;
        uniform.scale_factor += 0.25;
        uniform.mode = EditorMode::SCALE_UP;
        scaleTriangle(triangles[triangleToTranslate], triangles[triangleToTranslate + 1], triangles[triangleToTranslate + 2], uniform);
        break;
    case EditorMode::SCALE_DOWN:
        if (uniform.scale_factor > 1)
            uniform.scale_factor = 1;
        uniform.scale_factor -= 0.25;
        uniform.mode = EditorMode::SCALE_DOWN;
        scaleTriangle(triangles[triangleToTranslate], triangles[triangleToTranslate + 1], triangles[triangleToTranslate + 2], uniform);
        break;
    case EditorMode::ROTATE_CLOCKWISE:
        if (uniform.rotate_radians < 0)
            uniform.rotate_radians = 0;
        uniform.rotate_radians += 0.17453292519;
        uniform.mode = EditorMode::ROTATE_CLOCKWISE;
        rotateTriangle(triangles[triangleToTranslate], triangles[triangleToTranslate + 1], triangles[triangleToTranslate + 2], uniform);
        break;
    case EditorMode::ROTATE_COUNTERCLOCKWISE:
        if (uniform.rotate_radians > 0)
            uniform.rotate_radians = 0;
        uniform.rotate_radians -= 0.17453292519;
        uniform.mode = EditorMode::ROTATE_COUNTERCLOCKWISE;
        rotateTriangle(triangles[triangleToTranslate], triangles[triangleToTranslate + 1], triangles[triangleToTranslate + 2], uniform);
        break;
    }
}

/* Method to get nearest vertex */
int getNearestVertex(std::vector<VertexAttributes>& triangles, UniformAttributes& uniform, int selectedTriangle, Vector4f currPosition) {
    Vector4f v1 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle].position;
    Vector4f v2 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 1].position;
    Vector4f v3 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 2].position;
    std::vector<double> dist = {    (v1 - currPosition).norm(),
                                    (v2 - currPosition).norm(),
                                    (v3 - currPosition).norm()
    };
    return std::min_element(dist.begin(), dist.end()) - dist.begin() + selectedTriangle;
}

/* Method to update viewport */
void changeViewport(char key, float zoom, float delta, UniformAttributes& uniform, SDLViewer& viewer) {
    char temp_key = uniform.mode;
    uniform.mode = key;
    //Zooming & Paning function
    if (key == EditorMode::ZOOM_IN_KEY) {
        printMessage(ZOOM_OUT_MSG);
        if (zoom < 1)
            zoom = 1;
        zoom += 0.2;
        Matrix4f currentView;
        currentView << zoom, 0, 0, 0,
            0, zoom, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else if (key == EditorMode::ZOOM_OUT_KEY) {
        printMessage(ZOOM_IN_MSG);
        if (zoom > 1)
            zoom = 1;
        zoom -= 0.2;
        Matrix4f currentView;
        currentView << zoom, 0, 0, 0,
            0, zoom, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else if (key == EditorMode::PAN_DOWN_KEY) {
        uniform.mode = 'w';
        printMessage(PAN_DOWN_MSG);
        if (delta > 0)
            delta = 0;
        delta -= 0.2;
        Matrix4f currentView;
        currentView << 1, 0, 0, 0,
            0, 1, 0, delta,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else if (key == EditorMode::PAN_UP_KEY) {
        printMessage(PAN_UP_MSG);
        if (delta < 0)
            delta = 0;
        delta += 0.2;
        Matrix4f currentView;
        currentView << 1, 0, 0, 0,
            0, 1, 0, delta,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else if (key == EditorMode::PAN_RIGHT_KEY) {
        printMessage(PAN_RIGHT_MSG);
        if (delta < 0)
            delta = 0;
        delta += 0.2;
        Matrix4f currentView;
        currentView << 1, 0, 0, delta,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else if (key == EditorMode::PAN_LEFT_KEY) {
        printMessage(PAN_LEFT_MSG);
        if (delta > 0)
            delta = 0;
        delta -= 0.2;
        Matrix4f currentView;
        currentView << 1, 0, 0, delta,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;
        uniform.view = currentView * uniform.view;
        viewer.redraw(viewer);
    }
    else {
        uniform.mode = temp_key;
        return;
    }
}

int main(int argc, char *args[])
{
    int width = 500;
    int height = 500;

    // The Framebuffer storing the image rendered by the rasterizer
	Eigen::Matrix<FrameBufferAttributes,Eigen::Dynamic,Eigen::Dynamic> frameBuffer(width, height);

	// Global Constants (empty in this example)
	UniformAttributes uniform;
    uniform.view << identity;
    uniform.translate << identity;
    uniform.rotate << identity;
    uniform.scale << identity;

	// Basic rasterization program
	Program program;

	// The vertex shader is the identity
	program.VertexShader = [](const VertexAttributes& va, const UniformAttributes& uniform)
	{
        
        VertexAttributes v_new = va;
        if (va.selected) {
            if (uniform.mode == EditorMode::ROTATE_CLOCKWISE || uniform.mode == EditorMode::ROTATE_COUNTERCLOCKWISE 
                || uniform.mode == EditorMode::SCALE_DOWN || uniform.mode == EditorMode::SCALE_UP) {
                Matrix4f transOrigin;
                Matrix4f currentTransform;
                currentTransform << 1, 0, 0, -v_new.bary_center.x(),
                                    0, 1, 0, -v_new.bary_center.y(),
                                    0, 0, 1, 0,
                                    0, 0, 0, 1;
                v_new.position = currentTransform * v_new.position;
                v_new.position = uniform.view * uniform.translate * uniform.rotate * uniform.scale * v_new.position;
                Matrix4f currentTransform2;
                currentTransform2 << 1, 0, 0, v_new.bary_center.x(),
                                    0, 1, 0, v_new.bary_center.y(),
                                    0, 0, 1, 0,
                                    0, 0, 0, 1;
                v_new.position = currentTransform2 * v_new.position;
            }
            else {
                v_new.position = uniform.view * uniform.translate * uniform.rotate * uniform.scale * v_new.position;
            }
        }
        else {
            v_new.position = uniform.view * uniform.translate * uniform.rotate * uniform.scale * v_new.position;
        }
		return v_new;
	};

	// The fragment shader uses a fixed color
	program.FragmentShader = [](const VertexAttributes& va, const UniformAttributes& uniform)
	{
		return FragmentAttributes(va.color(0),va.color(1),va.color(2));
	};

	// The blending shader converts colors between 0 and 1 to uint8
	program.BlendingShader = [](const FragmentAttributes& fa, const FrameBufferAttributes& previous)
	{
		return FrameBufferAttributes(fa.color[0]*255,fa.color[1]*255,fa.color[2]*255,fa.color[3]*255);
	};

	// One triangle in the center of the screen
    Mode currentMode = INSERTION_MODE;
    printMessage(WELCOME_MSG);

    // No. of triangles
    unsigned numOfClicks = 0;

    //Translation mode
    int selectedTriangle = -1, prevClickedTriangle = -1;
    Vector4f oldPosition, newPosition;
    bool isClicked = false;
    bool isCursorMoving = false, firstTime = true;
    uniform.scale_factor = 1.00;
    uniform.rotate_radians = 0.0;
    uniform.translate_delta = Vector4f(0.0, 0.0, 0.0, 0.0);

    //Color Mode
    int vertex_index = -1;
    float zoom = 1;
    float delta = 0.0;

    //Animation Mode
    bool animationMode = false, isPositionSet = false;
    Vector4f oldAnimePosition1, oldAnimePosition2, oldAnimePosition3;

    //vector to store complete triangles
    std::vector<VertexAttributes> triangles;

    //vector to store triangle vertices which are being built in progress
    std::vector<VertexAttributes> lines;

    //vector to store triangle vertices
    std::vector<VertexAttributes> triangleVertices;

    // Initialize the viewer and the corresponding callbacks
    SDLViewer viewer;
    viewer.init("Viewer Example", width, height);

    viewer.mouse_move = [&](int x, int y, int xrel, int yrel) {
        float x_pos = (float(x) / float(width) * 2) - 1;
        float y_pos = (float(height - 1 - y) / float(height) * 2) - 1;
        if (currentMode == INSERTION_MODE) {
            if (numOfClicks % 3 != 0) {
                if(lines.size() > 1)
                    lines.pop_back();
                lines.push_back(VertexAttributes(x_pos, y_pos, 0, 1));
                viewer.redraw_next = true;
            }
        }
        else if (currentMode == TRANSLATION_MODE) {
            if (isClicked && selectedTriangle >= 0) {
                isCursorMoving = true;
                newPosition = Vector4f(x_pos, y_pos, 0, 1);
                if (animationMode && !isPositionSet) {
                    oldAnimePosition1 = uniform.view * uniform.rotate * uniform.scale * triangles[selectedTriangle].position;
                    oldAnimePosition2 = uniform.view * uniform.rotate * uniform.scale * triangles[selectedTriangle + 1].position;
                    oldAnimePosition3 = uniform.view * uniform.rotate * uniform.scale * triangles[selectedTriangle + 2].position;
                    isPositionSet = true;
                }
                uniform.translate_delta = newPosition - oldPosition;
                oldPosition = newPosition;
                firstTime = false;
                uniform.mode = EditorMode::TRANSLATION_MODE_KEY;
                translateTriangle(triangles[selectedTriangle], triangles[selectedTriangle + 1], triangles[selectedTriangle + 2], uniform);
                viewer.redraw_next = true;
            }
        }
    };

    viewer.mouse_pressed = [&](int x, int y, bool is_pressed, int button, int clicks, bool mouseButtonUp) {
        float x_pos = (float(x) / float(width) * 2) - 1;
        float y_pos = (float(height - 1 - y) / float(height) * 2) - 1;
        if (currentMode == INSERTION_MODE) {
            if (mouseButtonUp) {
                lines.push_back(VertexAttributes(x_pos, y_pos, 0, 1));
                triangleVertices.push_back(VertexAttributes(x_pos, y_pos, 0, 1));
                numOfClicks += 1;
                viewer.redraw_next = true;
            }
        }
        else if (currentMode == DELETION_MODE) {
            deleteTriangle(triangles, uniform, x_pos, y_pos, viewer);
        }
        else if(currentMode == TRANSLATION_MODE) {
            //Get the selected triangle
            selectedTriangle = getSelectedTriangleIndex(triangles, uniform, x_pos, y_pos);
            
            //If no triangle selected but was previously selected, make it blue now
            if (selectedTriangle < 0) { 
                if (prevClickedTriangle >= 0) {
                    setColor(triangles[prevClickedTriangle], triangles[prevClickedTriangle + 1], triangles[prevClickedTriangle + 2], BLUE);
                    isClicked = false;
                    isCursorMoving = false;
                    prevClickedTriangle = -1;
                    viewer.redraw_next = true;
                }
            }
            else {
                //After mouse button is released
                if (mouseButtonUp) {
                    //Check if it was dragged or selected
                    if (isClicked) {
                        //If it was dragged
                        if (isCursorMoving) {
                            isClicked = false;
                            isCursorMoving = false;
                            oldPosition = newPosition;
                            firstTime = true;
                            viewer.redraw_next = true;
                        }
                        else {
                            isClicked = false;
                            viewer.redraw_next = true;
                        }
                    }
                }
                //When mouse button is pressed (not released yet)
                else {
                    //If a triangle was selected
                    if (selectedTriangle >= 0) {
                        oldPosition = Vector4f(x_pos, y_pos, 0, 1);
                        prevClickedTriangle = selectedTriangle;
                        setColor(triangles[selectedTriangle], triangles[selectedTriangle + 1], triangles[selectedTriangle + 2], HIGHLIGHT);
                        isClicked = true;
                        viewer.redraw_next = true;
                    }
                }
            }
        }
        else if (currentMode == COLOR_MODE) {
            selectedTriangle = getSelectedTriangleIndex(triangles, uniform, x_pos, y_pos);
            if (selectedTriangle >= 0) {
                vertex_index = getNearestVertex(triangles, uniform, selectedTriangle, Vector4f(x_pos, y_pos, 0, 1));
            }
        }
    };

    viewer.mouse_wheel = [&](int dx, int dy, bool is_direction_normal) {
    };

    viewer.key_pressed = [&](char key, bool is_pressed, int modifier, int repeat) {
        
        setCurrentMode(key, currentMode); //Setting current mode

        if (key == EditorMode::ANIMATION_MODE_KEY) {
            animationMode = true;
        }

        if (animationMode) {
            oldAnimePosition1 = triangles[selectedTriangle].position;
            oldAnimePosition2 = triangles[selectedTriangle + 1].position;
            oldAnimePosition3 = triangles[selectedTriangle + 2].position;
            Vector4f temp1 = (uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle].position) - oldAnimePosition1;
            Vector4f temp2 = (uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 1].position) - oldAnimePosition2;
            Vector4f temp3 = (uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 2].position) - oldAnimePosition3;
            if (key == 'n') {
                std::cout << "Animating......";
                Matrix4f translate_temp;
                translate_temp << uniform.translate;
                uniform.translate << identity;
                for (float t = 0.0; t <= 1.0; t += 0.1) {
                    Vector4f now_pos1 = oldAnimePosition1 + t * temp1;
                    Vector4f now_pos2 = oldAnimePosition2 + t * temp2;
                    Vector4f now_pos3 = oldAnimePosition3 + t * temp3;

                    triangles[selectedTriangle].position =  now_pos1;
                    triangles[selectedTriangle + 1].position = now_pos2;
                    triangles[selectedTriangle + 2].position = now_pos3;
                    viewer.redraw(viewer);
                    Sleep(250);
                }
                animationMode = false;
                isPositionSet = false;
                key = 'q';//to exit
            }
            else if (key == 'b') {
                std::cout << "Animating......";
                int n = 3;
                Vector4f P0_1 = oldAnimePosition1;
                Vector4f P0_2 = oldAnimePosition2;
                Vector4f P0_3 = oldAnimePosition3;
                Vector4f P2_1 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle].position;
                Vector4f P2_2 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 1].position;
                Vector4f P2_3 = uniform.view * uniform.translate * uniform.rotate * uniform.scale * triangles[selectedTriangle + 2].position;
                Vector4f P1_1 = Vector4f(temp1.y(), -temp1.x(), temp1.z(), temp1.w());
                Vector4f P1_2 = Vector4f(temp2.y(), -temp2.x(), temp2.z(), temp2.w());
                Vector4f P1_3 = Vector4f(temp3.y(), -temp3.x(), temp3.z(), temp3.w());
                Matrix4f translate_temp;
                translate_temp << uniform.translate;
                uniform.translate << identity;

                for (float t = 0.0; t <= 1.0; t += 0.1) {
                    float t_one = 1 - t;
                    Vector4f now_pos1 = pow(t_one, 2.0) * P0_1 + 2 * t * t_one * P1_1 + pow(t, 2.0) * P2_1;
                    Vector4f now_pos2 = pow(t_one, 2.0) * P0_2 + 2 * t * t_one * P1_2 + pow(t, 2.0) * P2_2;;
                    Vector4f now_pos3 = pow(t_one, 2.0) * P0_3 + 2 * t * t_one * P1_3 + pow(t, 2.0) * P2_3;;

                    triangles[selectedTriangle].position = now_pos1;
                    triangles[selectedTriangle + 1].position = now_pos2;
                    triangles[selectedTriangle + 2].position = now_pos3;
                    viewer.redraw(viewer);
                    Sleep(250);
                }
                animationMode = false;
                isPositionSet = false;
                key = 'q';//to exit
            }
        }

        if (currentMode == TRANSLATION_MODE) {
            if (selectedTriangle >= 0) {
                performTranslationAction(key, triangles, uniform, selectedTriangle);
                viewer.redraw_next = true;
            }
        }
        else if (currentMode == COLOR_MODE) {
            if (key >= '1' && key <= '9') {
                if (vertex_index >= 0) {
                    double val = (key - '1') * 0.1;
                    triangles[vertex_index].color = Vector4f(val, val + 0.1, val + 0.2, 1);
                    viewer.redraw_next = true;
                }
            }
        }
        
        changeViewport(key, zoom, delta, uniform, viewer);
        
    };

    viewer.redraw = [&](SDLViewer &viewer) {
        // Clear the framebuffer
        for (unsigned i=0;i<frameBuffer.rows();i++)
            for (unsigned j=0;j<frameBuffer.cols();j++)
                frameBuffer(i,j).color << 0,0,0,1;

        if (currentMode == INSERTION_MODE) {
            if (numOfClicks == 1) {
                rasterize_lines(program, uniform, lines, 1.0, frameBuffer);
            }
            else if (numOfClicks == 2) {
                VertexAttributes v1 = triangleVertices[0];
                VertexAttributes v2 = triangleVertices[1];
                VertexAttributes v3 = lines[lines.size() - 1];
                std::vector<VertexAttributes> temp(6);
                temp[0] = v1;
                temp[1] = v2;
                temp[2] = v2;
                temp[3] = v3;
                temp[4] = v3;
                temp[5] = v1;
                rasterize_lines(program, uniform, temp, 1.0, frameBuffer);
            }
            else if (numOfClicks == 3) {
                numOfClicks = 0;
                setColor(triangleVertices[0], triangleVertices[1], triangleVertices[2], BLUE);
                Vector4f bary_center = (triangleVertices[0].position + triangleVertices[1].position + triangleVertices[2].position) / 3;
                triangleVertices[0].bary_center = bary_center;
                triangleVertices[1].bary_center = bary_center;
                triangleVertices[2].bary_center = bary_center;
                triangles.insert(triangles.end(), triangleVertices.begin(), triangleVertices.end());
                triangleVertices.clear();
                lines.clear();
            }
        }
        if (triangles.size() >= 3) 
            rasterize_triangles(program, uniform, triangles, frameBuffer);

        // Buffer for exchanging data between rasterizer and sdl viewer
        Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> R(width, height);
        Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> G(width, height);
        Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> B(width, height);
        Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> A(width, height);
        for (unsigned i=0; i<frameBuffer.rows();i++)
        {
            for (unsigned j=0; j<frameBuffer.cols();j++)
            {
                R(i,frameBuffer.cols()-1-j) = frameBuffer(i,j).color(0);
                G(i,frameBuffer.cols()-1-j) = frameBuffer(i,j).color(1);
                B(i,frameBuffer.cols()-1-j) = frameBuffer(i,j).color(2);
                A(i,frameBuffer.cols()-1-j) = frameBuffer(i,j).color(3);
            }
        }
        viewer.draw_image(R, G, B, A);
    };

    viewer.launch();

    return 0;
}