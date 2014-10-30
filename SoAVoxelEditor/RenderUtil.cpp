#include "RenderUtil.h"
#include "Camera.h"
#include "Shader.h"
#include "Errors.h"
#include "Voxel.h"

Mesh* RenderUtil::_mesh = nullptr;
Mesh* RenderUtil::_referenceCubeMesh = nullptr;
GLuint* RenderUtil::_referenceCubeIndices = new GLuint[36];
GLuint* RenderUtil::_brushIndices;
glm::vec3 RenderUtil::_lastPosition = glm::vec3(-1, -1, -1);
BlockMesh RenderUtil::_voxVerts = BlockMesh();
BlockMesh RenderUtil::_voxBaseVerts = BlockMesh();
vector <BlockVertex> RenderUtil::_brushVerts = vector <BlockVertex>();

bool RenderUtil::checkGlError(){
    GLenum err = glGetError();
    if (err != GL_NO_ERROR){
        switch (err){
        case GL_OUT_OF_MEMORY:
            error("Out of memory! Try lowering the voxel view distance.");
            return 1;
        case GL_INVALID_ENUM:
            error("GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument.");
            return 0;
        case GL_INVALID_VALUE:
            error("GL_INVALID_VALUE - A numeric argument is out of range.");
            return 0;
        case GL_INVALID_OPERATION:
            error("GL_INVALID_OPERATION - The specified operation is not allowed in the current state.");
            return 0;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error("The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete.");
            return 0;
        default:
            error(("OpenGL ERROR (" + to_string(err) + string(") ")).c_str());
            return 0;
        }
    }
    return 0;
}

void RenderUtil::drawLine(Camera *camera, glm::vec3 p1, glm::vec3 p2, GLubyte r, GLubyte g, GLubyte b, int thickness)
{
    GridVertex verts[2];

    gridShader.bind();

    glm::mat4 modelMatrix(1);
   
    const glm::vec3 &position = camera->getPosition();

    modelMatrix[3][0] = -position.x;
    modelMatrix[3][1] = -position.y;
    modelMatrix[3][2] = -position.z;
 

    glm::mat4 MVP = camera->getProjectionMatrix() * camera->getViewMatrix() * modelMatrix;

    glUniformMatrix4fv(gridShader.mvpID, 1, GL_FALSE, &MVP[0][0]);

    static GLuint vboID = 0;

    if (vboID == 0){
        glGenBuffers(1, &vboID);
    }

    verts[0].position = p1;
    verts[0].color[0] = r;
    verts[0].color[1] = g;
    verts[0].color[2] = b;
    verts[0].color[3] = 255;
    verts[1].position = p2;
    verts[1].color[0] = r;
    verts[1].color[1] = g;
    verts[1].color[2] = b;
    verts[1].color[3] = 255;

    glBindBuffer(GL_ARRAY_BUFFER, vboID);
   
    // orphan the buffer for speed
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), NULL, GL_STREAM_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)0); //vertexPosition
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GridVertex), (void *)12); //vertexColor

    glLineWidth((GLfloat)thickness);

    //when drawing lines theres no bonus for using indices so we just use draw arrays
    //so we unbind the element array buffer for good measure
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDrawArrays(GL_LINES, 0, 2);

    gridShader.unBind();

    glLineWidth(1);
}

void RenderUtil::uploadMesh(GLuint* vboID, GLuint* iboID, const BlockVertex* blockVertices, const int numVertices, const GLuint* indices, const int numIndices) {
    //generate a buffer object for the vboID. after this call, vboID will be a number > 0
    glGenBuffers(1, vboID);
    //generate buffer object for the indices
    glGenBuffers(1, iboID);

    //bind the buffers into the correct slots
    glBindBuffer(GL_ARRAY_BUFFER, *vboID);

    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(BlockVertex), blockVertices, GL_STATIC_DRAW);

    //now do the same thing for the elements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *iboID);

    //Don't need vTot, just use the ratio of indices to verts, which is 6/4
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);
}

void RenderUtil::uploadMesh(GLuint* vboID, GLuint* iboID, const glm::vec3* vertices, const int numVertices, const GLuint* indices, const int numIndices) {
    //generate a buffer object for the vboID. after this call, vboID will be a number > 0
    glGenBuffers(1, vboID);
    //generate buffer object for the indices
    glGenBuffers(1, iboID);

    //bind the buffers into the correct slots
    glBindBuffer(GL_ARRAY_BUFFER, *vboID);

    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), vertices, GL_STATIC_DRAW);

    //now do the same thing for the elements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *iboID);

    //Don't need vTot, just use the ratio of indices to verts, which is 6/4
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);
}

void RenderUtil::initializeWireframeBox() {
    _mesh = new Mesh;
    const static int numVertices = 8;
    const static int numIndices = 24;
    const static glm::vec3 vertices[numVertices] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),

        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f),
    };
    const static GLuint indices[numIndices] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        0, 4,
        1, 5,
        2, 6,
        3, 7,
        4, 5,
        5, 6,
        6, 7,
        7, 4
    };

    uploadMesh(&_mesh->vboID, &_mesh->iboID, vertices, numVertices, indices, numIndices);
    _mesh->numIndices = numIndices;
}

