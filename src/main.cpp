#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility
#include "trackball.h"	// virtual trackball
#include "sphere.h"
#include "virus.h"
#include "medicine.h"
#include "player.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "irrKlang\irrKlang.h"
#pragma comment(lib, "irrKlang.lib")

//*************************************
// global constants
static const char*	window_name = "T1 - Switch";
static const char*	vert_shader_path = "../bin/shaders/covid19.vert";
static const char*	frag_shader_path = "../bin/shaders/covid19.frag";
static const char*	virus_mesh_path	= "../bin/mesh/virus.obj";
static const char*  player_mesh_path = "../bin/mesh/player.obj";
static const char* medicine_mesh_path = "../bin/mesh/vaccine.obj";

static const char*	earth_image_path = "../bin/images/earth.jpg"; //http://www.shadedrelief.com/natural3/pages/textures.html
static const char* virus_image_path = "../bin/images/purple.jpg";
static const char* player_image_path = "../bin/images/red.jpg";
static const char* mp3_path = "../bin/sounds/bgm.mp3";
static const char* eat_sound_path = "../bin/sounds/yum.mp3";


player_t player = create_player();
vec3	O2P = player.center - vec3(0, 0, 0);	// Vector Origin to player

//*************************************
// common structures
struct camera
{
	vec3	eye = player.center + O2P*1.75;
	vec3	at = vec3( 0, 0, 0 );	// 왜 010 이어야 하나?ㅋㅋ 왜지
	vec3	up = vec3( 0, 0, 1 );
	mat4	view_matrix = mat4::look_at( eye, at, up );

	float	fovy = PI/4.0f; // must be in radian
	float	aspect;
	float	dnear = 1.0f;
	float	dfar = 1000.0f;
	mat4	projection_matrix;
};


struct myMesh
{
	std::vector<vec3>	vertex_list;
	std::vector<vec2>	uv_list;
	std::vector<vec3>	normal_list;
	std::vector<uint>	index_list;
	GLuint	vertex_buffer = 0;
	GLuint	uv_buffer = 0;
	GLuint	index_buffer = 0;
	GLuint	vertex_array = 0;
	GLuint	texture = 0;

	~myMesh()
	{
		if (vertex_buffer) glDeleteBuffers(1, &vertex_buffer);
		if (index_buffer) glDeleteBuffers(1, &index_buffer);
		if (vertex_array) glDeleteVertexArrays(1, &vertex_array);
	}
};

//*************************************
// sound
irrklang::ISoundEngine* engine;
irrklang::ISoundSource* wave_src = nullptr;
irrklang::ISoundSource* mp3_src = nullptr;
irrklang::ISoundSource* eat_sound_src = nullptr;


//*************************************
// window objects
GLFWwindow*	window = nullptr;
ivec2		window_size = cg_default_window_size(); // initial window size
float dpi_scale = cg_get_dpi_scale();

//*************************************
// OpenGL objects
GLuint	program	= 0;	// ID holder for GPU program
GLuint	vertex_array = 0;	// ID holder for vertex array object
GLuint	EARTH_IMG = 0;
GLuint	VIRUS_IMG = 0;
GLuint PLAYER_IMG = 0;

//*************************************
// global variables
int		frame = 0;				// index of rendering frames
float		t = 0.0f;						// current simulation parameter
static const uint	NUM1 = 72;
static const uint	NUM2 = 36;
int		frag_type = 0;


float	time_interval = 1.0f;	// to controll spreading speed of virus
float a = 0.0f;


myMesh* virus_mesh = nullptr;
myMesh* player_mesh = nullptr;
myMesh* medicine_mesh = nullptr;

#ifndef GL_ES_VERSION_2_0
bool	b_wireframe = false;
#endif


//*************************************
// scene objects
camera		cam;
trackball	tb;


sphere_t earth_sphere = create_sphere();
std::vector<virus_t>	viruses;
std::vector<medicine_t> medicines;

bool game_over = false;
bool set_text = false;
bool restart = false;
bool start = true;
bool win = false;



// **********************
// text
bool init_text();
void render_text(std::string text, GLint x, GLint y, GLfloat scale, vec4 color, GLfloat dpi_scale);

