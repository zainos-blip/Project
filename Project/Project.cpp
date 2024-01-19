#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ctime>


const int SCR_WIDTH = 400;
const int SCR_HEIGHT = 400;

const int cols = 30;     
const int rows = 30;    

bool m_previous = false;
bool s_previous = false;

enum states {
    PREMAZE,
    GENMAZE,
    PRESOLVE,
    SOLVING,
    SOLVED
};


int state = PREMAZE;

class Spot : public sf::Drawable
{
private:
    sf::Color col = sf::Color::White;
public:
    bool visited = false;

    float f = 0;
    float g = 0;
    float h = 0;
    
    float i = 0;
    float j = 0;
    
    std::vector<bool> walls = {true, true, true, true};
    
    std::vector<Spot*> neighbors;
    Spot* previous = 0;
    
    
    void addNeighbors(std::vector<std::vector<Spot>> &cells) {
        neighbors.clear();
        if (i < cols - 1) {
            if (!cells[i + 1][j].visited)
                neighbors.push_back(&cells[i + 1][j]);
        }
        if (i > 0) {
            if (!cells[i - 1][j].visited)
                neighbors.push_back(&cells[i - 1][j]);
        }
        if (j < rows - 1) {
            if (!cells[i][j + 1].visited)
                neighbors.push_back(&cells[i][j + 1]);
        }
        if (j > 0) {
            if (!cells[i][j - 1].visited)
                neighbors.push_back(&cells[i][j - 1]);
        }
    }
    
    void addMazeNeighbors(std::vector<std::vector<Spot>> &cells) {
        neighbors.clear();
        if (i < cols - 1 && !walls[1])
            neighbors.push_back(&cells[i + 1][j]);
        if (i > 0 && !walls[3])    
            neighbors.push_back(&cells[i - 1][j]);
        if (j < rows - 1 && !walls[2])
            neighbors.push_back(&cells[i][j + 1]);
        if (j > 0 && !walls[0])
            neighbors.push_back(&cells[i][j - 1]);
    }
    
    void setij(int x, int y) {
        i = x;
        j = y;
    }
    
    void setCol(sf::Color new_col) {
        col = new_col;
    }
    
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        int cell_w = SCR_WIDTH / cols;
        int cell_h = SCR_HEIGHT / rows;
        sf::RectangleShape shape(sf::Vector2f(cell_w, cell_h));
        shape.move(sf::Vector2f(i * cell_w, j * cell_h));
        shape.setFillColor(col);
        target.draw(shape);
        
        
        if (walls[0]) { // top wall
            sf::VertexArray wall(sf::Lines, 2);
            wall[0].position = sf::Vector2f(i * cell_w, j * cell_h);
            wall[0].color = sf::Color::Black;
            wall[1].position = sf::Vector2f((i + 1) * cell_w, j  * cell_h);
            wall[1].color = sf::Color::Black;
            target.draw(wall);
        }
        if (walls[1]) { // right wall
            sf::VertexArray wall(sf::Lines, 2);
            wall[0].position = sf::Vector2f((i + 1) * cell_w, j * cell_h);
            wall[0].color = sf::Color::Black;
            wall[1].position = sf::Vector2f((i + 1) * cell_w, (j + 1)  * cell_h);
            wall[1].color = sf::Color::Black;
            target.draw(wall);
        }
        if (walls[2]) { // bottom wall
            sf::VertexArray wall(sf::Lines, 2);
            wall[0].position = sf::Vector2f(i * cell_w, (j + 1) * cell_h);
            wall[0].color = sf::Color::Black;
            wall[1].position = sf::Vector2f((i + 1) * cell_w, (j + 1)  * cell_h);
            wall[1].color = sf::Color::Black;
            target.draw(wall);
        }
        if (walls[3]) { // left wall
            sf::VertexArray wall(sf::Lines, 2);
            wall[0].position = sf::Vector2f(i * cell_w, j * cell_h);
            wall[0].color = sf::Color::Black;
            wall[1].position = sf::Vector2f(i * cell_w, (j + 1)  * cell_h);
            wall[1].color = sf::Color::Black;
            target.draw(wall);
        }
    }
};

std::vector<Spot> tmp(rows);
std::vector<std::vector<Spot>> grid(cols, tmp);

std::vector<Spot*> openSet;
std::vector<Spot*> closedSet;

std::vector<Spot*> cell_stack;

Spot* start = &grid[0][0];
Spot* end = &grid[cols - 1][rows - 1];

void addToOpenSet(Spot* cell) {
    openSet.push_back(cell);
    cell->setCol(sf::Color(180,255,180));
}

void addToClosedSet(Spot* cell) {
    closedSet.push_back(cell);
    cell->setCol(sf::Color::Red);
}


void removeFromArray(Spot* cell, std::vector<Spot*> &vect) {
    for (int i = 0; i < (int)vect.size(); i++) {
        if (vect[i] == cell) {
            vect.erase(vect.begin() + i);
        }
    }
}

bool arrayContainsElem(Spot* cell, std::vector<Spot*> &vect) {
    for (int i = 0; i < (int)vect.size(); i++) {
        if (vect[i] == cell) {
            return true;
        }
    }
    return false;
}

