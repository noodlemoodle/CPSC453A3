// cpsc 453 assignment 3 qiyue zhang 10131658

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <math.h>

#define _USE_MATH_DEFINES
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "objmodel.h"

using namespace std;

OBJmodel om;

// window controls
int windowWidth, windowHeight;
bool closeWindow = false;

// pointers for image loading
unsigned char* pixels;
unsigned char* pixels2;

// vectors to hold obj coordinates
vector<float> objVertices;
vector<float> objTex;
vector<float> objNorms;
vector<float> materials;

// mvp+toCenter matrices
glm::mat4 T, M, V, P;

// perspective viewing controls
float lookAtX = 0.0, lookAtY = 0.0, lookAtZ = 0.0;
float camX = 5.0, camY = 5.0, camZ = 5.0;
float zoom = 90.0;
float up = 1.0;
float upsideDown = false;

// object transformation controls
float rollAngle = 0, pitchAngle = 0, yawAngle = 0;
float translateX = 0.0, translateY = 0.0, translateZ = 0.0;
float scale = 1.0;
float objScale = 1.0;
float cx = 0, cy = 0, cz = 0;

// texture mapping controls
int color = 1;
int ao = 1;

// light position
glm::vec3 lightPos = glm::vec3(0, 0, 0);

class Program {
	GLuint vertex_shader;
	GLuint fragment_shader;
public:
	GLuint id;
	Program() {
		vertex_shader = 0;
		fragment_shader = 0;
		id = 0;
	}
	Program(string vertex_path, string fragment_path) {
		init(vertex_path, fragment_path);
	}
	void init(string vertex_path, string fragment_path) {

		id = glCreateProgram();
		vertex_shader = addShader(vertex_path, GL_VERTEX_SHADER);
		fragment_shader = addShader(fragment_path, GL_FRAGMENT_SHADER);

		if (vertex_shader)
			glAttachShader(id, vertex_shader);
		if (fragment_shader)
			glAttachShader(id, fragment_shader);

		glLinkProgram(id);
	}

	GLuint addShader(string path, GLuint type) {
		std::ifstream in(path);
		string buffer = [&in] {
			std::ostringstream ss {};
			ss << in.rdbuf();
			return ss.str();
		}();
		const char *buffer_array[] = { buffer.c_str() };
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, buffer_array, 0);
		glCompileShader(shader);

		// Compile results
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			GLint length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			string info(length, ' ');
			glGetShaderInfoLog(shader, info.length(), &length, &info[0]);
			cerr << "ERROR compiling shader:" << endl << endl;
			cerr << info << endl;
		}
		return shader;
	}
	~Program() {
		glUseProgram(0);
		glDeleteProgram(id);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}
};

class VertexArray {
	std::map<string, GLuint> buffers;
	std::map<string, int> indices;
public:
	GLuint id;
	unsigned int count;
	VertexArray(int c) {
		glGenVertexArrays(1, &id);
		count = c;
	}

	VertexArray(const VertexArray &v) {
		glGenVertexArrays(1, &id);

		// Copy data from the old object
		this->indices = std::map<string, int>(v.indices);
		count = v.count;

		vector<GLuint> temp_buffers(v.buffers.size());

		// Allocate some temporary buffer object handles
		glGenBuffers(v.buffers.size(), &temp_buffers[0]);

		// Copy each old VBO into a new VBO
		int i = 0;
		for (auto &ent : v.buffers) {
			int size = 0;
			glBindBuffer(GL_ARRAY_BUFFER, ent.second);
			glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glBindBuffer(GL_COPY_READ_BUFFER, temp_buffers[i]);
			glBufferData(GL_COPY_READ_BUFFER, size, NULL, GL_STATIC_COPY);

			glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, 0, 0,
					size);
			i++;
		}

		// Copy those temporary buffer objects into our VBOs

		i = 0;
		for (auto &ent : v.buffers) {
			GLuint buffer_id;
			int size = 0;
			int index = indices[ent.first];

			glGenBuffers(1, &buffer_id);

			glBindVertexArray(this->id);
			glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
			glBindBuffer(GL_COPY_READ_BUFFER, temp_buffers[i]);
			glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

			// Allocate VBO memory and copy
			glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0,
					size);
			string indexs = ent.first;