//*************************************
void update()
{
	// update projection matrix
	cam.aspect = window_size.x/float(window_size.y);
	cam.projection_matrix = mat4::perspective( cam.fovy, cam.aspect, cam.dnear, cam.dfar );

	// build the model matrix for oscillating scale
	t = float(glfwGetTime());

	if (!game_over&&!start) {
		if (viruses.size() > 50)
			game_over = true;
		if (time_interval < t) {
			printf(" viruses in the earth: %d \n", viruses.size());
			
			virus_t more_virus = add_viruses(player.scale);
			viruses.emplace_back(more_virus);
			time_interval += 0.5f;	//?초마다 새로운 바이러스 생성
		}
		int collision = player.collision(viruses, medicines);
		if (collision == 1) {
			engine->play2D(eat_sound_src, false);
		}
		else if (collision == 3) {
			game_over = true;
			//engine->play2D(mp3_src, true, true);
		}
		//medicine과의 충돌 판단
		else if( collision == 4) {
			for (uint i = 0;i < viruses.size() / 2;i++) {
				viruses.erase(viruses.begin() + i);
			}
		}
		//medicine create
		int random = rand() % 20000;
		if (random == 0) {
			medicines.emplace_back(create_medicine());
		}
		// 북극 남극 지나갈때
		if (player.upsidedown) cam.up = vec3(0, 0, -1);
		else cam.up = vec3(0, 0, 1);
	}
	else {
		viruses.clear();
		time_interval = t + 1;
	}

	if (restart) {
		printf("restart the game!\n");
		viruses.clear();
		medicines.clear();
		player = create_player();
		time_interval = t + 1;
		restart = false;
		game_over = false;
		win = false;
	}

	O2P = player.center - vec3(0, 0, 0);
	cam.eye = player.center + O2P * 1.75;
	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);
	//printf("player center: %f, %f, %f\n", player.center.x, player.center.y, player.center.z);

	a = abs(sin(float(glfwGetTime()) * 2.5f));

	// update uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation(program, "view_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.view_matrix);
	uloc = glGetUniformLocation(program, "projection_matrix");	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// notify GL that we use our own program
	glUseProgram( program );

	if (start) {
		render_text("COVID 19 ", window_size.x / 3 + 35 , 100, 1.2f, vec4(1.0f, 0.4f, 0.1f, 0.8f), 1.0f);

		render_text("WIN : kill more than 50 viruses", 210 , 250, 0.4f, vec4(1.0f, 1.0f, 1.0f, 0.8f), dpi_scale);
		render_text("LOSE : remain more than 50 viruses in the earth", 210, 280, 0.4f, vec4(1.0f, 1.0f, 1.0f, 0.8f), dpi_scale);
		render_text("OR try to kill bigger virus than you", 210, 310, 0.4f, vec4(1.0f, 1.0f, 1.0f, 0.8f), dpi_scale);
		render_text("Green Vaccin: randomly appear & reduce viruses in half", 160, 370, 0.4f, vec4(0.0f, 1.0f, 0.0f, 0.8f), dpi_scale);
		render_text("Press 'S' to Start", window_size.x/3 + 25, 450, 0.6f, vec4(0.5f, 0.7f, 0.7f, a), dpi_scale);
	}

	else {

		// ==============================================================================
		// EARTH

		glBindVertexArray(vertex_array);

		// bind textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, EARTH_IMG);
		glUniform1i(glGetUniformLocation(program, "TEX0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, VIRUS_IMG);
		glUniform1i(glGetUniformLocation(program, "TEX1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, PLAYER_IMG);
		glUniform1i(glGetUniformLocation(program, "TEX2"), 2);


		//지구 그리기

		earth_sphere.update(t);

		GLint uloc;

		uloc = glGetUniformLocation(program, "frag_type");		if (uloc > -1) glUniform1i(uloc, 0);
		uloc = glGetUniformLocation(program, "type");		if (uloc > -1) glUniform1i(uloc, 0);
		uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, earth_sphere.model_matrix);

		// draw calls
		glDrawElements(GL_TRIANGLES, NUM1 * NUM1 * 3, GL_UNSIGNED_INT, nullptr);


		// ==============================================================================
		// VIRUS

		//vertex array for virus
		glGenVertexArrays(1, &virus_mesh->vertex_array);
		glBindVertexArray(virus_mesh->vertex_array);

		//바이러스 그리기
		for (auto& vi : viruses)
		{
			vi.update(t);
			//mesh 활용해서 virus 그리기
			if (virus_mesh && virus_mesh->vertex_buffer) {
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, virus_mesh->vertex_buffer);
				glVertexAttribPointer(
					0,                  // attribute
					3,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					0,                  // stride
					(void*)0            // array buffer offset 
				);
			}
			else printf("fail to bind vertex buffer of virus_mesh\n\n");
			if (virus_mesh && virus_mesh->uv_buffer) {
				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, virus_mesh->uv_buffer);
				glVertexAttribPointer(
					1,                                // attribute
					2,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void*)0                          // array buffer offset
				);
			}
			else printf("fail to bind uv buffer of virus_mesh\n\n");

			GLint uloc;
			uloc = glGetUniformLocation(program, "frag_type");		if (uloc > -1) glUniform1i(uloc, 1);	// set frag_type
			uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, vi.model_matrix);

			// use simple vertex buffering
			glDrawArrays(GL_TRIANGLES, 0, virus_mesh->vertex_list.size());

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
		}

		// ==============================================================================
		// PLAYER
		glGenVertexArrays(1, &player_mesh->vertex_array);
		glBindVertexArray(player_mesh->vertex_array);

		player.update(player.mv, t);

		if (player_mesh && player_mesh->vertex_buffer) {
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, player_mesh->vertex_buffer);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);
		}
		else printf("fail to bind vertex buffer of player_mesh\n\n");
		if (player_mesh && player_mesh->uv_buffer) {
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, player_mesh->uv_buffer);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);
		}
		else printf("fail to bind uv buffer of player_mesh\n\n");

		uloc = glGetUniformLocation(program, "frag_type");		if (uloc > -1) glUniform1i(uloc, 2);	// set frag_type
		uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, player.model_matrix);

		// use simple vertex buffering
		glDrawArrays(GL_TRIANGLES, 0, player_mesh->vertex_list.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		//************************************
		// MEDICINE
		glGenVertexArrays(1, &medicine_mesh->vertex_array);
		glBindVertexArray(medicine_mesh->vertex_array);

		for (auto& m : medicines)
		{
			m.update(t);
			//mesh 활용해서 virus 그리기
			if (medicine_mesh && medicine_mesh->vertex_buffer) {
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, medicine_mesh->vertex_buffer);
				glVertexAttribPointer(
					0,                  // attribute
					3,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					0,                  // stride
					(void*)0            // array buffer offset 
				);
			}
			else printf("fail to bind vertex buffer of virus_mesh\n\n");
			if (medicine_mesh && medicine_mesh->uv_buffer) {
				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, medicine_mesh->uv_buffer);
				glVertexAttribPointer(
					1,                                // attribute
					2,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void*)0                          // array buffer offset
				);
			}
			else printf("fail to bind uv buffer of virus_mesh\n\n");

			GLint uloc;
			uloc = glGetUniformLocation(program, "frag_type");		if (uloc > -1) glUniform1i(uloc, 3);	// set frag_type
			uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, m.model_matrix);
			uloc = glGetUniformLocation(program, "medicine_color");		if (uloc > -1) glUniform4fv(uloc, 1,  m.color);

			// use simple vertex buffering
			glDrawArrays(GL_TRIANGLES, 0, medicine_mesh->vertex_list.size());

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
		}

		// *****************************************
		// text
		dpi_scale = cg_get_dpi_scale();
		
		if (game_over) {
			//글씨 더 두껍게 색깔 예쁜걸로 좀
			render_text("Game Over", 50, 100, 1.0f, vec4(0.5f, 0.8f, 0.2f, 1.0f), dpi_scale);
			render_text("Press 'R' to restart", 50, 125, 0.5f, vec4(0.7f, 0.4f, 0.1f, 0.8f), dpi_scale);
			render_text("Created by SWITCH", 500, 450, 0.5f, vec4(0.7f, 0.4f, 1.0f, 0.8f), dpi_scale);

		}
		if (player.score > 50) {
			render_text("YOU WIN", 100, 100, 1.0f, vec4(0.5f, 0.8f, 0.2f, 1.0f), 1.0f);
			render_text("You kill 50 viruses", 100, 125, 0.5f, vec4(0.7f, 0.4f, 0.1f, 0.8f), dpi_scale);
			render_text("Created by SWITCH", 500, 450, 0.5f, vec4(0.7f, 0.4f, 1.0f, 0.8f), dpi_scale);
			win = true;
		}
	}
	//set_text = false;
	glfwSwapBuffers(window);
}