void removeWall(Spot* a, Spot* b) {
    int x = a->i - b->i;
    int y = a->j - b->j;
    if (x == 1) {
        a->walls[3] = false;
        b->walls[1] = false;
    }
    if (x == -1) {
        a->walls[1] = false;
        b->walls[3] = false;
    }
    if (y == 1) {
        a->walls[0] = false;
        b->walls[2] = false;
    }
    if (y == -1) {
        a->walls[2] = false;
        b->walls[0] = false;
    }
}

bool checkWall(Spot* a, Spot* b) {
    int x = a->i - b->i;
    int y = a->j - b->j;
    if (x == 1) {
        return a->walls[3];
    }
    if (x == -1) {
        return a->walls[1];
    }
    if (y == 1) {
        return a->walls[0];
    }
    return a->walls[2];
}

float heuristic(Spot* a, Spot* b) {
    return std::sqrt((a->i - b->i)*(a->i - b->i) + (a->j - b->j)*(a->j - b->j));    
}

void checkInput() {
    bool m_current = sf::Keyboard::isKeyPressed(sf::Keyboard::M);
    bool s_current = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
    
    if (!m_previous && m_current) {
        // M key was pressed, generate maze
        if (state == PREMAZE)
            state = GENMAZE;
    }
    if (!s_previous && s_current) {
        // S key was pressed, solve maze
        if (state == PRESOLVE) {
            addToOpenSet(start);    
            state = SOLVING;
        }
    }
}



int main()
{
    sf::RenderWindow window(sf::VideoMode(SCR_WIDTH, SCR_HEIGHT), "maze solver");
    //window.setFramerateLimit(144);
    
    
    std::srand(std::time(nullptr));
    
    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < rows; j++) {
            grid[i][j].setij(i, j);
        }
    }
      
    Spot* current = &grid[0][0];
    current->visited = true;
    Spot* next = nullptr;
    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }


        window.clear();
 
        checkInput();
        
        if (state == GENMAZE) {
            current->addNeighbors(grid);
            if (current->neighbors.size() > 0) {
                float r = static_cast <float> (std::rand()) / static_cast <float> (RAND_MAX);
                int i = std::floor(r * current->neighbors.size());
                next = current->neighbors[i];
                current->neighbors.erase(current->neighbors.begin() + i);
            }
            if (next != nullptr) {
                next->visited = true;
                
                cell_stack.push_back(current);
                
                removeWall(current, next);
                current = next;
                next = nullptr;
            } else {
                if (cell_stack.size() == 0) {
                    // done!
                    state = PRESOLVE;
                    for (int i = 0; i < cols; i++) {
                        for (int j = 0; j < rows; j++) {
                            grid[i][j].addMazeNeighbors(grid);
                        }
                    }
                } else {
                    current = cell_stack[cell_stack.size() - 1];
                    cell_stack.pop_back();
                }   
            }
        }
        if (state == SOLVING){
            if (openSet.size() > 0) {
                // we can keep going
                int winner = 0;
                for (int i = 0; i < (int)openSet.size(); i++) {
                    if (openSet[i]->f < openSet[winner]->f) {
                        winner = i;
                    }
                }
                
                current = openSet[winner];
                
                if (openSet[winner] == end) {
                    std::cout << "Done!" << std::endl;
                    state = SOLVED;
                }
                
                //remove current from open set
                removeFromArray(current, openSet);
                addToClosedSet(current);
                
                for (int i = 0; i < (int)current->neighbors.size(); i++) {          
                    if (!arrayContainsElem(current->neighbors[i], closedSet) && !checkWall(current, current->neighbors[i])/* && !current->neighbors[i]->wall */) {     
                        float tempG = current->g + 1;
                        
                        if (arrayContainsElem(current->neighbors[i], openSet)) {    
                            if (tempG < current->neighbors[i]->g) {
                                current->neighbors[i]->g = tempG;                   
                                current->neighbors[i]->previous = current;
                            }
                        } else {
                            current->neighbors[i]->g = tempG;                       
                            addToOpenSet(current->neighbors[i]);
                            current->neighbors[i]->g = heuristic(current->neighbors[i], end);
                            current->neighbors[i]->previous = current;
                        }
                        current->neighbors[i]->f = current->neighbors[i]->g + current->neighbors[i]->h;
                    }
                }
                
            } else {
                // no solution
                std::cout << "no solution" << std::endl;
                state = SOLVED;
            }
            
            for (int i = 0; i < (int)closedSet.size(); i++) {
                closedSet[i]->setCol(sf::Color(255,180,180));
            }
        }

        if (state == SOLVING || state == SOLVED) {
            while (current->previous != 0) {
                current->setCol(sf::Color(180,180,255));
                current = current->previous;
            }
            current->setCol(sf::Color(180,180,255));
        }
        
        
    
        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                if (state == GENMAZE) {
                    if (grid[i][j].visited)
                        //grid[i][j].setCol(sf::Color::Magenta);
                        grid[i][j].setCol(sf::Color(255,180,255));
                    current->setCol(sf::Color::Blue);
                    grid[i][j].setCol(sf::Color::White);
                }
                window.draw(grid[i][j]);
            }
        }
        
        
      
        window.display();
    }

    return 0;
}