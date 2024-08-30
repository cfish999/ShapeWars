#include "Game.h"
#include <iostream>
#include <fstream>
#include<sstream>
#include <ctime>
using namespace std;

Game::Game(const string& config) {
	init(config);
}

void Game::init(const string & path) {

	ifstream fin(path);

	// read in the player variables
	fin >> m_playerConfig.SR >> m_playerConfig.CR >> m_playerConfig.S >> m_playerConfig.FR >> m_playerConfig.FG >> m_playerConfig.FB >> m_playerConfig.OR 
		>> m_playerConfig.OG >> m_playerConfig.OB >> m_playerConfig.OT >> m_playerConfig.V;


	fin >> m_enemyConfig.SR >> m_enemyConfig.CR >> m_enemyConfig.SMIN >> m_enemyConfig.SMAX >> m_enemyConfig.OR >> m_enemyConfig.OG >> m_enemyConfig.OB
		>> m_enemyConfig.OT >> m_enemyConfig.VMIN >> m_enemyConfig.VMAX >> m_enemyConfig.L >> m_enemyConfig.SI;

	fin >> m_bulletConfig.SR >> m_bulletConfig.CR >> m_bulletConfig.S >> m_bulletConfig.FR >> m_bulletConfig.FG >> m_bulletConfig.FB >> m_bulletConfig.OR
		>> m_bulletConfig.OG >> m_bulletConfig.OB >> m_bulletConfig.OT >> m_bulletConfig.V >> m_bulletConfig.L;

	fin.close();

	// loads in full screen mode
	m_window.create(sf::VideoMode::getFullscreenModes()[0], "ShapeWars", sf::Style::Fullscreen);
	m_window.setFramerateLimit(60);

	spawnPlayer();
}

void Game::run() {
	
	//attempt to load the font from a file
	if (!m_font.loadFromFile("fonts/Midnight.otf")) {
		cerr << "Could  not load font" << endl;
		exit(-1);
	}

	// makes sure every time the program is run the random numbers we use are differently seeded to be random every time
	srand(time(NULL));
	
	// main game loop
	while (m_running) {

		m_entities.update();

		if (m_paused) {
			sRender();
			sUserInput();
		}
		else {
			sEnemySpawner();
			sMovement();
			sCollision();
			sUserInput();
			sLifespan();
			sRender();
		}

		// increment the current frame 
		m_currentFrame++;
		// decreases the time needed for the special ability cooldown
		m_specialCooldown--;
	}

}


// respawn the player in the middle of the screen
void Game::spawnPlayer() {

	// We create every entity by calling EntityManager.addEntity(tag)
	// This returns a std::shared_ptr<Entity> so we use 'auto' to save typing
	auto entity = m_entities.addEntity("player");


	float mx = m_window.getSize().x / 2.0f; // gets the middle x of the window
	float my = m_window.getSize().y / 2.0f; // gets the middle y of the window

	entity->cTransform = std::make_shared<CTransform>(Vec2(mx, my), Vec2(m_playerConfig.S, m_playerConfig.S), 60.0f);

	entity->cShape = std::make_shared<CShape>(m_playerConfig.SR, m_playerConfig.V, sf::Color(m_playerConfig.FR, m_playerConfig.FG, m_playerConfig.FB),
		sf::Color(m_playerConfig.OR, m_playerConfig.OG, m_playerConfig.OB), m_playerConfig.OT);

	// Add an input component to the player so we can use inputs
	entity->cInput = std::make_shared<CInput>();

	// add a player collision radius
	entity->cCollision = std::make_shared<CCollision>(m_playerConfig.CR);

	// Since we want this entity to be out player,set out Game's player variable to be this entity 
	// This goes slightly against the entity manager paradigm,  but we use the player so much it's worth it
	m_player = entity;

	entity->cLifeSpan = std::make_shared<CLifespan>(0);

	entity->cScore = std::make_shared<CScore>(0);

}

