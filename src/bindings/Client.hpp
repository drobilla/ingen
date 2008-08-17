
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
	
	virtual void response_ok(int32_t id) {}
	
	virtual void response_error(int32_t id, const std::string& msg) {}
	
	virtual void enable()  {}
	
	/** Signifies the client does not wish to receive any messages until
	 * enable is called.  Useful for performance and avoiding feedback.
	 */
	virtual void disable()  {}

	/** Bundles are a group of messages that are guaranteed to be in an
	 * atomic unit with guaranteed order (eg a packet).  For datagram
	 * protocols (like UDP) there is likely an upper limit on bundle size.
	 */
	virtual void bundle_begin()  {}
	virtual void bundle_end()    {}
	
	/** Transfers are 'weak' bundles.  These are used to break a large group
	 * of similar/related messages into larger chunks (solely for communication
	 * efficiency).  A bunch of messages in a transfer will arrive as 1 or more
	 * bundles (so a transfer can exceed the maximum bundle (packet) size).
	 */
	virtual void transfer_begin()  {}
	virtual void transfer_end()    {}
	
	virtual void error(const std::string& msg)  {}
	
	virtual void num_plugins(uint32_t num_plugins)  {}
	
	virtual void new_plugin(const std::string& uri,
	                        const std::string& type_uri,
	                        const std::string& symbol,
	                        const std::string& name)  {}
	
	virtual void new_patch(const std::string& path, uint32_t poly)  {}
	
	virtual void new_node(const std::string& plugin_uri,
	                      const std::string& node_path,
	                      bool               is_polyphonic,
	                      uint32_t           num_ports)  {}
	
	virtual void new_port(const std::string& path,
	                      uint32_t           index,
	                      const std::string& data_type,
	                      bool               is_output)  {}
	
	virtual void patch_cleared(const std::string& path)  {}
	
	virtual void object_renamed(const std::string& old_path,
	                            const std::string& new_path)  {}
	
	virtual void object_destroyed(const std::string& path)  {}
	
	virtual void connect(const std::string& src_port_path,
	                     const std::string& dst_port_path)  {}
	
	virtual void disconnect(const std::string& src_port_path,
	                        const std::string& dst_port_path)  {}
	
	virtual void set_variable(const std::string& subject_path,
	                          const std::string& predicate,
	                          const Raul::Atom&  value)  {}
	
	virtual void set_port_value(const std::string& port_path,
	                            const std::string& type_uri,
	                            uint32_t           data_size,
	                            const void*        data) {}
	
	virtual void set_port_value(const std::string& port_path,
	                            const std::string& type_uri,
	                            uint32_t           voice,
	                            uint32_t           data_size,
	                            const void*        data) {}
	
	virtual void program_add(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program,
	                         const std::string& program_name)  {}
	
	virtual void program_remove(const std::string& node_path,
	                            uint32_t           bank,
	                            uint32_t           program)  {}
};