void RenderUtil::drawWireframeBox(class Camera* camera, const glm::vec3& position, const glm::vec3& size, const glm::vec4& color) {
    if(!_mesh) initializeWireframeBox();

    wireframeShader.bind();

    glBindBuffer(GL_ARRAY_BUFFER, _mesh->vboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh->iboID);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    const glm::vec3 &cameraPosition = camera->getPosition();

    glm::mat4 modelMatrix(1);
    modelMatrix[3][0] = -cameraPosition.x + position.x;
    modelMatrix[3][1] = -cameraPosition.y + position.y;
    modelMatrix[3][2] = -cameraPosition.z + position.z;
    modelMatrix[0][0] = size.x;
    modelMatrix[1][1] = size.y;
    modelMatrix[2][2] = size.z;
    glm::mat4 MVP = camera->getProjectionMatrix() * camera->getViewMatrix() * modelMatrix;

    glUniformMatrix4fv(blockShader.mvpID, 1, GL_FALSE, &MVP[0][0]);
    glUniform4f(wireframeShader.colorID, color.r, color.g, color.b, color.a);

    glLineWidth(2);

    glDrawElements(GL_LINES, _mesh->numIndices, GL_UNSIGNED_INT, (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    wireframeShader.unBind();
}


void RenderUtil::releaseWireframeBox() {
    if(_mesh) {
        glDeleteBuffers(1, &_mesh->vboID);
        glDeleteBuffers(1, &_mesh->iboID);
        delete _mesh;
    }
}

void RenderUtil::initializeReferenceVoxel(){
	_referenceCubeIndices = new GLuint[36];
	
	for (int i = 0, j = 0; i < 36; i += 6, j += 4){
		_referenceCubeIndices[i] = j;
		_referenceCubeIndices[i + 1] = j + 1;
		_referenceCubeIndices[i + 2] = j + 2;
		_referenceCubeIndices[i + 3] = j + 2;
		_referenceCubeIndices[i + 4] = j + 3;
		_referenceCubeIndices[i + 5] = j;
	}

	for (int i = 0; i < 24; i++){
		_voxBaseVerts.verts[i].position.x = cubeVertices[i * 3];
		_voxBaseVerts.verts[i].position.y = cubeVertices[i * 3 + 1];
		_voxBaseVerts.verts[i].position.z = cubeVertices[i * 3 + 2];

		_voxBaseVerts.verts[i].normal.x = cubeNormals[i * 3];
		_voxBaseVerts.verts[i].normal.y = cubeNormals[i * 3 + 1];
		_voxBaseVerts.verts[i].normal.z = cubeNormals[i * 3 + 2];

		_voxBaseVerts.verts[i].color[0] = 0;
		_voxBaseVerts.verts[i].color[1] = 0;
		_voxBaseVerts.verts[i].color[2] = 0;
		_voxBaseVerts.verts[i].color[3] = 100;
	}

	_referenceCubeMesh = new Mesh();
	_referenceCubeMesh->iboID = 0;
	_referenceCubeMesh->vboID = 0;

	_lastPosition = glm::vec3(-1, -1, -1);
}

void RenderUtil::drawReferenceVoxel(class Camera* camera, const glm::vec3 position, Brush *brush){
	if (!_referenceCubeMesh) initializeReferenceVoxel();

	blockShader.bind();


	const glm::vec3 &pos = camera->getPosition();

	glm::mat4 modelMatrix(1);
	modelMatrix[3][0] = -pos.x;
	modelMatrix[3][1] = -pos.y;
	modelMatrix[3][2] = -pos.z;

	glm::mat4 MVP = camera->getProjectionMatrix() * camera->getViewMatrix() * modelMatrix;

	//send our uniform data, the matrix, the light position, and the texture data
	glUniformMatrix4fv(blockShader.mvpID, 1, GL_FALSE, &MVP[0][0]);

	glm::vec3 lightPos = position;
	lightPos = glm::normalize(lightPos);
	glUniform3f(blockShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);

	GLuint *indices;

	if ((int)position.x != (int)_lastPosition.x || (int)position.y != (int)_lastPosition.y || (int)position.z != (int)_lastPosition.z){
		if (brush == NULL){
			indices = new GLuint[36];
			for (int i = 0; i < 24; i++){
				_voxVerts.verts[i] = _voxBaseVerts.verts[i];
				_voxVerts.verts[i].position.x += position.x;
				_voxVerts.verts[i].position.y += position.y;
				_voxVerts.verts[i].position.z += position.z;
			}
			RenderUtil::uploadMesh(&_referenceCubeMesh->vboID, &_referenceCubeMesh->iboID, &_voxVerts.verts[0], 24, _referenceCubeIndices, 36);
		}
		else{
			glm::vec3 diff(position.x-_lastPosition.x, position.y-_lastPosition.y, position.z-_lastPosition.z);
			for (int i = 0; i < (int)_brushVerts.size(); i++){
				_brushVerts[i].position.x += diff.x;
				_brushVerts[i].position.y += diff.y;
				_brushVerts[i].position.z += diff.z;
			}
			cout << _brushVerts[0].color[0] << endl;
			RenderUtil::uploadMesh(&_referenceCubeMesh->vboID, &_referenceCubeMesh->iboID, &_brushVerts[0], _brushVerts.size(), _brushIndices, (_brushVerts.size() / 4) * 6);
		}
		_lastPosition = position;
	}
	else{
		glBindBuffer(GL_ARRAY_BUFFER, _referenceCubeMesh->vboID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _referenceCubeMesh->iboID);
	}

	//initialize the buffer, only happens once
	
	//set our attribute pointers using our interleaved vertex data. Last parameter is offset into the vertex
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)0); //vertexPosition
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BlockVertex), (void *)12); //vertexColor
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)16); //vertexNormal

	//Finally, draw our data. The last parameter is the offset into the bound buffer
	if (brush != NULL){
		glDrawElements(GL_TRIANGLES, (6 * _brushVerts.size()) / 4, GL_UNSIGNED_INT, NULL);
		//glDrawElements(GL_TRIANGLES, 36 * brushCoords.size(), GL_UNSIGNED_INT, NULL);
	}
	else{
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
	}

	blockShader.unBind();
}