			buffers[ent.first] = buffer_id;
			indices[ent.first] = index;

			// Setup the attributes
			size = size / (sizeof(float) * this->count);
			glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(index);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			i++;
		}

		// Delete temporary buffers
		glDeleteBuffers(v.buffers.size(), &temp_buffers[0]);
	}

	void addBuffer(string name, int index, vector<float> buffer) {
		GLuint buffer_id;
		glBindVertexArray(id);

		glGenBuffers(1, &buffer_id);
		glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
		glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
				buffer.data(), GL_STATIC_DRAW);
		buffers[name] = buffer_id;
		indices[name] = index;

		int components = buffer.size() == 9 ? 3 : 2;
		glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(index);

		// unset states
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void updateBuffer(string name, vector<float> buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, buffers[name]);
		glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
				buffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	~VertexArray() {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		for (auto &ent : buffers)
			glDeleteBuffers(1, &ent.second);
	}
};

struct Texture {
	GLuint id;
	GLuint type;
	int w;
	int h;
	int channels;

	Texture() : id(0), type(0), w(0), h(0), channels(0){}

}tex1, ao1;

// read materials file, return a float vector
vector<float> readMTL(string f) {
	if(f.empty()) {
		cerr << "ERROR: blank filename" << endl;
		exit(0);
	}
	// try to open the file
	ifstream file;
	file.open( f, fstream::in );
	// didn't work? fail!
	if ( file.fail() ) {
		cerr << "ERROR: OBJmodel: Couldn't load \"" << f << "\"." << endl;
		exit(0);
	}
	string buffer = "";
	string line;
	vector<float> mtl;
	int lineNum = 0;
	while( file.good() ) {
		getline( file, line );	// while we have data, read in a line
		if(lineNum >= 4 && lineNum <= 12) {
			switch( line[0] ) {
				case 'i':
					buffer += line.substr(6) + " ";
					lineNum++;
					continue;
				case 'd':
					buffer += line.substr(2) + " ";
					lineNum++;
					break;
				default:
					buffer += line.substr(3) + " "; lineNum++; continue;
			}
		}
		lineNum++;
	}

	istringstream iss(buffer);
	for (int i = 0; i < 16; i++ ) {
		float mtlVal;
		iss >> mtlVal;
		mtl.push_back(mtlVal);
	}

	return mtl;
}

// helper function for finding the transtations to center an object
vector<float> findBound(vector<float> v) {
	float tempMaxX = 0;
	float tempMinX = 0;
	float tempMaxY = 0;
	float tempMinY = 0;
	float tempMinZ = 0;
	float tempMaxZ = 0;
	float centerX, centerY, centerZ;

	for (uint i = 0; i < v.size(); i+=4) {
		if(v[i] < tempMinX){
			tempMinX = v[i];
		}
		if(v[i] > tempMaxX){
			tempMaxX = v[i];
		}
		if(v[i+1] < tempMinY){
			tempMinY = v[i+1];
		}
		if(v[i+1] > tempMaxY){
			tempMaxY = v[i+1];
		}
		if(v[i+2] < tempMinZ){
			tempMinZ = v[i+2];
		}
		if(v[i+2] > tempMaxZ){
			tempMaxZ = v[i+2];
		}
	}
	centerX = (tempMinX + tempMaxX)/2;
	centerY = (tempMinY + tempMaxY)/2;
	centerZ = (tempMinZ + tempMaxZ)/2;
	return vector<float> {tempMinX, tempMaxX, tempMinY, tempMaxY, tempMinZ, tempMaxZ, centerX, centerY, centerZ};

}

// bing ao and color textures
void initTextures(string f1, string f2, GLuint* textures, GLuint pid) {

	glGenTextures(2, textures);

	stbi_set_flip_vertically_on_load(true);
	pixels = stbi_load(f1.c_str(), &tex1.w, &tex1.h, &tex1.channels, 0);
	GLuint f11 = tex1.channels == 3 ? GL_RGB8 : GL_RGBA8;
	GLuint f12 = tex1.channels == 3 ? GL_RGB : GL_RGBA;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, f11, tex1.w, tex1.h, 0, f12, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(pixels);
	GLint colorLoc = glGetUniformLocation(pid, "tex");
	glUniform1i(colorLoc, 0);
	glBindTextures(GL_TEXTURE_2D, 2, 0);

	stbi_set_flip_vertically_on_load(true);
	pixels2 = stbi_load(f2.c_str(), &ao1.w, &ao1.h, &ao1.channels, 0);
	GLuint f21 = ao1.channels == 3 ? GL_RGB8 : GL_RGBA8;
	GLuint f22 = ao1.channels == 3 ? GL_RGB : GL_RGBA;
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, f21, ao1.w, ao1.h, 0, f22, GL_UNSIGNED_BYTE, pixels2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(pixels2);
	GLint aoLoc = glGetUniformLocation(pid, "ao");
	glUniform1i(aoLoc, 1);

	glBindTextures(GL_TEXTURE_2D, 2, 0);
}

