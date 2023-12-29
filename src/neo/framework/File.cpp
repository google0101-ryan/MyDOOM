#include "File.h"
#include "Common.h"
#include "Filesystem.h"

#include <string.h>

int idFile::Seek(long offset, fsOrigin_t origin)
{
    return -1;
}

int idFile::Read(void *buffer, int len)
{
    common->Error("idFile::Read: cannot read from idFile");
    return 0;
}

int idFile::ReadInt(int &value)
{
    int result = Read(&value, sizeof( value ));
    return result;
}

int idFile::ReadString(std::string &string)
{
    int len;
    int result = 0;

    ReadInt(len);
    if (len >= 0)
    {
        char* buf = new char[len];
        result = Read(buf, len);
        string = buf;
    }

    return result;
}

idFile_Permanent::idFile_Permanent()
{
    name = "invalid";
    o = NULL;
    mode = 0;
    fileSize = 0;
    handleSync = false;
}

int idFile_Permanent::Read(void *buffer, int size)
{
    int block, remaining;
    int read;
    uint8_t* buf;
    int tries;

    if (!(mode & (1 << FS_READ)))
    {
        common->Error("idFile_Permanent::Read: %s not opened in read mode", name.c_str());
        return 0;
    }

    if (!o)
    {
        common->Warning("Tried to read from NULL file\n");
        return 0;
    }

    buf = (uint8_t*)buffer;

    remaining = size;
    tries = 0;
    while (remaining)
    {
        block = remaining;

        read = fread(buf, 1, block, o);

        if (read == 0)
        {
            if (!tries)
            {
                tries = 1;
            }
            else
            {
                return size - remaining;
            }
        }

        if (read == -1)
        {
            common->Error("idFile_Permanent::Read: -1 bytes read from %s", name.c_str());
        }

        remaining -= read;
        buf += read;
    }

    return size;
}

int idFile_Permanent::Seek(long offset, fsOrigin_t origin)
{
    int _origin;

    switch (origin)
    {
    case FS_SEEK_CUR:
        _origin = SEEK_CUR;
        break;
    case FS_SEEK_END:
        _origin = SEEK_END;
        break;
    case FS_SEEK_SET:
        _origin = SEEK_SET;
        break;
    default:
        _origin = SEEK_CUR;
        common->Error("idFile_Permanent::Seek: Invalid seek origin\n");
        break;
    }

    return fseek(o, offset, _origin);
}

idFile_Memory::idFile_Memory()
{
    name = "*unknown*";
    maxSize = 0;
    fileSize = 0;
    allocated = 0;
    granularity = 16384;

    mode = (1 << FS_WRITE);
    filePtr = NULL;
    curPtr = NULL;
}

idFile_Memory::idFile_Memory(const char *name, const char *data, int length)
{
    this->name = name;
    maxSize = 0;
    fileSize = length;
    allocated = length;
    granularity = 16384;

    mode = (1 << FS_READ);
    filePtr = const_cast<char*>(data);
    curPtr = const_cast<char*>(data);
}

int idFile_Memory::Read(void *buffer, int size)
{
    if (!(mode & (1 << FS_READ)))
    {
        common->Error("idFile_Memory::Read: %s not opened in read mode", name.c_str());
        return 0;
    }

    if (curPtr + size > filePtr + fileSize)
    {
        size = filePtr + fileSize - curPtr;
    }

    memcpy(buffer, curPtr, size);

    curPtr += size;
    return size;
}

int idFile_Memory::Seek(long offset, fsOrigin_t origin)
{
    switch( origin )
	{
		case FS_SEEK_CUR:
		{
			curPtr += offset;
			break;
		}
		case FS_SEEK_END:
		{
			curPtr = filePtr + fileSize - offset;
			break;
		}
		case FS_SEEK_SET:
		{
			curPtr = filePtr + offset;
			break;
		}
		default:
		{
			common->Error( "idFile_Memory::Seek: bad origin for %s\n", name.c_str() );
			return -1;
		}
	}
	if( curPtr < filePtr )
	{
		curPtr = filePtr;
		return -1;
	}
	if( curPtr > filePtr + fileSize )
	{
		curPtr = filePtr + fileSize;
		return -1;
	}
	return 0;
}
