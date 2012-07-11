#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_
#define CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_

#include <vector>
#include <string>

#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/linenoise.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/suggester.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view.hpp"

const std::string& guarantee_param_0(const std::map<std::string, std::vector<std::string> >& params, const std::string& name);

struct admin_cluster_exc_t : public std::exception {
public:
    explicit admin_cluster_exc_t(const std::string& data) : info(data) { }
    ~admin_cluster_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

struct admin_retry_exc_t : public std::exception {
public:
    admin_retry_exc_t() { }
    ~admin_retry_exc_t() throw () { }
    const char *what() const throw () { return "metadata update to peer was rejected, try again"; }
};

class admin_cluster_link_t {
public:
    admin_cluster_link_t(const std::set<peer_address_t> &joins, int client_port, signal_t *interruptor);
    ~admin_cluster_link_t();

    // A way for the parser to do completions and parsing verification
    std::vector<std::string> get_ids(const std::string& base);
    std::vector<std::string> get_machine_ids(const std::string& base);
    std::vector<std::string> get_namespace_ids(const std::string& base);
    std::vector<std::string> get_datacenter_ids(const std::string& base);
    std::vector<std::string> get_conflicted_ids(const std::string& base);

    // Commands that may be run by the parser
    void do_admin_list(const admin_command_parser_t::command_data& data);
    void do_admin_list_stats(const admin_command_parser_t::command_data& data);
    void do_admin_list_issues(const admin_command_parser_t::command_data& data);
    void do_admin_list_machines(const admin_command_parser_t::command_data& data);
    void do_admin_list_directory(const admin_command_parser_t::command_data& data);
    void do_admin_list_namespaces(const admin_command_parser_t::command_data& data);
    void do_admin_list_datacenters(const admin_command_parser_t::command_data& data);
    void do_admin_resolve(const admin_command_parser_t::command_data& data);
    void do_admin_pin_shard(const admin_command_parser_t::command_data& data);
    void do_admin_split_shard(const admin_command_parser_t::command_data& data);
    void do_admin_merge_shard(const admin_command_parser_t::command_data& data);
    void do_admin_set_name(const admin_command_parser_t::command_data& data);
    void do_admin_set_acks(const admin_command_parser_t::command_data& data);
    void do_admin_set_replicas(const admin_command_parser_t::command_data& data);
    void do_admin_set_primary(const admin_command_parser_t::command_data& data);
    void do_admin_set_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_create_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_create_namespace(const admin_command_parser_t::command_data& data);
    void do_admin_remove_machine(const admin_command_parser_t::command_data& data);
    void do_admin_remove_namespace(const admin_command_parser_t::command_data& data);
    void do_admin_remove_datacenter(const admin_command_parser_t::command_data& data);

    void sync_from();

    size_t machine_count() const;
    size_t available_machine_count();
    size_t issue_count();

private:

    static std::string truncate_uuid(const uuid_t& uuid);

    static const size_t minimum_uuid_substring = 4;
    static const size_t uuid_output_length = 8;

    std::vector<std::string> get_ids_internal(const std::string& base, const std::string& path);

    std::string peer_id_to_machine_name(const std::string& peer_id);

    size_t get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter);

    template <class map_type>
    void do_admin_set_name_internal(map_type& obj_map, const uuid_t& id);

