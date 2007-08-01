
/** Need a stub ClientInterface without pure virtual methods
 * to allow inheritance in the script
 */
class Client : public Ingen::Shared::ClientInterface
{
public:
    //Client() : Ingen::Shared::ClientInterface() {}
	
    /** Wrapper for engine->register_client to appease SWIG */
    virtual void subscribe(Ingen::Shared::EngineInterface* engine) {
        engine->register_client("FIXME", this);
    }
	
	virtual void response(int32_t id, bool success, std::string msg) {}
	
	virtual void enable() {}
	
	/** Signifies the client does not wish to receive any messages until
	 * enable is called.  Useful for performance and avoiding feedback.
	 */
	virtual void disable() {}

	/** Bundles are a group of messages that are guaranteed to be in an
	 * atomic unit with guaranteed order (eg a packet).  For datagram
	 * protocols (like UDP) there is likely an upper limit on bundle size.
	 */
	virtual void bundle_begin() {}
	virtual void bundle_end()   {}
	
	/** Transfers are 'weak' bundles.  These are used to break a large group
	 * of similar/related messages into larger chunks (solely for communication
	 * efficiency).  A bunch of messages in a transfer will arrive as 1 or more
	 * bundles (so a transfer can exceep the maximum bundle (packet) size).
	 */
	virtual void transfer_begin() {}
	virtual void transfer_end()   {}
	
	virtual void error(std::string msg) {}
	
	virtual void num_plugins(uint32_t num_plugins) {}
	
	virtual void new_plugin(std::string uri,
	                        std::string type_uri,
	                        std::string name) {}
	
	virtual void new_patch(std::string path, uint32_t poly) {}
	
	virtual void new_node(std::string plugin_uri,
	                      std::string node_path,
	                      bool        is_polyphonic,
	                      uint32_t    num_ports) {}
	
	virtual void new_port(std::string path,
	                      std::string data_type,
	                      bool        is_output) {}
	
	virtual void patch_enabled(std::string path) {}
	
	virtual void patch_disabled(std::string path) {}
	
	virtual void patch_cleared(std::string path) {}
	
	virtual void object_renamed(std::string old_path,
	                            std::string new_path) {}
	
	virtual void object_destroyed(std::string path) {}
	
	virtual void connection(std::string src_port_path,
	                        std::string dst_port_path) {}
	
	virtual void disconnection(std::string src_port_path,
	                           std::string dst_port_path) {}
	
	virtual void metadata_update(std::string subject_path,
	                             std::string predicate,
	                             Raul::Atom  value) {}
	
	virtual void control_change(std::string port_path,
	                            float       value) {}
	
	virtual void program_add(std::string node_path,
	                         uint32_t    bank,
	                         uint32_t    program,
	                         std::string program_name) {}
	
	virtual void program_remove(std::string node_path,
	                            uint32_t    bank,
	                            uint32_t    program) {}
};
