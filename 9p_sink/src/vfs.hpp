#pragma once

#include <endian.h>

#include <atomic>
#include <shared_mutex>
#include <map>

enum FileMode {
    DMDIR = 1 << 31,
    DMAPPEND = 1 << 30,
    DMEXCL = 1 << 29,

    DMAUTH = 1 << 27,
    DMTMP = 1 << 26,
    U_R = 1 << 8,
    U_W = 1 << 7,
    U_X = 1 << 6,
    G_R = 1 << 5,
    G_W = 1 << 4,
    G_X = 1 << 3,
    O_R = 1 << 2,
    O_W = 1 << 1,
    O_X = 1,
};

static size_t PutString(char* dst, std::string str) {
    dst += Endian::LittleEndian::put(dst, (uint16_t)str.length());
    memcpy(dst, str.c_str(), str.length());
    return 2 + str.length();
}

#include <iostream>

class LESerializable {
public:
    virtual ~LESerializable() {}
    virtual size_t put(char* dst) = 0;
    virtual size_t size() = 0;
};

class SerString : public LESerializable {
public:
    SerString() {
    }
    SerString(std::string m) {
        str = m;
    }
    ~SerString() {
    }
    size_t size() {
        return 2 + str.length();
    }
    size_t put(char* dst) {
        dst += Endian::LittleEndian::put(dst, (uint16_t)str.length());
        memcpy(dst, str.c_str(), str.length());
        return size();
    }
private:
    std::string str;
};

class SerRaw : public LESerializable {
public:
    SerRaw() {
    }
    SerRaw(std::string&& m) {
        str = m;
    }
    size_t size() {
        return str.length();
    }
    size_t put(char* dst) {
        memcpy(dst, str.c_str(), str.length());
        return size();
    }
private:
    std::string str;
};

#include <iostream>

template <class T>
class SerAny : public LESerializable {
public:
    SerAny(T ma) {
        m = ma;
    }
    size_t size() {
        return sizeof(T);
    }
    size_t put(char* dst) {
        dst += Endian::LittleEndian::put(dst, m);
        return size();
    }
    T m;
};

#include <iostream>

class Qid : public LESerializable {
public:
    uint8_t mode;
    uint32_t version;
    uint64_t path;
    Qid() {}
    Qid(uint8_t m, uint32_t v, uint64_t p) {
        mode = m;
        version = v;
        path = p;
    }
    size_t size() {
        return 1 + 4 + 8;
    }
    size_t put(char* dst) {
        dst += Endian::LittleEndian::put(dst, mode);
        dst += Endian::LittleEndian::put(dst, version);
        dst += Endian::LittleEndian::put(dst, path);
        return size();
    }
};

class Stat : public LESerializable {
public:
    Stat() {}
    uint16_t sz;
    uint16_t type;
    uint32_t dev;
    Qid qid;
    uint32_t mode;
    uint32_t atime;
    uint32_t mtime;
    uint64_t length;
    std::string name;
    std::string uid;
    std::string gid;
    std::string muid;

    size_t size() {
        size_t len =  2 + 2 + 4 + qid.size() + 4 + 4 + 4 + 8
                        + 2 + name.length()
                        + 2 + uid.length()
                        + 2 + gid.length()
                        + 2 + muid.length();
        return len;
    }
    size_t put(char* dst) {
        sz = (uint16_t)size() - 2;
        dst += Endian::LittleEndian::put(dst, sz);
        dst += Endian::LittleEndian::put(dst, type);
        dst += Endian::LittleEndian::put(dst, dev);
        dst += qid.put(dst);
        dst += Endian::LittleEndian::put(dst, mode);
        dst += Endian::LittleEndian::put(dst, atime);
        dst += Endian::LittleEndian::put(dst, mtime);
        dst += Endian::LittleEndian::put(dst, length);
        dst += PutString(dst, name);
        dst += PutString(dst, uid);
        dst += PutString(dst, gid);
        dst += PutString(dst, muid);
        return sz;
    }
};

class VFS;
class File;

typedef uint32_t (*ReadHandler)(std::shared_ptr<File> file, std::string& buf, uint64_t offset, uint32_t count);
typedef uint32_t (*WriteHandler)(std::shared_ptr<File> file, uint64_t offset, uint32_t count, char* data);

class File {
public:
    static uint64_t pathcur;
    ~File() {
        std::cout << "file is deleted :vibe:" << std::endl;
    }
    File() {}
    File(uint32_t m, VFS* v, uint64_t p) {
        vfs = v;
        parent = p;
        path = pathcur++;
        qid = Qid((uint8_t)(m >> 24), 0, path);
        mode = m;
        
        atime = 5;
        mtime = 5;
        length = 1024;
        filename = "/";
        owner = "aws";
        group = "aws";
        modifier = "aws";
        iocount = 0;
        binned = false;
        writing = false;
        structural_readers = 0;
        self_readers = 0;
    }
    uint64_t path;
    Qid qid;
    std::atomic<uint32_t> mode;
    std::atomic<uint32_t> atime;
    std::atomic<uint32_t> mtime;
    std::atomic<uint64_t> length;
    std::string filename;
    std::string owner;
    std::string group;
    std::string modifier;

    VFS* vfs;
    uint64_t parent;
    std::map<std::string, uint64_t> children;
    std::string content;
    std::atomic<unsigned> iocount;
    std::shared_mutex hasio;
    std::shared_mutex structural; // tells whether stat-ed data is being accessed.
    std::atomic<bool> binned;
    std::atomic<bool> writing;
    std::atomic<unsigned> structural_readers;
    std::mutex self;
    std::atomic<unsigned> self_readers;

    ReadHandler rh;
    WriteHandler wh;

    Stat stat() { // what if stat-ed during deletion ??
        Stat s;
        structural.lock_shared();
        s.type = 0;
        s.dev = 0;
        s.qid = qid;
        s.mode = mode;
        s.atime = atime;
        s.mtime = mtime;
        s.length = length;
        s.name = filename;
        s.uid = owner;
        s.gid = group;
        s.muid = modifier;
        structural.unlock_shared();
        return s;
    }
};

uint64_t File::pathcur = 1;

class VFS {
public:
    std::shared_ptr<File> getFile(uint64_t path) {
        access.lock_shared();
        if (files.count(path) <= 0) {
            access.unlock_shared();
            return nullptr;
        }
        auto f = files[path];
        access.unlock_shared();
        return f;
    }
    void deleteFile(uint64_t path) {
        auto f = getFile(path);
        if (!f) { return; }
        auto p = getFile(f->parent);
        if(p) {
            p->structural.lock();
            p->children.erase(f->filename);
            p->structural.unlock();
        }
        access.lock();
        files.erase(path);
        access.unlock();
    }
    std::shared_mutex access;
    std::map<uint64_t, std::shared_ptr<File>> files;
    std::map<std::string, uint64_t> roots;
};

struct VFSIO {
    VFSIO() {}
    VFSIO(std::shared_ptr<File> f) {
        file = f;
        vfs = f->vfs;
        offset = 0;
        readdir = f->children.begin();
        enddir = f->children.end();
        file->iocount++;
        std::cout << "create io[" << file->path << "] = " << file->filename << " with iocount " << file->iocount << std::endl;
    }
    ~VFSIO() {
        std::cout << "delete io[" << file->path << "] = " << file->filename << " with iocount " << file->iocount << std::endl;
        file->iocount--;
    }
    VFS* vfs;
    std::shared_ptr<File> file;
    uint64_t offset;
    std::map<std::string, uint64_t>::iterator readdir;
    std::map<std::string, uint64_t>::iterator enddir;
};