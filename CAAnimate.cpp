#include "emp/web/Animate.hpp"
#include "emp/web/web.hpp"

emp::web::Document doc{"target"};

class CAAnimator : public emp::web::Animate {

// grid width and height
const int num_h_boxes = 10;
const int num_w_boxes = 15;
const double RECT_SIDE = 25;
const double width{num_w_boxes * RECT_SIDE};
const double height{num_h_boxes * RECT_SIDE};


//some vectors to hold information about the CA
std::vector<std::vector<int>> cells;

// vector to temporarily store cells for next generation
std::vector<std::vector<int>> next_generation;
        

// where we'll draw
emp::web::Canvas canvas{width, height, "canvas"};

public:

    CAAnimator() {
        // shove canvas into the div
        // along with some control buttons
        doc << canvas;
        doc << GetToggleButton("Toggle");
        doc << GetStepButton("Step");

        //fill the vectors with 0 to start
        cells.resize(num_w_boxes, std::vector<int>(num_h_boxes, 0));
        next_generation.resize(num_w_boxes, std::vector<int>(num_h_boxes, 0));

        AddGlider();
    }

    void DoFrame() override {
        canvas.Clear();

        for (int x = 0; x < num_w_boxes; x++){
             for (int y = 0; y < num_h_boxes; y++) {

                if (cells[x][y] == 0) {
                    //Draw a rectangle on the canvas filled with white and outlined in black
                    canvas.Rect(x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE, "white", "black");
                } else {
                    //Draw a rectangle filled with black
                    canvas.Rect(x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE, "black", "black");
                }
                
                
            }
        }

        // update all cell values using update rules
        ComputeNextGeneration();
        UpdateGridFromNextGeneration();

    }

    // get the value of the cell at (x, y), wrapping around if they are out of bounds
    int GetCell(int x, int y) {
        return cells[emp::Mod(x, num_w_boxes)][emp::Mod(y, num_h_boxes)];
    }

    // a cell is considered an alive neighbor if it differs in each coordinate from (x, y) by at most 1 and has value > 0
    int NumberOfAliveNeighbors(int x, int y) {
        int count = 0;
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                // don't count this cell
                if ((ox != 0 || oy != 0) && GetCell(x + ox, y + oy) > 0) {
                    count++;
                }
            }
        }
        return count;
    }

    // apply the update rule to each cell to determine what it will be in the next generation
    void ComputeNextGeneration() {
        for (int x=0; x<num_w_boxes; x++) {
            for (int y=0; y<num_h_boxes; y++) {
                next_generation[x][y] = UpdateFunction(x, y);
            }
        }
    }

    // applies the newly calculated values to the cells in the grid to step to the next generation
    void UpdateGridFromNextGeneration() {
        for (int x=0; x<num_w_boxes; x++) {
            for (int y=0; y<num_h_boxes; y++) {
                cells[x][y] = next_generation[x][y];
            }
        }
    }

    // returns the state that the cell at (x, y) will change to
    int UpdateFunction(int x, int y) {
        return LifeUpdateFunction(x, y);
    }

    // the update function for Conway's Game of Life
    int LifeUpdateFunction(int x, int y) {
        int number_of_alive_neighbors = NumberOfAliveNeighbors(x, y);
        bool is_alive = GetCell(x, y) > 0;
        if (is_alive) {
            // an alive cell survives if it has 2 or 3 neighbors
            return (number_of_alive_neighbors == 2 || number_of_alive_neighbors == 3) ? 1 : 0;
        } else {
            // a dead cell is born if it has 3 neighbors
            return number_of_alive_neighbors == 3 ? 1 : 0;
        }
    }

    // adds a glider from Conway's Game of Life to the scene at (0, 0) facing southeast
    void AddGlider() {
        cells[1][0] = 1;
        cells[2][1] = 1;
        cells[0][2] = 1;
        cells[1][2] = 1;
        cells[2][2] = 1;
    }


};

CAAnimator animator;

int main() {
    //Have animator call DoFrame once to start
    animator.Step();
}