void RenderUtil::changeReferenceColor(glm::vec4 color){
	for (int i = 0; i < 24; i++){
		_voxBaseVerts.verts[i].color[0] = (GLubyte)color.r;
		_voxBaseVerts.verts[i].color[1] = (GLubyte)color.g;
		_voxBaseVerts.verts[i].color[2] = (GLubyte)color.b;
		_voxBaseVerts.verts[i].color[3] = (GLubyte)color.a;
	}
}

inline void RenderUtil::addFace(BlockVertex* verts, glm::vec3 pos){
	BlockVertex tv;
	#define VERTS_PER_QUAD 4
	for (int i = 0; i < VERTS_PER_QUAD; i++){
		tv = *(verts++);
		tv.position += pos;
		_brushVerts.push_back(tv);
	}
}

void RenderUtil::meshBrush(Brush *brush){
	_brushVerts.clear();
	BlockVertex tv;

	int voxelIndex;

// Constants for readability
#define FRONT_INDEX 0
#define RIGHT_INDEX 4
#define TOP_INDEX 8
#define LEFT_INDEX 12
#define BOTTOM_INDEX 16
#define BACK_INDEX 20


	printf("creating brush mesh\n");
	for (int z = 0; z < brush->length; z++){
		for (int y = 0; y < brush->height; y++){
			for (int x = 0; x < brush->width; x++){
				voxelIndex = z*brush->width*brush->height + y*brush->width + x;
				if (brush->voxels[voxelIndex].type != '\0'){
					if (z < brush->length - 1){//Front face
						if (brush->voxels[voxelIndex + (brush->height + brush->width)].type == '\0'){
							addFace(&_voxBaseVerts.verts[FRONT_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[FRONT_INDEX], glm::vec3(x, y, z));
					}
					if (x < brush->width - 1){//Right face
						if (brush->voxels[voxelIndex + 1].type == '\0'){
							addFace(&_voxBaseVerts.verts[RIGHT_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[RIGHT_INDEX], glm::vec3(x, y, z));
					}
					if (y < brush->height - 1){//Top face
						if (brush->voxels[voxelIndex + brush->width].type == '\0'){
							addFace(&_voxBaseVerts.verts[TOP_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[TOP_INDEX], glm::vec3(x, y, z));
					}
					if (x > 0){//Left face
						if (brush->voxels[voxelIndex - 1].type == '\0'){
							addFace(&_voxBaseVerts.verts[LEFT_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[LEFT_INDEX], glm::vec3(x, y, z));
					}
					if (y > 0){//bottom face
						if (brush->voxels[voxelIndex - brush->width].type == '\0'){
							addFace(&_voxBaseVerts.verts[BOTTOM_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[BOTTOM_INDEX], glm::vec3(x, y, z));
					}
					if (z > 0){//Back face
						if (brush->voxels[voxelIndex - (brush->width * brush->height)].type == '\0'){
							addFace(&_voxBaseVerts.verts[BACK_INDEX], glm::vec3(x, y, z));
						}
					}
					else{
						addFace(&_voxBaseVerts.verts[BACK_INDEX], glm::vec3(x, y, z));
					}
				}
			}
		}
	}

	_brushIndices = new GLuint[(_brushVerts.size() / 4) * 6];
	for (int i = 0, j = 0; i < (int)((_brushVerts.size() / 4) * 6); i += 6, j += 4){
		_brushIndices[i] = j;
		_brushIndices[i + 1] = j + 1;
		_brushIndices[i + 2] = j + 2;
		_brushIndices[i + 3] = j + 2;
		_brushIndices[i + 4] = j + 3;
		_brushIndices[i + 5] = j;
	}
	_lastPosition = glm::vec3(0, 0, 0);
}