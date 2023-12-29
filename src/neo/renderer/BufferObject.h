#pragma once

#include "Allocator_VK.h"

enum bufferMapType_t
{
    BM_READ,
    BM_WRITE,
};

enum bufferUsageType_t
{
    BU_STATIC,
    BU_DYNAMIC,
};

void UnbindBufferObjects();
bool IsWriteCombined( void * base );
void CopyBuffer( uint8_t * dst, const uint8_t * src, int numBytes );

class idBufferObject
{
public:
    idBufferObject();
    void SetMapped() const { const_cast< int & >( m_size ) |= MAPPED_FLAG; }
    void SetUnmapped() const { const_cast< int & >( m_size ) &= ~MAPPED_FLAG; }
    int GetAllocedSize() const { return ( ( m_size & ~MAPPED_FLAG ) + 15 ) & ~15; }
    int GetSize() const { return ( m_size & ~MAPPED_FLAG ); }
    int	GetOffset() const { return ( m_offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }
	bufferUsageType_t GetUsage() const {return m_usage;}
protected:
    

    int m_size;
    int m_offsetInOtherBuffer;
    bufferUsageType_t m_usage;

    VkBuffer m_apiObject;
    vulkanAllocation_t m_allocation;

    static const int MAPPED_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
    static const int OWNS_BUFFER_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
};

class idIndexBuffer : public idBufferObject
{
public:
    idIndexBuffer();

    bool AllocBufferObject(const void* data, int allocSize, bufferUsageType_t usage);
    void Update( const void * data, int size, int offset = 0 ) const;
    void* MapBuffer(bufferMapType_t mapType);
    void UnmapBuffer() {SetUnmapped();}
private:
    idIndexBuffer& operator=(const idIndexBuffer&) = delete;
    idIndexBuffer(const idIndexBuffer&) = delete;
};

class idVertexBuffer : public idBufferObject
{
public:
    idVertexBuffer();

    bool AllocBufferObject(const void* data, int allocSize, bufferUsageType_t usage);
    void Update(const void* data, int size, int offset = 0) const;
    void* MapBuffer(bufferMapType_t mapType);
    void UnmapBuffer() {SetUnmapped();}
private:
    idVertexBuffer& operator=(const idVertexBuffer&) = delete;
    idVertexBuffer(const idVertexBuffer&) = delete;
};

class idUniformBuffer : public idBufferObject
{
public:
    idUniformBuffer();

    bool AllocBufferObject(const void* data, int allocSize, bufferUsageType_t usage);
    void Update(const void* data, int size, int offset = 0) const;
    void* MapBuffer(bufferMapType_t mapType);
    void UnmapBuffer() {SetUnmapped();}
private:
    idUniformBuffer& operator=(const idUniformBuffer&) = delete;
    idUniformBuffer(const idUniformBuffer&) = delete;
};