void reshape( GLFWwindow* window, int width, int height )
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width,height);
	glViewport( 0, 0, width, height );
}

void print_help()
{
	/* 
	printf( "[help]\n" );
	printf( "- press ESC or 'q' to terminate the program\n" );
	printf( "- press F1 or 'h' to see help\n" );
	printf( "- right button or shift + left button for zooming\n");
	printf( "- middle button or controll + left button for paning\n");
	printf( "- press Home to reset camera\n" );
	printf( "\n" ); */
}

std::vector<vertex> create_sphere_vertices()
{
	std::vector<vertex> v = { { vec3(0), vec3(0,0,-1.0f), vec2(0.5f) } }; // origin

	for (uint k = 0; k <= NUM2; k++)
	{
		float theta = PI * k / float(NUM2);
		for (uint j = 0; j <= NUM1; j++) {
			float phi = 2.0f * PI * j / float(NUM1);
			v.push_back({ vec3(sin(theta) * cos(phi) , sin(theta) * sin(phi), cos(theta)), vec3(sin(theta) * cos(phi) , sin(theta) * sin(phi), cos(theta)), vec2(phi / (2 * PI), 1 - theta / PI) });
		}
	}
	return v;
}
 
void update_vertex_buffer(const std::vector<vertex>& vertices)
{
	static GLuint vertex_buffer = 0;	// ID holder for vertex buffer
	static GLuint index_buffer = 0;		// ID holder for index buffer

	// clear and create new buffers
	if (vertex_buffer)	glDeleteBuffers(1, &vertex_buffer);	vertex_buffer = 0;
	if (index_buffer)	glDeleteBuffers(1, &index_buffer);	index_buffer = 0;

	// check exceptions
	if (vertices.empty()) { printf("[error] vertices is empty.\n"); return; }

	// create buffers
	std::vector<uint> indices;

	//이해 못했으니 건들지 말것!
	for (uint k = 0; k < (NUM2+1) * NUM1; k++)
	{
		if (k < NUM2 * NUM1 - (NUM2 + 1)) {
			indices.push_back(k); 
			indices.push_back(k + NUM1 + 1);
			indices.push_back(k + 1);

			indices.push_back(k + 1);  
			indices.push_back(k + NUM1 + 1);
			indices.push_back(k + NUM1 + 2);
		}
		else {
			indices.push_back(k);
			indices.push_back((NUM2 + 1) * NUM1 - 1);
			indices.push_back(k + 1);
		}
	}
	// generation of vertex buffer: use vertices as it is
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
 
	// geneation of index buffer
	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indices.size(), &indices[0], GL_STATIC_DRAW);
	
	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if (vertex_array) glDeleteVertexArrays(1, &vertex_array);
	vertex_array = cg_create_vertex_array(vertex_buffer, index_buffer);
	if (!vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return; }
	
}

