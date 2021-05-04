#pragma once

#include <buffer.hpp>
#include <tcp_sockets.hpp>
#include <vfs.hpp>
#include <data.hpp>

enum NinePMsgType {
    Tversion = 100,
    Rversion,
    Tauth = 102,
    Rauth,
    Tattach = 104,
    Rattach,
    Terror = 106,
    Rerror,
    Tflush = 108,
    Rflush,
    Twalk = 110,
    Rwalk,
    Topen = 112,
    Ropen,
    Tcreate = 114,
    Rcreate,
    Tread = 116,
    Rread,
    Twrite = 118,
    Rwrite,
    Tclunk = 120,
    Rclunk,
    Tremove = 122,
    Rremove,
    Tstat = 124,
    Rstat,
    Twstat = 126,
    Rwstat,
    Tmax,
};

struct NinePClient_t {
    NinePClient_t(VFS* vfs) : vfs(vfs) {
        msize = 256;
    }
    void putfid(uint32_t fid, std::shared_ptr<File> f) {
        fids[fid] = f->path;
    }
    std::shared_ptr<File> getfid(uint32_t fid) {
        if (fids.count(fid) <= 0) { return nullptr; }

        uint64_t path = fids[fid];

        auto f = vfs->getFile(path);
        if (!f) {
            clunkfid(fid);
            return nullptr;
        }

        return f;
    }
    void clunkfid(uint32_t fid) {
        if (io.count(fid) > 0) {
            io.erase(fid);
        }
        if (fids.count(fid) > 0) {
            fids.erase(fid);
        }
    }
    void initio(uint32_t fid) {
        auto f = getfid(fid);
        if (!f) { return; }
        io[fid] = std::make_shared<VFSIO>(f);
    }
    std::shared_ptr<VFSIO> getio(uint32_t fid) {
        // getfid(fid); // -> TODO: decide whether to invalidate IO when file deleted 
        if (io.count(fid) <= 0) { return nullptr; }
        return io[fid];
    }
    size_t msize;
    std::map<uint32_t, uint64_t> fids;
    std::map<uint32_t, std::shared_ptr<VFSIO>> io;
    VFS* vfs;
};

class NinePResponse : public LESerializable {
public:
    NinePResponse(uint8_t t, uint16_t ta) {
        type = t + 1;
        tag = ta;
    }
    ~NinePResponse() {
        components.clear();
    }
    size_t size() {
        uint32_t len = 4 + 1 + 2;
        for (auto& comp : components) {
            len += comp->size();
        }
        return len;
    }
    size_t put(char* dst) {
        uint32_t sz = size();
        dst += Endian::LittleEndian::put(dst, sz);
        dst[0] = type;
        dst++;
        dst += Endian::LittleEndian::put(dst, tag);
        for (auto& comp : components) {
            dst += comp->put(dst);
        }
        return sz;
    }
    void error(std::string msg) {
        type = Rerror;
        components.clear();
        components.push_back(std::make_unique<SerString>(SerString(msg)));
    }
    template <class T>
    void add(T&& s) {
        components.push_back(std::make_unique<T>(T(s)));
    }
    uint8_t type;
    uint16_t tag;
    std::vector<std::unique_ptr<LESerializable>> components;
};

void dumpreq(Buffer& r) {
    printf("==> data begin");
    for (int i = 0; i < r.getLength(); i++) {
        printf("%u\n", ((char*)r.data())[i]);
    }
    printf("==> data end");
}

uint32_t defRead(std::shared_ptr<File> file, std::string& buf, uint64_t offset, uint32_t count) {
    if (offset >= file->content.length()) {
        return 0;
    }
    std::cout << (unsigned)count << std::endl;
    count = (count > file->content.length() - offset ? file->content.length() - offset : count);
    buf = std::string(file->content.c_str() + offset, count);
    return count;
}

uint32_t stream(std::shared_ptr<File> file, std::string& buf, uint64_t offset, uint32_t count) {
    buf = "rsttsartars";
    return buf.length();
}

static std::chrono::time_point<std::chrono::system_clock> start;

