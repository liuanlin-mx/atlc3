#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <cstdint>
#include <stdlib.h>
#include <new>

template <typename T>
class matrix
{
public:
    matrix()
        : _data(NULL)
        , _rows(0)
        , _cols(0)
        , _border(0)
    {
        
    }
    
    ~matrix()
    {
        
    }
    
public:
    
    bool create(std::int32_t rows, std::int32_t cols, std::int32_t border = 0, const T& v = T())
    {
        _data = (T*)malloc(sizeof(T) * (rows + border * 2) * (cols + border * 2));
        if (_data == NULL)
        {
            return false;
        }
        
        for (std::int32_t i = 0; i < (rows + border * 2) * (cols + border * 2); i++)
        {
            new(_data + i) T(v);
        }
        
        _rows = rows;
        _cols = cols;
        _border = border;
        return true;
    }
    
    inline T& at(std::int32_t row, std::int32_t col)
    {
        return _data[row * _cols + col];
    }
    
    inline const T& at(std::int32_t row, std::int32_t col) const
    {
        return _data[row * _cols + col];
    }
    
    inline T* data()
    {
        return _data;
    }
    
    inline const T* data() const
    {
        return _data;
    }
    
    inline std::int32_t rows() const
    {
        return _rows;
    }
    
    inline std::int32_t cols() const
    {
        return _cols;
    }
private:
    T *_data;
    std::int32_t _rows;
    std::int32_t _cols;
    std::int32_t _border;
};
#endif