myMesh* obj_loader(const char* path) {

	FILE* fp = fopen(path, "r");
	if (fp == NULL) {
		printf("Impossible to open the file\n");
		return nullptr;
	}

	myMesh* new_mesh = new myMesh();

	std::vector<int> vertexIndices, uvIndices, normalIndices;
	std::vector<vec3> temp_vertices;
	std::vector<vec2> temp_uvs;
	std::vector<vec3> temp_normals;

	while (!feof(fp)) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(fp, "%s", lineHeader);

		if (strcmp(lineHeader, "v") == 0) {
			vec3 vertex;
			fscanf(fp, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			vec2 uv;
			fscanf(fp, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			vec3 normal;
			fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			int vertexIndex[4], uvIndex[4], normalIndex[4];
			int matches = fscanf(fp, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2], &vertexIndex[3], &uvIndex[3], &normalIndex[3]);
			//printf("matches : %d\n\n", matches);
			if (matches == 12) {
				//printf("%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", vertexIndex[0], uvIndex[0], normalIndex[0], vertexIndex[1], uvIndex[1], normalIndex[1], vertexIndex[2], uvIndex[2], normalIndex[2], vertexIndex[3], uvIndex[3], normalIndex[3]);
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[3]);

				vertexIndices.push_back(vertexIndex[3]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);

				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[3]);

				uvIndices.push_back(uvIndex[3]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);

				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[3]);

				normalIndices.push_back(normalIndex[3]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
			else if (matches == 9) {
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
		}
	}
	//printf("vertexIndices size = %d\n", vertexIndices.size());
	for (uint i = 0;i < vertexIndices.size();i++) {
		int vertexIndex = vertexIndices[i];
		vec3 vertex = temp_vertices[vertexIndex - 1];
		new_mesh->vertex_list.push_back(vertex);
		new_mesh->index_list.push_back(vertexIndex - 1);
	}
	for (uint i = 0;i < uvIndices.size();i++) {
		int uvIndex = uvIndices[i];
		vec2 uv = temp_uvs[uvIndex - 1];
		new_mesh->uv_list.push_back(uv);
	}
	for (uint i = 0;i < normalIndices.size();i++) {
		int normalIndex = normalIndices[i];
		vec3 normal = temp_normals[normalIndex - 1];
		new_mesh->normal_list.push_back(normal);
	}

	// create a vertex buffer
	glGenBuffers(1, &new_mesh->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, new_mesh->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * new_mesh->vertex_list.size(), &new_mesh->vertex_list[0], GL_STATIC_DRAW);

	glGenBuffers(1, &new_mesh->uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, new_mesh->uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, new_mesh->uv_list.size() * sizeof(vec2), &new_mesh->uv_list[0], GL_STATIC_DRAW);
	
	
	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	new_mesh->vertex_array = cg_create_vertex_array(new_mesh->vertex_buffer);
	if (!new_mesh->vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return nullptr; }
	
	fclose(fp);
	

	return new_mesh;

}