// load obj file
bool initObj(string objFile) {
	if(!om.load(objFile)) {
		cout<<"failed"<<endl;
		return false;
	} else {
		cout<<"done"<<endl;
		for(uint tri = 0; tri < om.triangleCount(); tri++) {
			for (uint vert = 0; vert < 3; vert++) {
				objVertices.push_back(om[tri].vertex[vert].pos.x);
				objVertices.push_back(om[tri].vertex[vert].pos.y);
				objVertices.push_back(om[tri].vertex[vert].pos.z);
				objVertices.push_back(0);

				objTex.push_back(om[tri].vertex[vert].tex.s);
				objTex.push_back(om[tri].vertex[vert].tex.t);
				objTex.push_back(om[tri].vertex[vert].tex.u);

				objNorms.push_back(om[tri].vertex[vert].norm.x);
				objNorms.push_back(om[tri].vertex[vert].norm.y);
				objNorms.push_back(om[tri].vertex[vert].norm.z);
			}
		}
		return true;
	}
}

// bind object materials
void bindMaterials(GLuint pid, string mtlFile) {
	glUseProgram(pid);
	materials = readMTL(mtlFile);
	GLint nsLoc = glGetUniformLocation(pid, "Ns");
	glUniform1f(nsLoc, materials[0]);

	GLint kaLoc = glGetUniformLocation(pid, "Ka");
	glUniform3f(kaLoc, materials[1], materials[2], materials[3]);

	GLint kdLoc = glGetUniformLocation(pid, "Kd");
	glUniform3f(kdLoc, materials[4], materials[5], materials[6]);

	GLint ksLoc = glGetUniformLocation(pid, "Ks");
	glUniform3f(ksLoc, materials[7], materials[8], materials[9]);

	GLint keLoc = glGetUniformLocation(pid, "Ke");
	glUniform3f(keLoc, materials[10], materials[11], materials[12]);

	GLint niLoc = glGetUniformLocation(pid, "Ni");
	glUniform1f(niLoc, materials[13]);

	GLint dLoc = glGetUniformLocation(pid, "d");
	glUniform1f(dLoc, materials[14]);

	GLint illumLoc = glGetUniformLocation(pid, "illum");
	glUniform1f(illumLoc, materials[15]);

}

// render object
void render(GLuint pid, VertexArray va, GLuint* textures) {
	glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pid);
	glBindVertexArray(va.id);
	glDrawArrays(GL_TRIANGLES, 0, va.count);
	glBindVertexArray(0); //unbind
	glUseProgram(0);

}

// initialize object vertex array
void display(GLuint pid, GLuint* textures) {

	VertexArray obj(objVertices.size()/4);

	obj.addBuffer("va", 0, objVertices);
	obj.addBuffer("tex", 1, objTex);
	obj.addBuffer("norms", 2, objNorms);

	render(pid, obj, textures);
}

