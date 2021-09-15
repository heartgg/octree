#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#define TopLeftBack 0
#define TopRightBack 1
#define TopRightFront 2
#define TopLeftFront 3
#define BottomLeftBack 4
#define BottomRightBack 5
#define BottomRightFront 6
#define BottomLeftFront 7

struct Point {
    double x;
    double y;
    double z;
    int type = 0;
    Point() { type = 1; }
    Point(double a, double b, double c) {
        x = a, y = b, z = c;
    }
};

struct Boundary {
    double x;
    double y;
    double z;
    Boundary() {}
    Boundary(int point[]) {
        x = point[0], y = point[1], z = point[2];
    }
    Boundary(double a, double b, double c) {
        x = a, y = b, z = c;
    }
};

class Octree {

    Point point;
    Boundary topRightFront, bottomLeftBack;
    std::vector<Octree> nodes;
    
    public:
        Octree(Boundary bLb, Boundary tRf) {

            point = Point();
            point.type = 2;
            bottomLeftBack = bLb;
            topRightFront = tRf;

            nodes.assign(8, Octree());
        }

        Octree(Point coords) {
            point = coords;
        }

        Octree() {
            point = Point();
        }

        void insert (Point coords) {
            if (outOfBounds(coords))
            {
                std::cout << "Error: Coordinates out of bounds." << std::endl;
                return;
            }

            double midX = (topRightFront.x + bottomLeftBack.x) / 2;
            double midY = (topRightFront.y + bottomLeftBack.y) / 2;
            double midZ = (topRightFront.z + bottomLeftBack.z) / 2;

            int position = 1;

            if (coords.x > midX) position *= 3;
            if (coords.y > midY) position *= 7;
            if (coords.z > midZ) position *= 11;

            int location;
            Boundary tRf;
            Boundary bLb;

            switch (position)
            {
            case 1: // BottomLeftBack
                location = BottomLeftBack;
                tRf = Boundary(midX, midY, midZ);
                bLb = Boundary(bottomLeftBack.x, bottomLeftBack.y, bottomLeftBack.z);
                break;
            case 11: // TopLeftBack
                location = TopLeftBack;
                tRf = Boundary(midX, midY, topRightFront.z);
                bLb = Boundary(bottomLeftBack.x, bottomLeftBack.y, midZ); 
                break;
            case 7: // BottomRightBack
                location = BottomRightBack;
                tRf = Boundary(midX, topRightFront.y, midZ);
                bLb = Boundary(bottomLeftBack.x, midY, bottomLeftBack.z); 
                break;
            case 3: // BottomLeftFront
                location = BottomLeftFront;
                tRf = Boundary(topRightFront.x, midY, midZ);
                bLb = Boundary(midX, bottomLeftBack.y, bottomLeftBack.z); 
                break;
            case 33: // TopLeftFront
                location = TopLeftFront;
                tRf = Boundary(topRightFront.x, midY, topRightFront.z);
                bLb = Boundary(midX, bottomLeftBack.y, midZ);
                break;
            case 21: // BottomRightFront
                location = BottomRightFront;
                tRf = Boundary(topRightFront.x, topRightFront.y, midZ);
                bLb = Boundary(midX, midY, bottomLeftBack.z); 
                break;
            case 77: // TopRightBack
                location = TopRightBack;
                tRf = Boundary(midX, topRightFront.y, topRightFront.z);
                bLb = Boundary(bottomLeftBack.x, midY, midZ); 
                break;
            case 231: // TopRightFront
                location = TopRightFront;
                tRf = Boundary(topRightFront.x, topRightFront.y, topRightFront.z); 
                bLb = Boundary(midX, midY, midZ);
                break;
            }

            if (nodes[location].point.type == 1) 
                nodes[location] = Octree(coords);
            else if (nodes[location].point.type == 0) 
            {
                Point holder = nodes[location].point;
                nodes[location] = Octree(bLb,tRf);
                nodes[location].insert(holder);
                nodes[location].insert(coords);
            }
            else 
                nodes[location].insert(coords);

        }

        bool outOfBounds (Point coords) {
            return coords.x > topRightFront.x 
            || coords.y > topRightFront.y
            || coords.z > topRightFront.z
            || coords.x < bottomLeftBack.x
            || coords.y < bottomLeftBack.y
            || coords.z < bottomLeftBack.z;
        }

        void subsample (int depth, std::vector<Point>& subsampled) {
            for (size_t i = 0; i < nodes.size(); i++)
            {
                if (nodes[i].point.type == 0) 
                    subsampled.push_back(nodes[i].point);
                else if (nodes[i].point.type == 2)
                    if (depth > 0)
                        nodes[i].subsample(depth--, subsampled);
                    else if (nodes[i].point.x != 0) 
                        subsampled.push_back(nodes[i].point);
            }
        }
};

int main () {
    
    int depth = 16;
    std::vector<Point> points;
    std::vector<Point> subsampled;

    int maxPoint[3] = {0};
    int minPoint[3] = {0};

    std::fstream file;
    std::vector<double> point;
    file.open("input.csv");

    std::string line,coord,temp;
    
    while (file.good()) {
        point.clear();
        std::getline(file, line);
        std::stringstream ss(line);

        if (line == "x,y,z") continue;

        while (ss.good())
        {
            std::getline(ss, coord, ',');
            point.push_back(std::stod(coord));
        }
        
        for (size_t i = 0; i < point.size(); i++)
        {
            if (point[i] > maxPoint[i]) maxPoint[i] = std::ceil(point[i]);
            else if (point[i] < minPoint[i]) minPoint[i] = std::floor(point[i]);
        }

        Point coordinate(point[0], point[1], point[2]);
        points.push_back(coordinate);
    }

    file.close();

    Boundary minCorner(minPoint);
    Boundary maxCorner(maxPoint);

    Octree tree(minCorner, maxCorner);

    for (Point coordinate : points) {
        tree.insert(coordinate);
    }

    tree.subsample(depth, subsampled);

    file.open("output.csv", std::ofstream::out | std::ofstream::trunc);
    file << "x,y,z\n";

    for (size_t i = 0; i < subsampled.size(); i++)
    {
        file << std::fixed << std::setprecision(5) << subsampled[i].x << ',' << subsampled[i].y << ',' << subsampled[i].z << "\n";
    }

    file.close();
}