// spawn an enemy at a random position
void Game::spawnEnemy()  {

	// the enemy must be spawned completely within the bounds of the window
	auto entity = m_entities.addEntity("enemy");

	float ex = 0, ey = 0;
	bool safeSpawn = false;

	// makes sure the enemy does not spawn on top of the player
	while (!safeSpawn) {

		// formula is 1r + random number between between size of screen + 2r
		ex = m_enemyConfig.SR + rand() % (m_window.getSize().x - (2 * m_enemyConfig.SR));
		ey = m_enemyConfig.SR + rand() % (m_window.getSize().y - (2 * m_enemyConfig.SR));

		Vec2 distance(m_player->cTransform->pos.x - ex, m_player->cTransform->pos.y - ey);
		float distanceSquared = (distance.x * distance.x) + (distance.y * distance.y);

		float radius = (m_player->cCollision->radius + m_enemyConfig.CR) * (m_player->cCollision->radius + m_enemyConfig.CR);

		// if the two do not intersect then there is no collision
		if (!distanceSquared < radius) {
			// formula is 1r + random number between between size of screen + 2r
			safeSpawn = true;
		}
	}

	float randSpeedX = m_enemyConfig.SMIN + rand() % m_enemyConfig.SMAX;
	float randSpeedY = m_enemyConfig.SMIN + rand() % m_enemyConfig.SMAX;

	entity->cTransform = std::make_shared<CTransform>(Vec2(ex, ey), Vec2(randSpeedX,randSpeedY), 90.0f);

	float vertices = m_enemyConfig.VMIN-1 + rand() % m_enemyConfig.VMAX;

	// store the vertices
	entity->cVertices = std::make_shared<CVertices>(vertices);

	// store the colours
	int fillR = 0 + rand() % 255;
	int fillG = 0 + rand() % 255;
	int fillB = 0 + rand() % 255;

	entity->cRandColour = std::make_shared<CRandColour>(fillR, fillG, fillB);

	entity->cShape = std::make_shared<CShape>(m_enemyConfig.SR,vertices, sf::Color(fillR, fillG, fillB), 
		sf::Color(m_enemyConfig.OR, m_enemyConfig.OG, m_enemyConfig.OB), m_enemyConfig.OT);

	// sets the radius
	entity->cCollision = std::make_shared<CCollision>(m_enemyConfig.CR);

	entity->cLifeSpan = std::make_shared<CLifespan>(0);

	// record when the most recent enemy was spawned
	m_lastEnemySpawnTime = m_currentFrame;

}

// spawns the small enemies when a big one (input entity e) explodes
void Game::spawnSmallEnemies(std::shared_ptr<Entity> e) {

	for (int i = 0; i < e->cVertices->vertices; i++) {
		auto entity = m_entities.addEntity("smallEnemy");

		float angle = 360 / e->cVertices->vertices * i;
		float radians = 2 * PI * (angle / 360);

		entity->cTransform = std::make_shared<CTransform>(Vec2(e->cTransform->pos.x, e->cTransform->pos.y),
			Vec2(e->cTransform->velocity.x * cos(radians), e->cTransform->velocity.y * sin(radians)), 90.0f);

		// stores the colours (prevents memory access violation)
		entity->cRandColour = std::make_shared<CRandColour>(e->cRandColour->fillR, e->cRandColour->fillG, e->cRandColour->fillB);

		entity->cShape = std::make_shared<CShape>(m_enemyConfig.SR / 2 , e->cVertices->vertices, 
			sf::Color(e->cRandColour->fillR, e->cRandColour->fillG, e->cRandColour->fillB),
			sf::Color(m_enemyConfig.OR, m_enemyConfig.OG, m_enemyConfig.OB), m_enemyConfig.OT);

		// store the vertices
		entity->cVertices = std::make_shared<CVertices>(e->cVertices->vertices);

		// sets the radius
		entity->cCollision = std::make_shared<CCollision>(m_enemyConfig.CR/2);

		entity->cLifeSpan = std::make_shared<CLifespan>(m_enemyConfig.L);
	}
	
}

// spawn a bullet from a given entity to a target location
void Game::spawnBullet(std::shared_ptr<Entity> entity, const Vec2& target) {

	auto bullet = m_entities.addEntity("bullet");

	// below code spawns the bullet at the centre of the player
	bullet->cTransform = std::make_shared<CTransform>(Vec2(entity->cTransform->pos.x,entity->cTransform->pos.y), Vec2(m_bulletConfig.S, m_bulletConfig.S), 0);
	bullet->cShape = std::make_shared<CShape>(m_bulletConfig.SR, m_bulletConfig.V, sf::Color(m_bulletConfig.FR, m_bulletConfig.FG, m_bulletConfig.FB),
		sf::Color(m_bulletConfig.OR, m_bulletConfig.OG, m_bulletConfig.OB), m_bulletConfig.OT);

	// sets the lifespan of our bullet
	bullet->cLifeSpan = std::make_shared<CLifespan>(m_bulletConfig.L);
	// sets the radius
	bullet->cCollision = std::make_shared<CCollision>(m_bulletConfig.CR);

	// use target to work out velocity here 
	Vec2 difference(target.x - entity->cTransform->pos.x, target.y - entity->cTransform->pos.y);
	
	float angle = atan2f(difference.x, difference.y);

	int speed = m_bulletConfig.S;
	Vec2 bulletVelocity(speed * sin(angle), speed * cos(angle));


	bullet->cTransform->velocity.x = bulletVelocity.x;
	bullet->cTransform->velocity.y = bulletVelocity.y;


}