uint32_t uptimeRead(std::shared_ptr<File> file, std::string& buf, uint64_t offset, uint32_t count) {
    if (offset != 0) {
        return 0;
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    buf = std::to_string(elapsed_seconds.count()) + "\n";
    return buf.length();
}

class NinePServer : public TcpListener {
public:
    NinePServer(uint16_t port) : TcpListener(port) {
        start = std::chrono::system_clock::now();
        auto root = std::make_shared<File>(DMDIR | (U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, 0);
        auto f = std::make_shared<File>((U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, root->path);
        auto uptime = std::make_shared<File>((U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, root->path);
        auto fo = std::make_shared<File>(DMDIR | (U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, root->path);
        auto kernel = std::make_shared<File>((U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, root->path);
        auto initramfs = std::make_shared<File>((U_R|U_X) | (G_R|G_X) | (O_R|O_X), &vfs, root->path);
        f->filename = "hello.wav";
        f->rh = &defRead;
        f->content = std::string((char*)HEYYEYAAEYAAAEYAEYAA_ZZ5LpwO_An4_wav, HEYYEYAAEYAAAEYAEYAA_ZZ5LpwO_An4_wav_len);
        f->length = f->content.length();
        kernel->filename = "vmlinuz-linux";
        kernel->rh = &defRead;
        kernel->content = std::string((char*)_boot_vmlinuz_linux, _boot_vmlinuz_linux_len);
        kernel->length = kernel->content.length();
        initramfs->filename = "initramfs-linux.img";
        initramfs->rh = &defRead;
        initramfs->content = std::string((char*)_boot_initramfs_linux_img, _boot_initramfs_linux_img_len);
        initramfs->length = initramfs->content.length();
        uptime->filename = "uptime";
        uptime->rh = &uptimeRead;
        uptime->length = 1024;
        fo->filename = "fo";
        root->children[f->filename] = f->path;
        root->children[fo->filename] = fo->path;
        root->children[uptime->filename] = uptime->path;
        root->children[kernel->filename] = kernel->path;
        root->children[initramfs->filename] = initramfs->path;
        vfs.roots[""] = root->path;
        vfs.files[f->path] = f;
        vfs.files[root->path] = root;
        vfs.files[fo->path] = fo;
        vfs.files[uptime->path] = uptime;
        vfs.files[kernel->path] = kernel;
        vfs.files[initramfs->path] = initramfs;
    }

    void hVersion(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t msize = T.LEGet<uint32_t>();
        ncl.msize = msize;
        std::string version = T.get9PString();
        res.add(SerAny<uint32_t>(msize));
        if (version != "9P2000") {
            res.add(SerString(std::string("unknown")));
            return;
        }
        res.add(SerString(std::string("9P2000")));
    }

    void hAttach(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        uint32_t afid = T.LEGet<uint32_t>();

        std::string uname, aname;
        
        uname = T.get9PString();
        aname = T.get9PString();

        bool authWish = afid != ~((uint32_t)0);
        if (authWish) {
            res.error("authentication not implemented");
            return;
        }

        if (vfs.roots.count(aname) == 0) {
            res.error("no such fs");
            return;
        }

        auto f = vfs.getFile(vfs.roots[aname]);
    
        ncl.putfid(fid, f);

        res.add(Qid(f->qid));
    }


    // todo -> rewrite this recursively with structural locks
    void hWalk(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        uint32_t nwfid = T.LEGet<uint32_t>();
        uint16_t nwname = T.LEGet<uint16_t>();

        auto f = ncl.getfid(fid);

        if (nwname == 0) {
            std::cout << "empty, nwfid = " << (unsigned)nwfid << std::endl;
            ncl.putfid(nwfid, f);
            res.add(SerAny<uint16_t>(0));
            // res.add(Qid(f->qid));
            return;
        }

        std::vector<std::string> wname(nwname);
        std::cout << (unsigned)nwname << std::endl;
        for (size_t i = 0; i < nwname; i++) {
            wname[i] = T.get9PString();
            std::cout << wname[i] << std::endl;
        }

        bool succ = true;

        std::vector<Qid> nwqids;

        auto visit = f;

        for (size_t i = 0; i < nwname; i++) {
            if (wname[i] == ".") {
                // nwqids.push_back(visit->qid);
            } else if (wname[i] == "..") {
                if (visit->parent == 0) { // is root
                    nwqids.push_back(visit->qid);
                } else {
                    visit = vfs.files[visit->parent];
                    nwqids.push_back(visit->qid);
                }
            } else if (visit->children.count(wname[i]) > 0) {
                std::cout << "hello found" << std::endl;
                visit = vfs.files[visit->children[wname[i]]];
                nwqids.push_back(visit->qid);
            } else if (i > 0) {
                succ = false;
                break;
            } else {
                std::cout << "bruh" << std::endl;
                res.error("no such path");
                return;
            }
        }
        
        if (succ) {
            ncl.putfid(nwfid, visit);
        }

        std::cout << (unsigned)nwqids.size() << std::endl;
        res.add(SerAny<uint16_t>(nwqids.size()));
        for(auto& qid : nwqids) {
            res.add(Qid(qid));
        }
    }

    void hStat(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        auto f = vfs.getFile(ncl.fids[fid]);
        if (!f) { return res.error("no such file"); }
        Stat st = f->stat();
        res.add(SerAny<uint16_t>(st.size()));
        res.add(Stat(st));
    }

    void hClunk(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        ncl.clunkfid(fid);
    }

    void hRemove(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        auto f = ncl.getfid(fid);
        if (!f) { return res.error("no such file"); }
        auto p = vfs.getFile(f->parent);
        /*if (f->iocount > 0 || p->iocount > 0) {
            return res.error("file busy");
        }*/
        // f->writing = true;
        std::cout << p->filename << " structural lock" << std::endl;
        std::cout << p->filename << " structural unlock" << std::endl;
        vfs.deleteFile(f->path);
        ncl.clunkfid(fid);
    }

    void hOpen(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();

        auto f = ncl.getfid(fid);
        ncl.initio(fid);
        // for now, we prepare nothing, we just send OK

        res.add(Qid(f->qid));
        res.add(SerAny<uint32_t>(0)); // check what this is for
    }

    void hRead(NinePClient_t& ncl, Buffer& T, NinePResponse& res) {
        uint32_t fid = T.LEGet<uint32_t>();
        uint64_t offset = T.LEGet<uint64_t>();
        uint32_t count = T.LEGet<uint32_t>();

        auto io = ncl.getio(fid);
        if (!io) { 
            return res.error("no such file");
        }

        if (io->file->writing) {
            return res.error("file busy");
        }

        if (io->file->mode & DMDIR) {
            // if (offset != 0) { return res.error("illegal offset"); } // check other condidion
            io->file->structural.lock_shared();
retry:
            if (offset != io->offset || io->readdir == io->enddir) {
                res.add(SerAny<uint32_t>(0));
                io->file->structural.unlock_shared();
                return;
            }
            auto ch = vfs.getFile(io->readdir->second);
            if (!ch) { // should not happen
                io->readdir++;
                goto retry;
            }
            Stat chstat = ch->stat();
            uint32_t count = chstat.size();
            io->readdir++;
            io->offset += count;
            res.add(SerAny<uint32_t>(count));
            res.add(Stat(chstat));
            io->file->structural.unlock_shared();
            return;
        }
        // is file
        uint32_t normalcount = (count + 11 > ncl.msize ? ncl.msize : count);
        io->file->structural.lock_shared();
        std::string cont;
        uint32_t realcount = io->file->rh(io->file, cont, offset, normalcount);
        io->file->structural.unlock_shared();
        res.add(SerAny<uint32_t>(realcount));
        if (realcount != 0) { res.add(SerRaw(std::move(cont))); }
    }

    void client(TcpConn conn) {
        NinePClient_t ncl(&vfs);
        std::cout << "client?" << std::endl;
        while (true) {
            Buffer T(4); // allocate 4 bytes for size
            if (conn.read(T) <= 0) { break; } // read 4 bytes from incoming

            uint32_t size = T.LEGet<uint32_t>();
            if (size > ncl.msize) { break; } // too big, error or malicious, discard connection
            T.setCapacity(size); // resize to actual message size

            if (conn.read(T) <= 0) { break; } // read remaining of message

            uint8_t type = T[0];
            T += 1;

            uint16_t tag = T.LEGet<uint16_t>();

            NinePResponse res(type, tag);
            std::cout << (unsigned)type << std::endl;
            switch (type) {
            case Tversion:
                hVersion(ncl, T, res);
                break;
            case Tattach:
                hAttach(ncl, T, res);
                break;
            case Twalk:
                hWalk(ncl, T, res);
                break;
            case Tstat:
                hStat(ncl, T, res);
                break;
            case Tclunk:
                hClunk(ncl, T, res);
                break;
            case Topen:
                hOpen(ncl, T, res);
                break;
            case Tread:
                hRead(ncl, T, res);
                break;
            case Tremove:
                hRemove(ncl, T, res);
                break;
            default:
                std::cout << (unsigned)type << std::endl;
                res.error("unimplemented");
                break;
            }

            Buffer R(res.size());
            res.put((char*)R.data());
            // dumpreq(R);
            conn.send(R);
        }
        conn.close();
    }

    void run() {
        while (true) {
            TcpConn conn = TcpListener::accept();
            std::thread(&NinePServer::client, this, conn).detach();
        }
    }

private:
    VFS vfs;
};