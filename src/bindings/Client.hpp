#include "interface/Resource.hpp"

/** Need a stub ClientInterface without pure virtual methods
 * to allow inheritance in the script
 */
class Client : public Ingen::Shared::ClientInterface
{
public:
    /** Wrapper for engine->register_client to appease SWIG */
    virtual void subscribe(Ingen::Shared::EngineInterface* engine) {
        engine->register_client(this);
    }

	void bundle_begin() {}
	void bundle_end() {}
	void transfer_begin() {}
	void transfer_end() {}
	void response_ok(int32_t id) {}
	void response_error(int32_t id, const std::string& msg) {}
	void error(const std::string& msg) {}
	void put(const Raul::URI& path, const Ingen::Shared::Resource::Properties& properties) {}
	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void del(const Raul::Path& path) {}
	void move(const Raul::Path& old_path, const Raul::Path& new_path) {}
	void disconnect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value) {}
	void set_voice_value(const Raul::Path& port_path, uint32_t voice, const Raul::Atom& value) {}
	void activity(const Raul::Path& port_path) {}
	void binding(const Raul::Path& path, const MessageType& type) {}
};