void Game::spawnSpecialWeapon(std::shared_ptr<Entity> entity, const Vec2& target) {
	
	// shoots in all directions (10 shots total)
	for (int i = 0; i < 11; i++) {

		auto bullet = m_entities.addEntity("bullet");
		int extraAngle = 36*i;


		float radianAngle = 2 * PI * (extraAngle / 360);
		// below code spawns the bullet at the centre of the player
		bullet->cTransform = std::make_shared<CTransform>(Vec2(entity->cTransform->pos.x, entity->cTransform->pos.y), Vec2(m_bulletConfig.S, m_bulletConfig.S), 0);
		bullet->cShape = std::make_shared<CShape>(m_bulletConfig.SR, m_bulletConfig.V, sf::Color(m_bulletConfig.FR, m_bulletConfig.FG, m_bulletConfig.FB),
			sf::Color(m_bulletConfig.OR, m_bulletConfig.OG, m_bulletConfig.OB), m_bulletConfig.OT);

		// sets the lifespan of our bullet
		bullet->cLifeSpan = std::make_shared<CLifespan>(m_bulletConfig.L);
		// sets the radius
		bullet->cCollision = std::make_shared<CCollision>(m_bulletConfig.CR);

		// use target to work out velocity here 
		Vec2 difference(target.x - entity->cTransform->pos.x, target.y - entity->cTransform->pos.y);

		float angle = atan2f(difference.x, difference.y);

		int speed = m_bulletConfig.S;
		Vec2 bulletVelocity(speed * sin(angle+extraAngle), speed * cos(angle+extraAngle));


		bullet->cTransform->velocity.x = bulletVelocity.x;
		bullet->cTransform->velocity.y = bulletVelocity.y;
	}
}

void Game::sMovement() {

	m_player->cTransform->velocity = { 0,0 };
	
	// determines the player's movement 
	if (m_player->cInput->up) {
		if (m_player->cTransform->pos.y - m_player->cCollision->radius <= 0) {
			m_player->cTransform->velocity.y = 0;
		}
		else {
			m_player->cTransform->velocity.y = -m_playerConfig.S;
		}
	}
	if (m_player->cInput->down) {
		if (m_player->cTransform->pos.y + m_player->cCollision->radius >= m_window.getSize().y) {
			m_player->cTransform->velocity.y = 0;
		}
		else {
			m_player->cTransform->velocity.y = m_playerConfig.S;
		}
	}
	if (m_player->cInput->left) {
		if (m_player->cTransform->pos.x - m_player->cCollision->radius <= 0) {
			m_player->cTransform->velocity.x = 0;
		}
		else {
			m_player->cTransform->velocity.x = -m_playerConfig.S;
		}
	}
	if (m_player->cInput->right) {
		if (m_player->cTransform->pos.x + m_player->cCollision->radius >= m_window.getSize().x) {
			m_player->cTransform->velocity.x = 0;
		}
		else {
			m_player->cTransform->velocity.x = m_playerConfig.S;
		}
	}
	
	// moves all entities
	for (auto e : m_entities.getEntities()) {
		e->cTransform->pos.x += e->cTransform->velocity.x;
		e->cTransform->pos.y += e->cTransform->velocity.y;
	}

}

