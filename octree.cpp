#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>

enum NodeType {
    EMPTY,
    ONODE, // leaf
    INODE // branch node
};

enum RelativeLocation {
    GREATER_X = 1,
    GREATER_Y = 2,
    GREATER_Z = 4
};

struct Point {
    double x, y, z;

    Point () {
        x = 0, y = 0, z = 0;
    }
    Point (double pX, double pY, double pZ) {
        x = pX, y = pY, z = pZ;
    }
};

class Octree {
    // boundaries that define the octree prism, opposite corners
    private:
        NodeType type = EMPTY; // Tree type
        Point bLbBoundary, tRfBoundary, midpoint;
        Point* firstPoint = nullptr; // Representative point for subsampling
        Octree* nodes = nullptr;

    public:
        Octree() {
            type = EMPTY;
        }

        Octree(Point* coord) {
            firstPoint = coord;
            type = ONODE;
        }

        Octree(Point bLb, Point tRf) {
            bLbBoundary = bLb;
            tRfBoundary = tRf;
            midpoint = calcMidpoint(bLbBoundary, tRfBoundary);
            nodes = new Octree[8];
            type = INODE;
        }

        // Inserts a point into the octree
        // by figuring out which octant to go into
        // and then recursively running the function
        // until an empty spot is found
        void insert(Point *coord) {
            if (outOfBounds(coord)) return;

            Point tRf, bLb;
            int position = 0;
            if (coord->x > midpoint.x) position |= GREATER_X;
            if (coord->y > midpoint.y) position |= GREATER_Y;
            if (coord->z > midpoint.z) position |= GREATER_Z;

            switch (position) {
            case 0: // 000 BottomLeftBack
                tRf = midpoint;
                bLb = bLbBoundary;
                break;
            case 1: // 001 BottomLeftFront
                tRf = Point(tRfBoundary.x, midpoint.y, midpoint.z);
                bLb = Point(midpoint.x, bLbBoundary.y, bLbBoundary.z); 
                break;
            case 2: // 010 BottomRightBack
                tRf = Point(midpoint.x, tRfBoundary.y, midpoint.z);
                bLb = Point(bLbBoundary.x, midpoint.y, bLbBoundary.z); 
                break;
            case 3: // 011 BottomRightFront
                tRf = Point(tRfBoundary.x, tRfBoundary.y, midpoint.z);
                bLb = Point(midpoint.x, midpoint.y, bLbBoundary.z); 
                break;
            case 4: // 100 TopLeftBack
                tRf = Point(midpoint.x, midpoint.y, tRfBoundary.z);
                bLb = Point(bLbBoundary.x, bLbBoundary.y, midpoint.z);
                break;
            case 5: // 101 TopLeftFront
                tRf = Point(tRfBoundary.x, midpoint.y, tRfBoundary.z);
                bLb = Point(midpoint.x, bLbBoundary.y, midpoint.z);
                break;
            case 6: // 110 TopRightBack
                tRf = Point(midpoint.x, tRfBoundary.y, tRfBoundary.z);
                bLb = Point(bLbBoundary.x, midpoint.y, midpoint.z); 
                break;
            case 7: // 111 TopRightFront
                tRf = tRfBoundary; 
                bLb = midpoint;
                break;
            }

            if (nodes[position].type == EMPTY) {
                nodes[position] = Octree(coord);
            // This converts an ONODE into an INODE
            } else if (nodes[position].type == ONODE) {
                Point* placeholder = nodes[position].firstPoint;
                nodes[position] = Octree(bLb, tRf);
                nodes[position].firstPoint = placeholder;
                nodes[position].insert(placeholder);
                nodes[position].insert(coord);
            } else {
                nodes[position].insert(coord);
            }
        }

        // Returns a Point thats a midpoint between two Points
        Point calcMidpoint(Point bLb, Point tRf) {
            double midX = (tRf.x + bLb.x) / 2;
            double midY = (tRf.y + bLb.y) / 2;
            double midZ = (tRf.z + bLb.z) / 2;
            return Point(midX, midY, midZ);
        }

        // Checks if a Point is outside the Octrees boundaries
        bool outOfBounds (Point *coords) {
            return coords->x > tRfBoundary.x 
            || coords->y > tRfBoundary.y
            || coords->z > tRfBoundary.z
            || coords->x < bLbBoundary.x
            || coords->y < bLbBoundary.y
            || coords->z < bLbBoundary.z;
        }

        // For every node in the octree,
        // if the node is a leaf, read the coordinate into the vector
        // if the node is a branch, subsample recursively or read the
        // representative point (firstPoint) if the depth has been reached
        void subsample (int depth, std::vector<Point*>& subsampled) {
            int passedDepth = --depth;
            for (int i = 0; i < 8; i++) {
                if (nodes[i].type == ONODE) {
                    subsampled.push_back(nodes[i].firstPoint);
                } else if (nodes[i].type == INODE) {
                    if (passedDepth > 0) {
                        nodes[i].subsample(passedDepth, subsampled);
                    } else {
                        subsampled.push_back(nodes[i].firstPoint);
                    }
                }
            }
        }
};

int main () {
    const int DEPTH = 10; // for recursive subsampling
    std::fstream file;
    file.open("input.csv");

    std::string line,coord,temp;
    std::vector<Point> array;
    
    // Read all of the CSV coordinate points into a Point vector
    while (file.good()) {
        std::getline(file, line);
        std::stringstream ss(line);

        if (line == "x,y,z" || line == "") continue;

        double placeholder[3];
        for (int i = 0; i < 3; i++)
        {
            std::getline(ss, coord, ',');
            placeholder[i] = std::stod(coord);
        }
        
        array.push_back(Point(placeholder[0], placeholder[1], placeholder[2]));
    }
    file.close();
    
    // Create the root octree
    Octree root = Octree(Point(-10, -10, -10), Point(10, 10, 10));
    
    // Insert each Point from the root down
    for (int i = 0; i < array.size(); i++) {
        root.insert(&array.at(i));
    }

    // Downsample the Points into the Point* vector
    std::vector<Point*> subsampled;
    root.subsample(DEPTH, subsampled);
    
    file.open("output.csv", std::ofstream::out | std::ofstream::trunc);
    file << "x,y,z\n";

    // Write every subsampled point into the output file
    for (int i = 0; i < subsampled.size(); i++) {
        file << std::fixed << std::setprecision(6) << subsampled.at(i)->x << ',' << subsampled.at(i)->y << ',' << subsampled.at(i)->z << "\n";
    }

    file.close();
}