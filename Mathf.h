#ifndef MATHF
#define MATHF

class Mathf {
    public :
        inline static float lerp(float from, float to, float ratio){
            return (to - from) * ratio + from;
        }

        inline static float inverseLerp(float value, float from, float to){
            return (value - from) / (to - from);
        }

        inline static float remap(float value, float fromRangeStart, float fromRangeEnd, float toRangeStart, float toRangeEnd){
            return lerp(toRangeStart, toRangeEnd, inverseLerp(value, fromRangeStart, fromRangeEnd));
        }
};

#endif