void keyboard( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if(action==GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_H || key == GLFW_KEY_F1)	print_help();
		else if (key == GLFW_KEY_R && (game_over ||win))					restart = true;
		else if (key == GLFW_KEY_S)					start = false;
		else if (key == GLFW_KEY_W)
		{
			b_wireframe = !b_wireframe;
			glPolygonMode(GL_FRONT_AND_BACK, b_wireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode\n", b_wireframe ? "wireframe" : "solid");
		}
		else if (key == GLFW_KEY_RIGHT) {
			player.mv.right = true;
			//printf("pressed key : ->  %f, %f\n", player.theta, player.phi);
		}
		else if (key == GLFW_KEY_LEFT) {
			player.mv.left = true;
			//printf("pressed key : <- %f, %f\n", player.theta, player.phi);
		}
		else if (key == GLFW_KEY_UP) {
			player.mv.up = true;
			//printf("pressed key : ^ %f, %f\n", player.theta, player.phi);
		}
		else if (key == GLFW_KEY_DOWN) {
			player.mv.down = true;
			//printf("pressed key : v%f, %f\n", player.theta, player.phi);
		}
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_RIGHT)	player.mv.right = false;
		else if (key == GLFW_KEY_LEFT) player.mv.left = false;
		else if (key == GLFW_KEY_UP) player.mv.up = false;
		else if (key == GLFW_KEY_DOWN) player.mv.down = false;
	}
}

