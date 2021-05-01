#include <imgui.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <signal_path/sink.h>
#include <endian.hpp>
#include <vfs.hpp>

#include <iostream>
#include <tcp_sockets.hpp>
#include <9p.hpp>

SDRPP_MOD_INFO {
    /* Name:            */ "9p_sink",
    /* Description:     */ "Stream audio and control sdrpp via the 9p2000 protocol",
    /* Author:          */ "aosync",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ -1
};

class NinePSink : SinkManager::Sink {
public:
    NinePSink(SinkManager::Stream* stream, std::string streamName) {

    }
    ~NinePSink() {

    }
    void start() {

    }
    void stop() {

    }
    void menuHandler() {
        float menuWidth = ImGui::GetContentRegionAvailWidth();

        char buf[6] = { '5', '6', '4', 0 };
        ImGui::SetNextItemWidth(menuWidth);
        if(ImGui::InputText("Port", buf, 6)) {

        }
    }
private:
    std::string txtDevList;
};

class NineP : public ModuleManager::Instance {
public:
    NineP(std::string name) {
        this->name = name;
        provider.create = create_sink;
        provider.ctx = this;
        sigpath::sinkManager.registerSinkProvider("9P", provider);
    }

    ~NineP() {
        
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

private:
    static void menuHandler(void* ctx) {
        NineP* _this = (NineP*)ctx;
        ImGui::Text("Hello SDR++, my name is %s", _this->name.c_str());
    }

    static SinkManager::Sink* create_sink(SinkManager::Stream* stream, std::string streamName, void* ctx) {
        return (SinkManager::Sink*)(new NinePSink(stream, streamName));
    }

    std::string name;
    bool enabled = true;
    SinkManager::SinkProvider provider;
};

MOD_EXPORT void _INIT_() {
    // Nothing here
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    NinePServer s(5640);
    s.run();
    return new NineP(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
    delete (NineP*)instance;
}

MOD_EXPORT void _END_() {
    // Nothing here
}