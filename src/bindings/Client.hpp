#include "ingen/Resource.hpp"

/** Need a stub ClientInterface without pure virtual methods
 * to allow inheritance in the script
 */
class Client : public Ingen::ClientInterface
{
public:
    /** Wrapper for engine->register_client to appease SWIG */
    virtual void subscribe(Ingen::ServerInterface* engine) {
        engine->register_client(this);
    }

	void bundle_begin() {}
	void bundle_end() {}
	void response(int32_t id, Status status) {}
	void error(const std::string& msg) {}
	void put(const Raul::URI& path, const Ingen::Resource::Properties& properties) {}
	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void del(const Raul::URI& uri) {}
	void move(const Raul::Path& old_path, const Raul::Path& new_path) {}
	void disconnect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value) {}
};