void Game::sLifespan() {

	for (auto e : m_entities.getEntities()) {
		if (e->cLifeSpan->total > 0) {
			// decrement by 1 every frame
			e->cLifeSpan->remaining--;
			if (e->cLifeSpan->remaining == 0) {
				// dies when its lifetime is up 
				e->destroy();
				break;
			}
			float alpha = (static_cast<float>(e->cLifeSpan->remaining) / e->cLifeSpan->total) ;
			if (e->tag() == "bullet") {
				e->cShape = std::make_shared<CShape>(m_bulletConfig.SR, m_bulletConfig.V, 
					sf::Color(m_bulletConfig.FR * alpha, m_bulletConfig.FG * alpha, m_bulletConfig.FB * alpha),
					sf::Color(m_bulletConfig.OR * alpha, m_bulletConfig.OG * alpha, m_bulletConfig.OB * alpha), m_bulletConfig.OT);
			}
			if (e->tag() == "smallEnemy") {
				e->cShape = std::make_shared<CShape>(m_enemyConfig.SR/ 2, e->cVertices->vertices,
					sf::Color(e->cRandColour->fillR * alpha, e->cRandColour->fillG * alpha, e->cRandColour->fillB * alpha),
					sf::Color(m_enemyConfig.OR * alpha, m_enemyConfig.OG * alpha, m_enemyConfig.OB * alpha), m_enemyConfig.OT);
			}
			
		}
	}

}

void Game::sCollision() {

	// for enemies and bullets
	for (auto e : m_entities.getEntities()) {

		// left wall
		if (e->cTransform->pos.x - e->cCollision->radius <= 0) {
			e->cTransform->velocity.x *= -1.0;
		}
		// top wall
		if (e->cTransform->pos.y - e->cCollision->radius <= 0) {
			e->cTransform->velocity.y *= -1.0;
		}
		// right wall 
		if (e->cTransform->pos.x + e->cCollision->radius >= m_window.getSize().x) {
			e->cTransform->velocity.x *= -1.0;
		}
		// bottom wall
		if (e->cTransform->pos.y + e->cCollision->radius >= m_window.getSize().y) {
			e->cTransform->velocity.y *= -1.0;
		}
		
	}

	// bullet kills small enemy
	for (auto b : m_entities.getEntities("bullet")) {
		for (auto e : m_entities.getEntities("smallEnemy")) {

			Vec2 distance(b->cTransform->pos.x - e->cTransform->pos.x, b->cTransform->pos.y - e->cTransform->pos.y);
			float distanceSquared = (distance.x * distance.x) + (distance.y * distance.y);

			float radius = (b->cCollision->radius + e->cCollision->radius) * (b->cCollision->radius + e->cCollision->radius);

			if (distanceSquared < radius) {
				b->destroy();
				e->destroy();
				m_player->cScore->score += (e->cVertices->vertices * 100) * 2;
			}

		}
	}

	// be sure to use the collision radius, NOT the shape radius
	for (auto b : m_entities.getEntities("bullet")) {
		for (auto e : m_entities.getEntities("enemy")) {
	
			Vec2 distance(b->cTransform->pos.x - e->cTransform->pos.x, b->cTransform->pos.y - e->cTransform->pos.y);
			float distanceSquared = (distance.x * distance.x) + (distance.y * distance.y);

			float radius = (b->cCollision->radius + e->cCollision->radius) * (b->cCollision->radius + e->cCollision->radius);

			if (distanceSquared < radius) {
				b->destroy();
				spawnSmallEnemies(e);
				m_player->cScore->score += e->cVertices->vertices * 100;
				e->destroy();
			}

		}
	}



	// player to big enemy collision
	for (auto e : m_entities.getEntities("enemy")) {
		// collision
		Vec2 distance(m_player->cTransform->pos.x - e->cTransform->pos.x, m_player->cTransform->pos.y - e->cTransform->pos.y);
		float distanceSquared = (distance.x * distance.x) + (distance.y * distance.y);

		float radius = (m_player->cCollision->radius + e->cCollision->radius) * (m_player->cCollision->radius + e->cCollision->radius);

		if (distanceSquared < radius) {
			m_score = m_player->cScore->score;
			m_player->destroy();
			e->destroy();
			// spawns the player back in the centre
			spawnPlayer();
		}
	}

	for (auto e : m_entities.getEntities("smallEnemy")) {
		// collision
		Vec2 distance(m_player->cTransform->pos.x - e->cTransform->pos.x, m_player->cTransform->pos.y - e->cTransform->pos.y);
		float distanceSquared = (distance.x * distance.x) + (distance.y * distance.y);

		float radius = (m_player->cCollision->radius + e->cCollision->radius) * (m_player->cCollision->radius + e->cCollision->radius);

		if (distanceSquared < radius) {
			m_score = m_player->cScore->score;
			m_player->destroy();
			e->destroy();
			// spawns the player back in the centre
			spawnPlayer();
		}
	}
	
}

