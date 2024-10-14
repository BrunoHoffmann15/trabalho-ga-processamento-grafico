/* 
 *
 * Trabalho GA - 2024/02 - Bel Cogo, Bruno Hoffmann e João Accorsi
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <random>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#include <stb_image/stb_image.h>

using namespace glm;

struct Sprite{
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions;
    float angle;
    // Para controle da animação
    int nAnimations, nFrames;
    int iAnimation, iFrame;
    vec2 d;
    float FPS;
    float lastTime;
    vec2 pMin; // Minimum coordinates (top-left corner)
    vec2 pMax; // Maximum coordinates (bottom-right corner)

    // Função de inicialização
    void setupSprite(int texID, vec3 position, vec3 dimensions, int nFrames, int nAnimations, vec2 pMin, vec2 pMax);
    vec2 getPMin() const { return vec2(position.x - (dimensions.x / 2), position.y - (dimensions.y / 2)); }
    vec2 getPMax() const { return vec2(position.x + (dimensions.x / 2), position.y + (dimensions.y / 2)); }
};

// Vetor de meteoros
std::vector<Sprite> meteors; 

// Estados do Jogo
enum GameState {
		BEFORE_START,
    RUNNING,
    GAME_OVER
};

GameState gameState = BEFORE_START; // Initialize the game state

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int loadTexture(string filePath, int &imgWidth, int &imgHeight);
void drawSprite(Sprite spr, GLuint shaderID);

// Colisão
bool checkCollision(Sprite &one, Sprite &two);
void updateSpriteBounds(Sprite &spr);

// Reset Game
void resetGame(Sprite &spaceship, std::vector<Sprite> &meteors);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 400\n"
                                     "layout (location = 0) in vec3 position;\n"
                                     "layout (location = 1) in vec2 texc;\n"
                                     "uniform mat4 projection;\n"
                                     "uniform mat4 model;\n"
                                     "out vec2 texCoord;\n"
                                     "void main()\n"
                                     "{\n"
                                     "    gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);\n"
                                     "    texCoord = vec2(texc.s, 1.0 - texc.t);\n"
                                     "}\0";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 400\n"
                                      "in vec2 texCoord;\n"
                                      "uniform sampler2D texBuffer;\n"
                                      "uniform vec2 offsetTex;\n"
                                      "out vec4 color;\n"
                                      "void main()\n"
                                      "{\n"
                                      "    color = texture(texBuffer, texCoord + offsetTex);\n"
                                      "}\n\0";

float vel = 1.2;

bool keys[1024] = {false};

bool collision = false;

// Função MAIN
int main(){
    // Inicialização da GLFW
    glfwInit();

    // Muita atenção aqui: alguns ambientes não aceitam essas configurações
    // Você deve adaptar para a versão do OpenGL suportada por sua placa
    // Sugestão: comente essas linhas de código para desobrir a versão e
    // depois atualize (por exemplo: 4.5 com 4 e 5)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Essencial para computadores da Apple
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Spaceship Game!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    // Obtendo as informações de versão
    const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
    const GLubyte *version = glGetString(GL_VERSION);      /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Gerando um buffer simples, com a geometria de um triângulo
    // Sprite do fundo da cena
    Sprite background, spaceship, meteor, gameOver, startGame;
    // Carregando uma textura (recebendo seu ID)

    // Inicializando a sprite do background
    int imgWidth, imgHeight;
    int texID = loadTexture("textures/space.jpg", imgWidth, imgHeight);
    background.setupSprite(texID, vec3(400.0, 300.0, 0.0), vec3(imgWidth * 0.2, imgHeight * 0.2, 1.0), 1, 1, vec2(0.0, 0.0), vec2(0.0, 0.0));

    // Inicializando a sprite da nave
    texID = loadTexture("./textures/spaceship.png", imgWidth, imgHeight);
    spaceship.setupSprite(texID, vec3(100.0, 300.0, 0.0), vec3(imgWidth * 0.1, imgHeight * 0.1, 1.0), 1, 1, vec2(0.0, 0.0), vec2(0.0, 0.0));

    // Inicializando a sprite do meteoro
	int numMeteors = 5; // Number of meteors
	for (int i = 0; i < numMeteors; i++) {
		Sprite meteor;
		texID = loadTexture("./textures/meteor.png", imgWidth, imgHeight);
		meteor.setupSprite(texID, vec3(500.0 + i * 100, 300.0, 0.0), vec3(imgWidth * 0.2, imgHeight * 0.2, 1.0), 1, 1, vec2(0.0, 0.0), vec2(0.0, 0.0));
		
		// Randomize the Y position for each meteor
		meteor.position.y = rand() % (HEIGHT - (int)(meteor.dimensions.y * 2)) + (int)(meteor.dimensions.y);
		
		meteors.push_back(meteor); // Add meteor to the vector
	}

    texID = loadTexture("textures/game-over.png", imgWidth, imgHeight);
    gameOver.setupSprite(texID, vec3(400.0, 300.0, 0.0), vec3(imgWidth * 0.5, imgHeight * 0.5, 1.0), 1, 1, vec2(0.0, 0.0), vec2(0.0, 0.0));

		texID = loadTexture("textures/start-game.png", imgWidth, imgHeight);
		startGame.setupSprite(texID, vec3(400.0, 300.0, 0.0), vec3(imgWidth, imgHeight, 1.0), 1, 1, vec2(0.0, 0.0), vec2(0.0, 0.0));

		glUseProgram(shaderID);

		// Enviando a cor desejada (vec4) para o fragment shader
    // Utilizamos a variáveis do tipo uniform em GLSL para armazenar esse tipo de info
    // que não está nos buffers
    glUniform1i(glGetUniformLocation(shaderID, "texBuffer"), 0);

    // Matriz de projeção ortográfica
    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Ativando o primeiro buffer de textura da OpenGL
    glActiveTexture(GL_TEXTURE0);

    // Habilitar a transparência
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Habilitar o teste de profundidade
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window)) {

		// Poll for events (input)
		glfwPollEvents();

		// Clear the color buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Background color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vec2 offsetTex = vec2(0.0, 0.0);
		glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

		// Draw the background
		drawSprite(background, shaderID);

		if (gameState == BEFORE_START)
		{
			drawSprite(startGame, shaderID);

			// Tecla Espaço
			if (keys[GLFW_KEY_ENTER])
			{
				gameState = RUNNING;
			}
		}
		else if (gameState == RUNNING)
		{
			// Movement controls
			if (keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_A])
				spaceship.position.x -= vel;
			if (keys[GLFW_KEY_RIGHT] || keys[GLFW_KEY_D])
				spaceship.position.x += vel;
			if (keys[GLFW_KEY_UP] || keys[GLFW_KEY_W])
				spaceship.position.y += vel;
			if (keys[GLFW_KEY_DOWN] || keys[GLFW_KEY_S])
				spaceship.position.y -= vel;

			// Atualização meteoros na tela
			for (size_t i = 0; i < meteors.size(); i++) {
				meteors[i].position.x -= vel; // Move each meteor left

				// If the meteor moves off-screen, reset its position to a random location on the right
				if (meteors[i].position.x < -100) { // Adjust as needed for your sprite's width
					meteors[i].position.x = WIDTH;
					meteors[i].position.y = rand() % (HEIGHT - (int)(meteors[i].dimensions.y * 2)) + (int)(meteors[i].dimensions.y); // New random Y position
				}

				// Update bounds for collision detection
				updateSpriteBounds(meteors[i]);

				// Check for collision with the spaceship
				collision = checkCollision(spaceship, meteors[i]);

				if (collision) {
					gameState = GAME_OVER;
					break; // Exit the loop if collision occurs
				}
			}

			// Draw all meteors
			for (size_t i = 0; i < meteors.size(); i++) {
				drawSprite(meteors[i], shaderID);
			}

			// Atualiza as distâncias para o check decolisão
			updateSpriteBounds(spaceship);
			updateSpriteBounds(meteor);

			// Check for collision
			collision = checkCollision(spaceship, meteor);

			if (collision) {
				gameState = GAME_OVER; 
			}

			drawSprite(spaceship, shaderID);
			drawSprite(meteor, shaderID);
		}
		else if (gameState == GAME_OVER)
		{
			drawSprite(gameOver, shaderID);

			// Tecla Espaço
			if (keys[GLFW_KEY_SPACE]) {
				gameState = BEFORE_START;
				resetGame(spaceship, meteors);
			}
		}

		// Swap buffers to display the drawn frame
		glfwSwapBuffers(window);
	}

		// Limpeza de memória
		glDeleteVertexArrays(1, &background.VAO);
		glDeleteVertexArrays(1, &spaceship.VAO);
		glDeleteVertexArrays(1, &meteor.VAO);
		glDeleteVertexArrays(1, &gameOver.VAO);
		glfwTerminate();

		return 0;
	}

	void resetGame(Sprite &spaceship, std::vector<Sprite> &meteors) {
		spaceship.position = vec3(100.0f, 300.0f, 0.0f);

		for (size_t i = 0; i < meteors.size(); i++) {
			meteors[i].position.x = WIDTH; // Start off-screen to the right
			meteors[i].position.y = rand() % (HEIGHT - (int)(meteors[i].dimensions.y * 2)) + (int)(meteors[i].dimensions.y); // New random Y position
			updateSpriteBounds(meteors[i]); // Update bounds after resetting position
		}
	}

	// Função de configuração do shader
	int setupShader(){

		// Compilando Vertex Shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		// Compilando Fragment Shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);

		// Criando o shader program
		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		// Removendo os shaders após a vinculação
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return shaderProgram;
	}

// Função para carregar a textura
int loadTexture(string filePath, int &imgWidth, int &imgHeight){

    unsigned char *image = stbi_load(filePath.c_str(), &imgWidth, &imgHeight, 0, 4);
    GLuint textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Definindo os parâmetros de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Carregando a textura
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);

    return textureID;
}

// Função para desenhar a sprite
void drawSprite(Sprite spr, GLuint shaderID){
    glBindTexture(GL_TEXTURE_2D, spr.texID);
    glBindVertexArray(spr.VAO);
    
    // Matriz de modelo
    mat4 model = translate(mat4(1.0f), spr.position);
    model = scale(model, spr.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

// Função de configuração do sprite
void Sprite::setupSprite(int texID, vec3 position, vec3 dimensions, int nFrames, int nAnimations, vec2 pMin, vec2 pMax){
    this->texID = texID;
    this->dimensions = dimensions;
    this->position = position;
    this->nAnimations = nAnimations;
    this->nFrames = nFrames;
    this->iAnimation = 0;
    this->iFrame = 0;

    this->d.s = 1.0f / (float)nFrames;
    this->d.t = 1.0f / (float)nAnimations;

    // Removed the manual setting of pMin and pMax from here
    // We'll call updateSpriteBounds in the main loop after modifying the position

    GLfloat vertices[] = {
        // x    y    z    s    t
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // V0
        -0.5f,  0.5f, 0.0f, 0.0f, d.t,   // V1
         0.5f, -0.5f, 0.0f, d.s, 0.0f,   // V2
         0.5f,  0.5f, 0.0f, d.s, d.t     // V3
    };

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    this->VAO = VAO;
    this->FPS = 12.0f;
    this->lastTime = 0.0f;

    // Initialize bounds
    updateSpriteBounds(*this);
}


// Função para verificar colisão entre dois sprites
bool checkCollision(Sprite &one, Sprite &two){
    return (one.getPMax().x >= two.getPMin().x && one.getPMin().x <= two.getPMax().x &&
            one.getPMax().y >= two.getPMin().y && one.getPMin().y <= two.getPMax().y);
}

// Atualiza os limites da sprite
void updateSpriteBounds(Sprite &spr){
    spr.pMin = spr.getPMin();
    spr.pMax = spr.getPMax();
}