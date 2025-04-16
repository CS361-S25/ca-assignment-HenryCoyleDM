#include "emp/web/Animate.hpp"
#include "emp/web/web.hpp"
#include "FloatPair.h"

emp::web::Document doc{"target"};

class CAAnimator : public emp::web::Animate {

// grid width and height
const int num_h_boxes = 30;
const int num_w_boxes = 45;
const double RECT_SIDE = 25;
const double width{num_w_boxes * RECT_SIDE};
const double height{num_h_boxes * RECT_SIDE};


//some vectors to hold information about the CA
// I experimented with having two variables per cell, one for the blue value and one for the yellow (red+green)
std::vector<std::vector<FloatPair>> cells;

// vector to temporarily store cells for next generation
std::vector<std::vector<FloatPair>> next_generation;
        

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
        cells.resize(num_w_boxes, std::vector<FloatPair>(num_h_boxes, FloatPair(0, 0)));
        next_generation.resize(num_w_boxes, std::vector<FloatPair>(num_h_boxes, FloatPair(0, 0)));

        // initialize the state
        AddRandomCells();
    }

    void DoFrame() override {
        canvas.Clear();

        for (int x = 0; x < num_w_boxes; x++){
             for (int y = 0; y < num_h_boxes; y++) {
                // draw a rectange with black strokes and filled with the cell's amount of yellow and blue
                canvas.Rect(x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE, emp::ColorRGB(cells[x][y].yellow * 255, cells[x][y].yellow * 255, cells[x][y].blue * 255), "black");
            }
        }

        // update all cell values using update rules
        ComputeNextGeneration();
        UpdateGridFromNextGeneration();
        // this update rule typically leads to interesting structures with a period of 2, so I apply the rule twice per frame to 
        // reduce flashing and make the structures clearer
        ComputeNextGeneration();
        UpdateGridFromNextGeneration();

    }

    // get the value of the cell at (x, y), wrapping around if they are out of bounds
    FloatPair GetCell(int x, int y) {
        return cells[emp::Mod(x, num_w_boxes)][emp::Mod(y, num_h_boxes)];
    }

    // a cell is considered alive if either its blue or yellow values are at least 0.5
    bool IsAlive(int x, int y) {
        return GetCell(x, y).blue > 0.5 || GetCell(x, y).yellow > 0.5;
    }

    // a cell is considered an alive neighbor of (x, y) if it differs in each coordinate from (x, y) by at most 1 and has either its
    // blue value or its yellow value at least 0.5
    int NumberOfAliveNeighbors(int x, int y) {
        int count = 0;
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                // don't count this cell
                if ((ox != 0 || oy != 0) && IsAlive(x + ox, y + oy)) {
                    count++;
                }
            }
        }
        return count;
    }

    // the average blue and yellow values of neighboring cells to (x, y) that are alive
    FloatPair AverageOfAliveNeighbors(int x, int y) {
        int number_of_alive_neighbors = 0;
        float total_blue = 0.0;
        float total_yellow = 0.0;
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                // don't count this cell
                if ((ox != 0 || oy != 0) && IsAlive(x + ox, y + oy)) {
                    number_of_alive_neighbors++;
                    total_blue += GetCell(x + ox, y + oy).blue;
                    total_yellow += GetCell(x + ox, y + oy).yellow;
                }
            }
        }
        if (number_of_alive_neighbors == 0) {
            return FloatPair(0.0, 0.0);
        }
        return FloatPair(total_blue / number_of_alive_neighbors, total_yellow / number_of_alive_neighbors);
    }

    // the maximum blue value and the maximum yellow value (not necessarily on same cell) of neighboring cells
    FloatPair MaxOfNeighbors(int x, int y) {
        float best_blue = 0.0;
        float best_yellow = 0.0;
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                // don't count this cell
                if (ox != 0 || oy != 0) {
                    FloatPair value = GetCell(x + ox, y + oy);
                    if (value.blue > best_blue) {
                        best_blue = value.blue;
                    }
                    if (value.yellow > best_yellow) {
                        best_yellow = value.yellow;
                    }
                }
            }
        }
        return FloatPair(best_blue, best_yellow);
    }

    // returns the minimum blue and minimum yellow values of neighboring cells. These may not be in the same cell
    FloatPair MinOfNeighbors(int x, int y) {
        float best_blue = 0.0;
        float best_yellow = 0.0;
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                // don't count this cell
                if (ox != 0 || oy != 0) {
                    FloatPair value = GetCell(x + ox, y + oy);
                    if (value.blue < best_blue) {
                        best_blue = value.blue;
                    }
                    if (value.yellow < best_yellow) {
                        best_yellow = value.yellow;
                    }
                }
            }
        }
        return FloatPair(best_blue, best_yellow);
    }

    

    // apply the update rule to each cell to determine its value in the next generation
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
    FloatPair UpdateFunction(int x, int y) {
        FloatPair result = LifelikeUpdateFunction(x, y);
        return Clamp(result);
    }

    // makes sure the blue/yellow values stay clamped between 0 and 1
    FloatPair Clamp(FloatPair pair) {
        if (pair.blue < 0.0)
            pair.blue = 0.0;
        if (pair.yellow < 0.0)
            pair.yellow = 0.0;
        if (pair.blue > 1.0)
            pair.blue = 1.0;
        if (pair.yellow > 1.0)
            pair.yellow = 1.0;
        return pair;
    }

    // the update function for Conway's Game of Life
    // cells must have int values, not FloatPair
    int LifeUpdateFunction(int x, int y) {
        int number_of_alive_neighbors = NumberOfAliveNeighbors(x, y);
        bool is_alive = IsAlive(x, y);
        if (is_alive) {
            // an alive cell survives if it has 2 or 3 neighbors
            return (number_of_alive_neighbors == 2 || number_of_alive_neighbors == 3) ? 1 : 0;
        } else {
            // a dead cell is born if it has 3 neighbors
            return number_of_alive_neighbors == 3 ? 1 : 0;
        }
    }

    // I completely just made up this update function, and it's so complicated I don't know how to describe it
    // basically, I wrote random code and fiddled with it until it led to some interesting structure
    // this update rule leads to mazelike regions of stripes of yellow cells and other regions with stripes of blue cells
    // with chaos in between the two regions. The stripes flash with period about 2, so I apply this rule twice every frame
    // to ease the eyes. Eventually the pattern settles into well-defined regions.
    FloatPair LifelikeUpdateFunction(int x, int y) {
        FloatPair value = GetCell(x, y);
        int number_of_alive_neighbors = NumberOfAliveNeighbors(x, y);
        FloatPair max_of_neighbors = MaxOfNeighbors(x, y);
        FloatPair min_of_neighbors = MinOfNeighbors(x, y);
        if (value.blue > value.yellow && value.blue > 0.1) {
            if (max_of_neighbors.blue - min_of_neighbors.blue < 0.2 || max_of_neighbors.yellow - min_of_neighbors.yellow < 0.2) {
                min_of_neighbors.blue -= 0.4;
                return min_of_neighbors;
            } else {
                if (number_of_alive_neighbors >= 3) {
                    return AverageOfAliveNeighbors(x, y);
                } else {
                    value.yellow -= 0.4;
                    return value;
                }
            }
        } else {
            if (number_of_alive_neighbors == 3) {
                return FloatPair(value.blue, max_of_neighbors.yellow);
            } else if (number_of_alive_neighbors == 0) {
                return FloatPair(0.0, 0.0);
            } else {
                FloatPair average_of_neighbors = AverageOfAliveNeighbors(x, y);
                if (average_of_neighbors.blue > 0.5) {
                    value.blue -= 0.3;
                } else {
                    value.blue += 0.3;
                }
                if (average_of_neighbors.yellow > 0.5) {
                    value.yellow -= 0.3;
                } else {
                    value.yellow += 0.3;
                }
                return value;
            }
        }
    }

    // sets the cells in a 4x4 square to random values
    void AddRandomCells() {
        emp::Random random = *new emp::Random(1);
        for (int x = 6; x <=9; x++) {
            for (int y = 4; y <= 7; y++) {
                cells[x][y] = FloatPair(random.GetDouble(0.0, 1.0), random.GetDouble(0.0, 1.0));
            }
        }
    }
};

CAAnimator animator;

int main() {
    //Have animator call DoFrame once to start
    animator.Step();
}