// key callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	// close window
     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) closeWindow = closeWindow?false:true;

	// reset to default state
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		ao = 1; color = 1;
		camX = 5; camY = 5; camZ = 5; zoom = 90; upsideDown = false;
		rollAngle = 0; pitchAngle = 0; yawAngle = 0; scale = 1.0; translateX = 0; translateY = 0, translateZ = 0; //THESE SHOULDNT BE 0 (TRANS AND SCALE)
		lightPos[0] = 0.0; lightPos[1] = 0.0; lightPos[2] = 0.0;
	}

	// toggle texture mappings
	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) ao = (ao == 1) ? 0 : 1;
	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) color = (color == 1) ? 0 : 1;
	if (key == GLFW_KEY_F3 && action == GLFW_PRESS) { ao = (ao == 1) ? 0 : 1; color = (color == 1) ? 0 : 1; }

	// perspective viewing controls
	if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) camX += upsideDown ? -0.5 : 0.5;
    	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) camY += upsideDown ? -0.5 : 0.5;
     if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) camZ += upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) camX -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) camY -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) camZ -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_I && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtX += upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_O && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtY += upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_P && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtZ += upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_J && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtX -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_K && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtY -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_L && (action == GLFW_REPEAT || action == GLFW_PRESS)) lookAtZ -= upsideDown ? -0.5 : 0.5;
	if (key == GLFW_KEY_F && (action == GLFW_REPEAT || action == GLFW_PRESS)) zoom = zoom + 5.0 < 165.0 ? zoom + 5.0 : zoom;
	if (key == GLFW_KEY_R && (action == GLFW_REPEAT || action == GLFW_PRESS)) zoom = zoom - 5.0 > 25.0 ? zoom - 5.0 : zoom;
	if (key == GLFW_KEY_CAPS_LOCK && action == GLFW_PRESS) upsideDown = upsideDown?false:true;

	// intrinsic rotation controls
	if (key == GLFW_KEY_KP_7 && (action == GLFW_REPEAT || action == GLFW_PRESS)) rollAngle -= 0.1;
    	if (key == GLFW_KEY_KP_9 && (action == GLFW_REPEAT || action == GLFW_PRESS)) rollAngle += 0.1;
     if (key == GLFW_KEY_KP_4 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitchAngle -= 0.1;
	if (key == GLFW_KEY_KP_6 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitchAngle += 0.1;
	if (key == GLFW_KEY_KP_1 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yawAngle -= 0.1;
	if (key == GLFW_KEY_KP_3 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yawAngle += 0.1;
	if (key == GLFW_KEY_KP_5 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rollAngle += 0.1;
		pitchAngle += 0.1;
		yawAngle += 0.1;
	}
	if (key == GLFW_KEY_KP_2 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rollAngle -= 0.1;
		pitchAngle -= 0.1;
		yawAngle -= 0.1;
	}

	// scaling controls
	if (key == GLFW_KEY_KP_ADD && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 1.2;
	if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 0.8;

	// translation controls
	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS)) translateY += 0.2;
	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS)) translateY -= 0.2;
	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS)) translateX += 0.2;
	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS)) translateX -= 0.2;

	// light positioning
	if (key == GLFW_KEY_INSERT && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[0] += 1.0;
	if (key == GLFW_KEY_HOME && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[1] += 1.0;
	if (key == GLFW_KEY_PAGE_UP && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[2] += 1.0;
	if (key == GLFW_KEY_DELETE && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[0] -= 1.0;
	if (key == GLFW_KEY_END && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[1] -= 1.0;
	if (key == GLFW_KEY_PAGE_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS)) lightPos[2] -= 1.0;

	// display
	cout << "CURRENT" << endl;
	cout << "CAMERA POSITION: ("<< camX <<", "<< camY <<", "<< camZ <<")" << endl;
	cout << "LOOKING AT: (" << lookAtX << ", " << lookAtY << ", " << lookAtZ << ")" << endl;
	cout << "LIGHT POSITION: (" << lightPos[0] << ", " << lightPos[1] << ", " << lightPos[2] << ")" << endl;
	cout << "ZOOM = " << zoom << endl;
	cout << "UPSIDEDOWN = " << upsideDown << endl;
	cout << "OBJECT ROTATION IN X (ROLL): " << rollAngle << " degrees" << endl;
	cout << "OBJECT ROTATION IN Y (PITCH): " << pitchAngle << " degrees" << endl;
	cout << "OBJECT ROTATION IN Z (YAW): " << yawAngle << " degrees" << endl;
	cout << "OBJECT TRANSLATION IN X: " << translateX << endl;
	cout << "OBJECT TRANSLATION IN Y: " << translateY << endl;
	cout << "OBJECT TRANSLATION IN Z: " << translateZ << endl;
	cout << "OBJECT SCALING: " << scale << endl;
	cout << "AMBIENT OCCLUSION APPLIED: " << ao << endl;
	cout << "COLOR APPLIED: " << color << endl;

}

// get matrix for centering and scaling object to window
glm::mat4 getT() {
	vector<float> v = findBound(objVertices);
	cx = v[6]; cy = v[7]; cz = v[8];
	float xmin = v[0], xmax = v[1], ymin = v[2], ymax = v[3], zmin = v[4], zmax = v[5];

	objScale = 1/fmax(fmax(xmax - xmin, ymax - ymin), zmax - zmin);
	glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(objScale, objScale, objScale));
	glm::mat4 Translate = glm::translate(glm::mat4(1.0f), glm::vec3(-cx, -cy, -cz));
	glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), rollAngle, glm::vec3(1, 0, 0));
	glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), pitchAngle, glm::vec3(0, 1, 0));
	glm::mat4 RotateZ = glm::rotate(glm::mat4(1.0f), yawAngle, glm::vec3(0, 0, 1));

	glm::mat4 Transform = Scale*RotateZ*RotateY*RotateX*Translate;
	return Transform;
}