void mouse( GLFWwindow* window, int button, int action, int mods )
{
	if(button==GLFW_MOUSE_BUTTON_LEFT)
	{
		dvec2 pos; glfwGetCursorPos(window,&pos.x,&pos.y);
		vec2 npos = cursor_to_ndc( pos, window_size );

		if(action==GLFW_PRESS)			tb.begin( cam.view_matrix, npos );
		else if(action==GLFW_RELEASE)	tb.end();
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_LEFT &&(mods&GLFW_MOD_SHIFT))
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		vec2 npos = cursor_to_ndc(pos, window_size);

		if (action == GLFW_PRESS)		tb.begin_zoom(cam.view_matrix, npos);
		else if (action == GLFW_RELEASE) tb.end_zoom();
	}

	if (button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_LEFT && (mods & GLFW_MOD_CONTROL)) {
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
			
		vec2 npos = cursor_to_ndc(pos, window_size);
		if (action == GLFW_PRESS) 	tb.begin_panning(cam.view_matrix, npos);

		else if (action == GLFW_RELEASE) tb.end_panning();
	}
}

void motion( GLFWwindow* window, double x, double y )
{
	vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);

	if (tb.is_tracking()) {
		cam.view_matrix = tb.update(npos);
	}
	if (tb.b_zooming) {
		cam.view_matrix = tb.update_zoom(npos);
	}
	if (tb.b_panning)
		cam.view_matrix = tb.update_panning(npos);

	else return;
}

GLuint create_texture(const char* image_path, bool b_mipmap)
{
	// load the image with vertical flipping
	image* img = cg_load_image(image_path); if (!img) return -1;
	int w = img->width, h = img->height;

	// create a src texture (lena texture)
	GLuint texture; glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img->ptr);
	if (img) delete img; // release image

	// build mipmap
	if (b_mipmap && glGenerateMipmap)
	{
		int mip_levels = 0; for (int k = max(w, h); k; k >>= 1) mip_levels++;
		for (int l = 1; l < mip_levels; l++)
			glTexImage2D(GL_TEXTURE_2D, l, GL_RGB8, max(1, w >> l), max(1, h >> l), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	// set up texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, b_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

	return texture;
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable(GL_BLEND);
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_DEPTH_TEST );								// turn on depth tests
	glEnable(GL_TEXTURE_2D);			// enable texturing
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);		// notify GL the current texture slot is 0

	//unit_sphere_vertices = std::move(create_sphere_vertices());
	update_vertex_buffer(create_sphere_vertices());
	
	virus_mesh = obj_loader(virus_mesh_path);
	if (virus_mesh == nullptr) {
		printf("fail to load mesh (virus mesh)"); return false;
	}

	player_mesh = obj_loader(player_mesh_path);
	if (player_mesh == nullptr) {
		printf("fail to load mesh(player_mesh)"); return false;
	}

	medicine_mesh = obj_loader(medicine_mesh_path);
	if (medicine_mesh == nullptr) {
		printf("fail to load mesh(medicine_mesh)"); return false;
	}


	EARTH_IMG = create_texture(earth_image_path, false);		if (EARTH_IMG == -1) return false;
	VIRUS_IMG = create_texture(virus_image_path, true);		if (VIRUS_IMG == -1)	return false;
	PLAYER_IMG = create_texture(player_image_path, false);		if (PLAYER_IMG == -1)	return false;

	engine = irrklang::createIrrKlangDevice();
	if (!engine) return false;

	//add sound source from the sound file
	//wave_src = engine->addSoundSourceFromFile(wave_path);
	mp3_src = engine->addSoundSourceFromFile(mp3_path);
	eat_sound_src = engine->addSoundSourceFromFile(eat_sound_path);

	//set default volume
//	wave_src->setDefaultVolume(0.5f);
	mp3_src->setDefaultVolume(0.5f);
	eat_sound_src->setDefaultVolume(0.5f);

	//play the sound file
	engine->play2D(mp3_src, true);

	printf("> playing %s\n", "mp3");

	if (!init_text()) return false;

	return true;
}

void user_finalize()
{
	engine->drop();
}

int main( int argc, char* argv[] )
{
	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return 1; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return 1; }	// version and extensions

	// initializations and validations
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return 1; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movement

	// enters rendering/event loop
	for( frame=0; !glfwWindowShouldClose(window); frame++ )
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}