    template <class protocol_t>
    void do_admin_set_acks_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                    const datacenter_id_t& datacenter,
                                    int num_acks);
    template <class protocol_t>
    void do_admin_set_replicas_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                        const datacenter_id_t& datacenter,
                                        int num_replicas);

    template <class obj_map>
    void do_admin_set_name_internal(obj_map& metadata,
                                    const uuid_t& uuid,
                                    const std::string& new_name);

    void do_admin_remove_internal(const std::string& obj_type, const std::vector<std::string>& ids);

    template <class T>
    void do_admin_remove_internal_internal(std::map<uuid_t, T>& obj_map, const uuid_t& key);

    template <class protocol_t>
    void remove_machine_pinnings(const machine_id_t& machine,
                                 std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map);

    template <class protocol_t>
    namespace_id_t do_admin_create_namespace_internal(namespaces_semilattice_metadata_t<protocol_t>& ns,
                                                      const std::string& name,
                                                      int port,
                                                      const datacenter_id_t& primary);

    template <class obj_map>
    void do_admin_set_datacenter_namespace(obj_map& metadata,
                                           const uuid_t obj_uuid,
                                           const datacenter_id_t dc);

    void do_admin_set_datacenter_machine(machines_semilattice_metadata_t::machine_map_t& metadata,
                                         const uuid_t obj_uuid,
                                         const datacenter_id_t dc,
                                         cluster_semilattice_metadata_t& cluster_metadata);


    void remove_datacenter_references(const datacenter_id_t& datacenter, cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void remove_datacenter_references_from_namespaces(const datacenter_id_t& datacenter,
                                                      std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map);

    template <class map_type>
    void list_all_internal(const std::string& type, bool long_format, const map_type& obj_map, std::vector<std::vector<std::string> >& table_out);

    void list_all(bool long_format, const cluster_semilattice_metadata_t& cluster_metadata);
    void list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);

    void admin_stats_to_table(const std::string& machine,
                              const std::string& prefix,
                              const perfmon_result_t& stats,
                              std::vector<std::vector<std::string> >& table);

    template <class map_type>
    void add_namespaces(const std::string& protocol, bool long_format, map_type& namespaces, std::vector<std::vector<std::string> >& table);

    struct shard_input_t {
        struct {
            bool exists;
            bool unbounded;
            store_key_t key;
        } left, right;
    };

    template <class protocol_t>
    void do_admin_pin_shard_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                     const shard_input_t& shard_in,
                                     const std::string& primary_str,
                                     const std::vector<std::string>& secondary_strs,
                                     const cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    typename protocol_t::region_t find_shard_in_namespace(const namespace_semilattice_metadata_t<protocol_t>& ns,
                                                          const shard_input_t& shard_in);

    template <class protocol_t>
    void list_pinnings(namespace_semilattice_metadata_t<protocol_t>& ns,
                       const shard_input_t& shard_in,
                       cluster_semilattice_metadata_t& cluster_metadata);

    template <class bp_type>
    void list_pinnings_internal(const bp_type& bp,
                                const key_range_t& shard,
                                cluster_semilattice_metadata_t& cluster_metadata);

    struct machine_info_t {
        machine_info_t() : status(), primaries(0), secondaries(0), namespaces(0) { }

        std::string status;
        size_t primaries;
        size_t secondaries;
        size_t namespaces;
    };

    template <class bp_type>
    void add_machine_info_from_blueprint(const bp_type& bp, std::map<machine_id_t, machine_info_t>& results);

    template <class map_type>
    void build_machine_info_internal(const map_type& ns_map, std::map<machine_id_t, machine_info_t>& results);

    std::map<machine_id_t, machine_info_t> build_machine_info(const cluster_semilattice_metadata_t& cluster_metadata);

    struct namespace_info_t {
        namespace_info_t() : shards(0), replicas(0), primary() { }

        // These may be set to -1 in the case of a conflict
        int shards;
        int replicas;
        std::string primary;
    };

    template <class ns_type>
    namespace_info_t get_namespace_info(const ns_type& ns);

    template <class bp_type>
    size_t get_replica_count_from_blueprint(const bp_type& bp);

    struct datacenter_info_t {
        datacenter_info_t() : machines(0), primaries(0), secondaries(0), namespaces(0) { }

        size_t machines;
        size_t primaries;
        size_t secondaries;
        size_t namespaces;
    };

    std::map<datacenter_id_t, datacenter_info_t> build_datacenter_info(const cluster_semilattice_metadata_t& cluster_metadata);

    template <class map_type>
    void add_datacenter_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t>& results);

    void list_single_datacenter(const datacenter_id_t& dc_id,
                                datacenter_semilattice_metadata_t& dc,
                                cluster_semilattice_metadata_t& cluster_metadata);

    void list_single_machine(const machine_id_t& machine_id,
                             machine_semilattice_metadata_t& machine,
                             cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void list_single_namespace(const namespace_id_t& ns_id,
                               namespace_semilattice_metadata_t<protocol_t>& ns,
                               cluster_semilattice_metadata_t& cluster_metadata,
                               const std::string& protocol);

    template <class map_type>
    void add_single_datacenter_affinities(const datacenter_id_t& dc_id,
                                          map_type& ns_map,
                                          std::vector<std::vector<std::string> >& table,
                                          const std::string& protocol);

    template <class map_type>
    size_t add_single_machine_replicas(const machine_id_t& machine_id,
                                       map_type& ns_map,
                                       std::vector<std::vector<std::string> >& table);

    template <class protocol_t>
    bool add_single_machine_blueprint(const machine_id_t& machine_id,
                                      persistable_blueprint_t<protocol_t>& blueprint,
                                      std::vector<std::vector<std::string> >& table,
                                      const std::string& ns_uuid,
                                      const std::string& ns_name);

    template <class protocol_t>
    void add_single_namespace_replicas(std::set<typename protocol_t::region_t>& shards,
                                       persistable_blueprint_t<protocol_t>& blueprint,
                                       machines_semilattice_metadata_t::machine_map_t& machine_map,
                                       std::vector<std::vector<std::string> >& table);

    template <class T>
    void resolve_value(vclock_t<T>& field);

    void resolve_machine_value(machine_semilattice_metadata_t& machine,
                               const std::string& field);

    void resolve_datacenter_value(datacenter_semilattice_metadata_t& dc,
                                  const std::string& field);

    template <class protocol_t>
    void resolve_namespace_value(namespace_semilattice_metadata_t<protocol_t>& ns,
                                 const std::string& field);

    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata);

    std::string path_to_str(const std::vector<std::string>& path);

    metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t choose_sync_peer();

    local_issue_tracker_t local_issue_tracker;
    log_writer_t log_writer;
    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;
    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    stat_manager_t stat_manager;
    log_server_t log_server;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;
    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler;
    message_multiplexer_t::client_t directory_manager_client;
    watchable_variable_t<cluster_directory_metadata_t> our_directory_metadata;
    directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager;
    directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;
    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    // Issue tracking etc.
    admin_tracker_t admin_tracker;

    // Initial join
    initial_joiner_t initial_joiner;

    machine_id_t change_request_id;
    peer_id_t sync_peer_id;

    struct metadata_info_t {
        uuid_t uuid;
        std::string name;
        std::vector<std::string> path;
    };

    std::map<std::string, metadata_info_t*> uuid_map;
    std::multimap<std::string, metadata_info_t*> name_map;

    void clear_metadata_maps();
    void update_metadata_maps();
    template <class T>
    void add_subset_to_maps(const std::string& base, T& data_map);
    metadata_info_t* get_info_from_id(const std::string& id);

    DISABLE_COPYING(admin_cluster_link_t);
};

#endif /* CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_ */