// get object model matrix
glm::mat4 getM() {
	glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
	glm::mat4 Translate = glm::translate(glm::mat4(1.0f), glm::vec3(translateX, translateY, translateZ));
	return Scale*Translate;
}

// get view matrix
glm::mat4 getV() {
	up = upsideDown? -1.0 : 1.0;
	return glm::lookAt(glm::vec3(camX, camY, camZ), glm::vec3(lookAtX, lookAtY, lookAtZ), glm::vec3(0.0, up, 0.0));
}

// get projection matrix
glm::mat4 getP() {
	return glm::perspective(glm::radians(zoom), (float)((float)windowWidth/(float)windowHeight), 0.1f, 100.0f);
}

int main( const int argc, const char** argv ) {

	// takes in exactly 4 commandline arguments
	if (argc != 5) {

		cout << " USAGE: " << argv[0] << " [objFile.obj] [objColor.png] [objAo.png] [objMaterials.mtl] " << endl;
		return 1;
	}
	for(uint i = 0; i < 4; i++) {
		cout << "- Loading " << argv[i+1] << "..";
	}

	if (!glfwInit()) {
			cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
			return -1;
	}
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(800, 800, "Assignment 3", 0, 0);
	glfwSetWindowAspectRatio(window, 1, 1);

	if (!window) {
			cout << "failed to create window, TERMINATING" << endl;
			glfwTerminate();
			return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	Program p("data/vertex.glsl", "data/fragment.glsl");
	glUseProgram(p.id);
	GLuint textures[2];
	initTextures(argv[2], argv[3], textures, p.id);
	initObj(argv[1]);
	bindMaterials(p.id, argv[4]);

	while(!glfwWindowShouldClose(window)) {

		glfwGetWindowSize(window, &windowWidth, &windowHeight);
		glViewport(0, 0, windowWidth, windowHeight);

		T = getT();
		M = getM();
		V = getV();
		P = getP();

		glm::mat4 mvp = P*V*M*T;

		glUseProgram(p.id);

		GLint lightLoc = glGetUniformLocation(p.id, "lightPos");
		glUniform3f(lightLoc, lightPos.x, lightPos.y, lightPos.z);

		GLint mvpLoc = glGetUniformLocation(p.id, "mvp");
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);

		GLint tLoc = glGetUniformLocation(p.id, "tM");
		glUniformMatrix4fv(tLoc, 1, GL_FALSE, &T[0][0]);

		GLint mLoc = glGetUniformLocation(p.id, "mM");
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, &M[0][0]);

		GLint vLoc = glGetUniformLocation(p.id, "vM");
		glUniformMatrix4fv(vLoc, 1, GL_FALSE, &V[0][0]);

		GLint pLoc = glGetUniformLocation(p.id, "pM");
		glUniformMatrix4fv(pLoc, 1, GL_FALSE, &P[0][0]);

		GLint aoBoolLoc = glGetUniformLocation(p.id, "ao_bool");
		glUniform1i(aoBoolLoc, ao);

		GLint colorBoolLoc = glGetUniformLocation(p.id, "color_bool");
		glUniform1i(colorBoolLoc, color);

		display(p.id, textures);

		glfwSwapBuffers(window);
		glfwPollEvents();

		if(closeWindow){ break; }

	}

	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "bui bui" << endl;
	return 0;

}