void Game::sEnemySpawner() {
	
	// spawns enemies with a 1 second delay
	if (m_currentFrame - m_lastEnemySpawnTime == 0 || m_currentFrame % 60 == 0) {
		spawnEnemy();
	} 
	
}

void Game::sRender() {
	
	// clears the window before we redraw
	m_window.clear();

	// draws the players and the enemies currently present 
	for (auto e : m_entities.getEntities()) {
		
		// set the position of the shape based on the entity's transform->pos
		e->cShape->circle.setPosition(e->cTransform->pos.x, e->cTransform->pos.y);

		// set the rotation of the shape based on the entity's transform angle
		e->cTransform->angle += 1.0f;
		e->cShape->circle.setRotation(e->cTransform->angle);

		// draw the entity's circle shape
		m_window.draw(e->cShape->circle);
	}

	// calculating the score
	int score = m_player->cScore->score;
	string text = "Score: ";
	text += std::to_string(score);
	m_text.setString(text);
	m_text.setFont(m_font);
	m_text.setCharacterSize(80);

	if (score >= highscore) {
		highscore = score;
		string highestScore = "Highscore: ";
		highestScore += std::to_string(highscore);
		m_textScore.setString(highestScore);
		m_textScore.setFont(m_font);
		m_textScore.setCharacterSize(80);
	}

	// position the top-left corner of the text so that the text aligns on the bottom
	// text character size is in pixels, so move the text up from the bottom by it's height
	m_text.setPosition(0, 0);
	m_textScore.setPosition(0,80);

	m_window.draw(m_textScore);
	m_window.draw(m_text);

	// Drawing the game name and information to the screen 
	string shapeWars = "Shape Wars!";
	m_gameName.setString(shapeWars);
	m_gameName.setFont(m_font);
	m_gameName.setCharacterSize(80);
	m_gameName.setPosition((m_window.getSize().x*2/5),0);

	string escape = "ESC to QUIT";
	m_helpfulInfo.setString(escape);
	m_helpfulInfo.setFont(m_font);
	m_helpfulInfo.setCharacterSize(50);
	m_helpfulInfo.setPosition(0,m_window.getSize().y*19/20);


	m_window.draw(m_gameName);
	m_window.draw(m_helpfulInfo);

	// displays them on the screen
	m_window.display();
}

void Game::sUserInput() {

	sf::Event event;
	while (m_window.pollEvent(event)) {
		// this event triggers when the window is closed
		if (event.type == sf::Event::Closed) {
			m_running = false;
		}

		// this event is triggered when a key is pressed 
		if (event.type == sf::Event::KeyPressed) {

			switch (event.key.code) {

				case sf::Keyboard::W:
					m_player->cInput->up = true;
					break;
				case sf::Keyboard::S:
					m_player->cInput->down = true;
					break;
				case sf::Keyboard::A:
					m_player->cInput->left = true;
					break;
				case sf::Keyboard::D:
					m_player->cInput->right = true;
					break;
				case sf::Keyboard::P:
					if (m_paused) {
						m_paused = false;
					}
					else {
						m_paused = true;
					}
					break;
				case sf::Keyboard::Escape:
					// closes the window
					m_running = false;
					break;

				default: break;
			}
		}

		// this event is triggered when a key is released 
		if (event.type == sf::Event::KeyReleased) {

			switch (event.key.code) {
			case sf::Keyboard::W:
				m_player->cInput->up = false;
				break;
			case sf::Keyboard::S:
				m_player->cInput->down = false;
				break;
			case sf::Keyboard::A:
				m_player->cInput->left = false;
				break;
			case sf::Keyboard::D:
				m_player->cInput->right = false;
				break;

			default: break;
			}
		}

		if (event.type == sf::Event::MouseButtonPressed) {

			if (event.mouseButton.button == sf::Mouse::Left) {
				spawnBullet(m_player, Vec2(event.mouseButton.x,event.mouseButton.y));
			}

			if (event.mouseButton.button == sf::Mouse::Right) {
				if (m_specialCooldown <= 0) {
					spawnSpecialWeapon(m_player, Vec2(event.mouseButton.x, event.mouseButton.y));
					m_specialCooldown = 120; // 2 second cooldown
				}
			}
		}
	}
}