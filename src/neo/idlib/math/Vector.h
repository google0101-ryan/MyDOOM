#pragma once

class idVec4
{
public:
    float x, y, z, w;

    idVec4() {}

    void Set(const float x, const float y, const float z, const float w);

    float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
};

inline void idVec4::Set(const float x, const float y, const float z, const float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

inline float idVec4::operator[](const int index) const
{
    return (&x)[index];
}

inline float &idVec4::operator[](const int index)
{
    return (&x)[index];
}

class idVec3
{
public:
    float x, y, z;

    idVec3() {}
	idVec3(const float x, const float y, const float z)
	: x(x), y(y), z(z) {}

    void Set(const float x, const float y, const float z);

    float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
  
	idVec3 operator*(const float a) const;
	idVec3 operator+(const idVec3& a) const;
};

inline void idVec3::Set(const float x, const float y, const float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

inline float idVec3::operator[](const int index) const
{
    return (&x)[index];
}

inline float &idVec3::operator[](const int index)
{
    return (&x)[index];
}

inline idVec3 idVec3::operator*(const float a) const
{
	return idVec3(x * a, y * a, z * a);
}

inline idVec3 idVec3::operator+(const idVec3 &a) const
{
	return idVec3(x + a.x, y + a.y, z + a.z);
}
