#ifndef MAP
#define MAP

#include "Vector2.h"
#include "Consts.h"
#include "Array.h"

struct Map {
    private:
        bool _map[MAP_SIZE][MAP_SIZE];

    public:
        Array<Vector2, MAP_SIZE * MAP_SIZE> getOffPositions() const {
            Array<Vector2, MAP_SIZE * MAP_SIZE> offPositions;

            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (!this->getAt({x, y})){
                        offPositions.push_back({x, y});
                    }
                }
            }

            return offPositions;
        }

        byte getOnAmount(){
            byte amount = 0;

            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (this->getAt({x, y}))
                        ++amount;
                }
            }

            return amount;
        }

        bool getAt(Vector2 position) const {
            return this->_map[position.x][position.y];
        }

        void setAt(Vector2 position, bool isOn) {
            this->_map[position.x][position.y] = isOn;
        }

        void clear(){
            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    this->setAt({x, y}, false);
                }
            }
        }

        inline bool operator==(const Map& rhs) { 
            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (this->getAt({x, y}) != rhs.getAt({x,y}))
                        return false;
                }
            }

            return true;
        }

        inline bool operator!=(const Map& rhs) { return !(*this == rhs); }
};
#endif