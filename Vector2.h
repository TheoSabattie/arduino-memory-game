#ifndef VECTOR2
#define VECTOR2

struct Vector2 {
    public:
        char x;
        char y;

        const bool isZero() const {
            return x == 0 && y == 0; 
        }

        inline bool operator==(const Vector2& rhs) { return x == rhs.x && y == rhs.y; }
        inline bool operator!=(const Vector2& rhs) { return x != rhs.x || y != rhs.y; }
};

#endif